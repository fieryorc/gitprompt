#include "stdafx.h"
#include "RemoteLink.h"
#include "ConsoleLogger.h"
#include <memory>
#include "..\cache\CacheService.h"
#include <TlHelp32.h>
#include <WinBase.h>
#include <process.h>

CRemoteLink::CRemoteLink(void)
	: m_hPipe(INVALID_HANDLE_VALUE),
	m_hEvent(0)	
{
	SecureZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
}

CRemoteLink::~CRemoteLink(void)
{
	//ClosePipe();
}

bool CRemoteLink::InternalEnsurePipeOpen(HANDLE& hPipe
	, const wstring& pipeName) const
{
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	if (hPipe != INVALID_HANDLE_VALUE)
		return true;

	int tryleft = 2;

	while (hPipe == INVALID_HANDLE_VALUE && tryleft--)
	{

		hPipe = CreateFile(
			pipeName.c_str(),                       // pipe name
			GENERIC_READ |                  // read and write access
			GENERIC_WRITE,
			0,                              // no sharing
			NULL,                           // default security attributes
			OPEN_EXISTING,                  // opens existing pipe
			FILE_FLAG_OVERLAPPED,           // default attributes
			NULL);                          // no template file
		if ((hPipe != INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_PIPE_BUSY))
		{
			// Service is busy connecting a different client.
			if (!WaitNamedPipe(pipeName.c_str(), 50))
			{
				continue;
			}
		}
	}

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		// The pipe connected; change to message-read mode.
		DWORD dwMode;

		dwMode = PIPE_READMODE_MESSAGE;
		if (!SetNamedPipeHandleState(
			hPipe,    // pipe handle
			&dwMode,  // new pipe mode
			NULL,     // don't set maximum bytes
			NULL))    // don't set maximum time
		{
			Logger::LogError(converter.from_bytes(__FUNCTION__ ": SetNamedPipeHandleState failed"));
			CloseHandle(hPipe);
		}		
	}

	return hPipe != INVALID_HANDLE_VALUE;
}

bool CRemoteLink::EnsurePipeOpen()
{
	if (InternalEnsurePipeOpen(m_hPipe, GetCachePipeName()))
	{
		// create an unnamed (=local) manual reset event for use in the overlapped structure
		if (m_hEvent)
			return true;

		m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hEvent)
			return true;

		wstring_convert<codecvt_utf8<wchar_t>> converter;
		Logger::LogError(converter.from_bytes(__FUNCTION__ ": CreateEvent failed"));
		ClosePipe();
	}

	return false;
}

void CRemoteLink::ClosePipe()
{
	CloseHandle(m_hPipe);
	CloseHandle(m_hEvent);
}

