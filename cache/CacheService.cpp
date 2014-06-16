#include "stdafx.h"
#include "CacheService.h"
#include "Utils\SmartHandle.h"
#include <memory>
#include "DebugLogger.h"

volatile LONG CCacheService::nThreadCount = 0;
CCacheService* CCacheService::s_instance = nullptr;


CCacheService::CCacheService()	
	: m_dirWatcher()
{
}


CCacheService::~CCacheService()
{
}

wstring CCacheService::GetCachePipeName()
{
	return CACHE_NAME + GetCacheID();
}

wstring CCacheService::GetCacheMutexName()
{
	return CACHE_MUTEX_NAME + GetCacheID();
}

wstring CCacheService::GetCacheID()
{
	CString t;
	CAutoGeneralHandle token;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.GetPointer());
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

void CCacheService::Start()
{
	// The main loop creates an instance of the named pipe and
	// then waits for a client to connect to it. When the client
	// connects, a thread is created to handle communications
	// with that client, and the loop is repeated.
	DWORD dwThreadId;
	BOOL fConnected;
	CAutoFile hPipe;

	while (true)
	{
		hPipe = CreateNamedPipe(
			GetCachePipeName().c_str(),
			PIPE_ACCESS_DUPLEX,       // read/write access
			PIPE_TYPE_MESSAGE |       // message type pipe
			PIPE_READMODE_MESSAGE |   // message-read mode
			PIPE_WAIT,                // blocking mode
			PIPE_UNLIMITED_INSTANCES, // max. instances
			BUFSIZE,                  // output buffer size
			BUFSIZE,                  // input buffer size
			NMPWAIT_USE_DEFAULT_WAIT, // client time-out
			NULL);					  // NULL DACL

		if (!hPipe)
		{
			Logger::LogError(L"Unable to create pipe. Continuing...");
			Sleep(200);
			continue;
		}

		// Wait for the client to connect; if it succeeds,
		// the function returns a nonzero value. If the function returns
		// zero, GetLastError returns ERROR_PIPE_CONNECTED.
		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (fConnected)
		{
			// Create a thread for this client.
			
			HANDLE hInstanceThread = CreateThread(
				NULL,              // no security attribute
				0,                 // default stack size
				CCacheService::ThreadStart,
				(HANDLE)hPipe,    // thread parameter
				0,                 // not suspended
				&dwThreadId);      // returns thread ID

			if (!hInstanceThread)
			{
				DisconnectNamedPipe(hPipe);
				// since we're now closing this thread, we also have to close the whole application!
				// otherwise the thread is dead, but the app is still running, refusing new instances
				// but no pipe will be available anymore.
				// PostMessage(hWnd, WM_CLOSE, 0, 0);
				ExitProcess(2);
				return;
			}
			// detach the handle, since we passed it to the thread
			hPipe.Detach();
		}
		else
		{
			hPipe.CloseHandle();
			Sleep(200);
			continue;	// don't end the thread!
		}
	}
	
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	Logger::LogInfo(converter.from_bytes(__FUNCTION__ ": Pipe thread exited\n"));
	return;
}

DWORD WINAPI CCacheService::ThreadStart(LPVOID lpvParam)
{
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	Logger::LogInfo(converter.from_bytes(__FUNCTION__ ": InstanceThread started\n"));
	CacheServiceResponse response;
	DWORD cbBytesRead, cbWritten;
	CAutoFile hPipe;

	// The thread's parameter is a handle to a pipe instance.

	hPipe = lpvParam;
	InterlockedIncrement(&nThreadCount);
	
	{
		// Read client requests from the pipe.
		CacheServiceRequest request;
		BOOL fSuccess = ReadFile(
			hPipe,        // handle to pipe
			&request,    // buffer to receive data
			sizeof(request), // size of buffer
			&cbBytesRead, // number of bytes read
			NULL);        // not overlapped I/O

		if (!fSuccess || cbBytesRead == 0)
		{
			DisconnectNamedPipe(hPipe);
			Logger::LogError(converter.from_bytes(__FUNCTION__ ": Instance thread exited\n"));
			InterlockedDecrement(&nThreadCount);
			//if (nThreadCount == 0)
			//	PostMessage(hWnd, WM_CLOSE, 0, 0);
			return 1;
		}

		int responseLength;
		ProcessRequest(request, response, responseLength);

		// Write the reply to the pipe.
		fSuccess = WriteFile(
			hPipe,        // handle to pipe
			&response,      // buffer to write from
			responseLength, // number of bytes to write
			&cbWritten,   // number of bytes written
			NULL);        // not overlapped I/O

		if (!fSuccess || responseLength != cbWritten)
		{
			DisconnectNamedPipe(hPipe);
			Logger::LogError(converter.from_bytes(__FUNCTION__ ": Instance thread exited\n"));
			InterlockedDecrement(&nThreadCount);
			//if (nThreadCount == 0)
			//	PostMessage(hWnd, WM_CLOSE, 0, 0);
			return 1;
		}
	}

	// Flush the pipe to allow the client to read the pipe's contents
	// before disconnecting. Then disconnect the pipe, and close the
	// handle to this pipe instance.

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	Logger::LogInfo(converter.from_bytes(__FUNCTION__ ": Instance thread exited\n"));
	InterlockedDecrement(&nThreadCount);
	return 0;
}

void CCacheService::ProcessRequest(CacheServiceRequest& request, CacheServiceResponse& response, int& responseLength)
{
	wstring path = request.path;
	CGitStatus *gs = CCacheService::Instance()->m_dirWatcher.GetStatus(path);
	SecureZeroMemory(&response, sizeof(response));
	if (gs != nullptr)
	{
		response.state = gs->GetRepoStatus();
		response.n_added = gs->GetAddedFileCount();
		response.n_modified = gs->GetModifiedFileCount();
		response.n_deleted = gs->GetDeletedFileCount();
		gs->GetBranch().copy(response.branch, MAX_PATH);
	}
}


void CCacheService::Stop()
{
}

CCacheService* CCacheService::Instance()
{
	if (s_instance == nullptr)
		s_instance = new CCacheService();
	return s_instance;
}