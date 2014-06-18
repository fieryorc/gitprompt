#include "stdafx.h"
#include "DirectoryMonitor.h"
#include "DebugLogger.h"

CDirectoryMonitor::CDirectoryMonitor(const wstring& path)
:m_path(path),
m_callback(nullptr),
m_callbackContext(nullptr),
m_hDirectory(INVALID_HANDLE_VALUE)
{
	::ZeroMemory(&m_overlapped, sizeof(m_overlapped));
	this->m_Buffer.resize(16384);
}


CDirectoryMonitor::~CDirectoryMonitor()
{
	if (this->m_hDirectory != INVALID_HANDLE_VALUE)
	{
		CancelIo(this->m_hDirectory);
		CloseHandle(this->m_hDirectory);
		this->m_hDirectory = INVALID_HANDLE_VALUE;
	}
}

bool CDirectoryMonitor::OpenDirectory()
{
	// Allow this routine to be called redundantly.
	if (m_hDirectory != INVALID_HANDLE_VALUE)
		return true;

	m_hDirectory = ::CreateFile(
		this->m_path.c_str(),    		    // pointer to the file name
		FILE_LIST_DIRECTORY,                // access (read/write) mode
		FILE_SHARE_READ						// share mode
		| FILE_SHARE_WRITE
		| FILE_SHARE_DELETE,
		NULL,                               // security descriptor
		OPEN_EXISTING,                      // how to create
		FILE_FLAG_BACKUP_SEMANTICS			// file attributes
		| FILE_FLAG_OVERLAPPED,
		NULL);                              // file with attributes to copy

	if (m_hDirectory == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwThreadId;
	HANDLE hInstanceThread = CreateThread(
		NULL,              // no security attribute
		0,                 // default stack size
		CDirectoryMonitor::ThreadStart,
		this,			   // thread parameter
		0,                 // not suspended
		&dwThreadId);      // returns thread ID

	if (!hInstanceThread)
	{
		Logger::LogError(L"Unable to start new thread");
		// TODO:
		// ExitProcess(2);
		return false;
	}
	return true;
}

DWORD WINAPI CDirectoryMonitor::ThreadStart(LPVOID lpvParam)
{
	CDirectoryMonitor *me = (CDirectoryMonitor*)lpvParam;

	while (me->WaitForChanges());

	me->Notify(true);
	return 0;
}

/**
 * Returns true if the call needs to be restarted.
 */
bool CDirectoryMonitor::WaitForChanges()
{
	DWORD dwBytes = 0;

	// This call needs to be reissued after every APC.
	BOOL success = ::ReadDirectoryChangesW(
		this->m_hDirectory,					// handle to directory
		&this->m_Buffer[0],                 // read results buffer
		this->m_Buffer.size(),				// length of buffer
		TRUE,								// monitoring option
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE,        // filter conditions
		&dwBytes,                           // bytes returned
		&this->m_overlapped,                // overlapped buffer
		NULL);								// completion routine

	if (!success)
	{
		Logger::LogError(L"Failed: ReadDirectoryChangesW");
		this->Notify(false);
		return false;
	}

	BOOL result = GetOverlappedResult(this->m_hDirectory, &this->m_overlapped, &dwBytes, TRUE);
	if (!result || dwBytes <= 0)
		return false;

	FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)&this->m_Buffer[0];

	if (!IsIgnorable(info))
		return false;
	else
	{
		vector<wchar_t> strBuffer(1024);
		wnsprintfW(&strBuffer[0], strBuffer.size(), L"File '%s' changed. Ignoring and restarting monitor", info->FileName);
		Logger::LogInfo(&strBuffer[0]);
	}

	return true;
}

bool CDirectoryMonitor::IsIgnorable(FILE_NOTIFY_INFORMATION *info)
{
	wstring fileName = info->FileName;
	if (info->Action == FILE_ACTION_MODIFIED && fileName.compare(L".git") == 0)
		return true;
	if (fileName.compare(L".git\\index.lock") == 0)
		return true;
	return false;
}


bool CDirectoryMonitor::Monitor(ChangeCallback callback, void *context)
{
	this->m_callback = callback;
	this->m_callbackContext = context;

	if (!this->OpenDirectory())
	{
		Logger::LogError(L"Unable to open directory : " + this->m_path);
		this->Notify(false);
		return false;
	}
	return true;
}

void CDirectoryMonitor::Notify(bool isSucceeded)
{
	if (this->m_callback != nullptr)
	{
		// TODO: Pass in the correct change type.
		this->m_callback(ChangeType::FILE_MODIFIED, this->m_callbackContext);
	}
}