bool CRemoteLink::GetStatus(const wstring& path, CacheServiceResponse& response)
{
	SecureZeroMemory(&response, sizeof(response));
	if (!EnsurePipeOpen())
	{
		// if we're in protected mode, don't try to start the cache: since we're
		// here, we know we can't access it anyway and starting a new process will
		// trigger a warning dialog in IE7+ on Vista - we don't want that.
		if (GetProcessIntegrityLevel() < SECURITY_MANDATORY_MEDIUM_RID)
			return false;

		if (!EnsureCacheServiceProcess())
			return false;

		if (!EnsurePipeOpen())
			return false;
	}

	DWORD nBytesRead;
	CacheServiceRequest request;

	wcsncpy_s(request.path, path.c_str(), sizeof(request.path));
	SecureZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
	m_Overlapped.hEvent = m_hEvent;
	// Do the transaction in overlapped mode.
	// That way, if anything happens which might block this call
	// we still can get out of it. We NEVER MUST BLOCK THE SHELL!
	// A blocked shell is a very bad user impression, because users
	// who don't know why it's blocked might find the only solution
	// to such a problem is a reboot and therefore they might loose
	// valuable data.
	// One particular situation where the shell could hang is when
	// the cache crashes and our crash report dialog comes up.
	// Sure, it would be better to have no situations where the shell
	// even can get blocked, but the timeout of 10 seconds is long enough
	// so that users still recognize that something might be wrong and
	// report back to us so we can investigate further.

	BOOL fSuccess = TransactNamedPipe(m_hPipe,
		&request, sizeof(request),
		&response, sizeof(response),
		&nBytesRead, &m_Overlapped);

	if (!fSuccess)
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			//OutputDebugStringA("TortoiseShell: TransactNamedPipe failed\n");
			ClosePipe();
			return false;
		}

		// TransactNamedPipe is working in an overlapped operation.
		// Wait for it to finish
		DWORD dwWait = WaitForSingleObject(m_hEvent, PIPE_TIMEOUT);
		if (dwWait == WAIT_OBJECT_0)
		{
			fSuccess = GetOverlappedResult(m_hPipe, &m_Overlapped, &nBytesRead, FALSE);
		}
		else
		{
			// the cache didn't respond!
			fSuccess = FALSE;
		}
	}

	if (fSuccess)
	{
		return true;
	}
	ClosePipe();
	return false;
}
DWORD CRemoteLink::GetProcessIntegrityLevel() const
{
	DWORD dwIntegrityLevel = SECURITY_MANDATORY_MEDIUM_RID;

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hToken;
	if (OpenProcessToken(hProcess, TOKEN_QUERY |
		TOKEN_QUERY_SOURCE, &hToken))
	{
		// Get the Integrity level.
		DWORD dwLengthNeeded;
		if (!GetTokenInformation(hToken, TokenIntegrityLevel,
			NULL, 0, &dwLengthNeeded))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_INSUFFICIENT_BUFFER)
			{
				PTOKEN_MANDATORY_LABEL pTIL =
					(PTOKEN_MANDATORY_LABEL)LocalAlloc(0, dwLengthNeeded);
				if (pTIL != NULL)
				{
					if (GetTokenInformation(hToken, TokenIntegrityLevel,
						pTIL, dwLengthNeeded, &dwLengthNeeded))
					{
						dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
							(DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));
					}
					LocalFree(pTIL);
				}
			}
		}
	}

	return dwIntegrityLevel;
}

bool CRemoteLink::EnsureCacheServiceProcess()
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	bool found = false;
	while (hRes)
	{
		wstring procName = pEntry.szExeFile;
		std::transform(procName.begin(), procName.end(), procName.begin(), ::toupper);

		if (procName == CACHE_PROCESS_NAME)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
				(DWORD)pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				// Process is running.
				found = true;
				break;
			}
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);

	if (!found)
	{
		vector<wchar_t> fileNameBuffer(MAX_PATH + 1);
		int length = GetModuleFileName(NULL, &fileNameBuffer[0], MAX_PATH + 1);
		wstring fileName(&fileNameBuffer[0], length);
		wstring exeDir = fileName.substr(0, fileName.find_last_of(L"/\\"));
		wstring exeFilePath = exeDir + L"\\" + CACHE_PROCESS_NAME;

		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		GetStartupInfo(&startupInfo);
		PROCESS_INFORMATION procInfo;
		if (!CreateProcess(exeFilePath.c_str(), NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, exeDir.c_str(), &startupInfo, &procInfo))
		{
			Logger::LogError(L"Unable to run cache service.");
			return false;
		}
		Sleep(2000);
	}

	return true;
}


wstring CRemoteLink::GetCachePipeName()
{
	return CACHE_NAME + GetCacheID();
}

wstring CRemoteLink::GetCacheMutexName()
{
	return CACHE_MUTEX_NAME + GetCacheID();
}

wstring CRemoteLink::GetCacheID()
{
	CString t;
	HANDLE token;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
	if (result)
	{
		DWORD len = 0;
		GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
		if (len >= sizeof(TOKEN_STATISTICS))
		{
			std::unique_ptr<BYTE[]> data(new BYTE[len]);
			GetTokenInformation(token, TokenStatistics, data.get(), len, &len);
			LUID uid = ((PTOKEN_STATISTICS)data.get())->AuthenticationId;
			t.Format(_T("-%08x%08x"), uid.HighPart, uid.LowPart);
		}
	}
	return t.GetString();
}
