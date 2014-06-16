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

	return true;
}

void CDirectoryMonitor::BeginRead()
{
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
		return;
	}

}

DWORD WINAPI CDirectoryMonitor::ThreadStart(LPVOID lpvParam)
{
	CDirectoryMonitor *me = (CDirectoryMonitor*)lpvParam;
	DWORD dwBytes = 0;

	// This call needs to be reissued after every APC.
	BOOL success = ::ReadDirectoryChangesW(
		me->m_hDirectory,					// handle to directory
		&me->m_Buffer[0],                   // read results buffer
		me->m_Buffer.size(),				// length of buffer
		TRUE,								// monitoring option
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,        // filter conditions
		&dwBytes,                           // bytes returned
		&me->m_overlapped,                  // overlapped buffer
		NULL);								// completion routine

	if (!success)
	{
		Logger::LogError(L"Failed: ReadDirectoryChangesW");
		me->Notify(false);
		return 0;
	}

	BOOL result = GetOverlappedResult(me->m_hDirectory, &me->m_overlapped, &dwBytes, TRUE);
	me->Notify(result);
	return 0;
}


bool CDirectoryMonitor::Monitor(ChangeCallback callback, void *context)
{
	this->m_callback = callback;
	this->m_callbackContext = context;

	if (this->OpenDirectory())
	{
		this->BeginRead();
	}
	else
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

