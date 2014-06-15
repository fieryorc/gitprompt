#include "stdafx.h"
#include "DirWatcher.h"
#include "DebugLogger.h"

#include <algorithm>
#include <string>

CDirWatcher::CDirWatcher()
{
	this->m_critSec.Init();
}


CDirWatcher::~CDirWatcher()
{
	m_critSec.Term();
}

CGitStatus* CDirWatcher::TryGetFromCache(const wstring& path)
{
	auto result = std::find_if(this->m_watchedPaths.begin(), this->m_watchedPaths.end(), [path](CGitStatus* gs) {
		return path.compare(gs->GetPath()) == 0;
	});

	return result != this->m_watchedPaths.end() ? *result : nullptr;
}

void CDirWatcher::BeginWatch(CGitStatus *gs)
{
	WaitObject *waitObj = new WaitObject();
	ZeroMemory(waitObj, sizeof(waitObj));
	waitObj->gs = gs;
	waitObj->me = this;

	DWORD dwThreadId;
	HANDLE hInstanceThread = CreateThread(
		NULL,              // no security attribute
		0,                 // default stack size
		CDirWatcher::ThreadStart,
		waitObj,    // thread parameter
		0,                 // not suspended
		&dwThreadId);      // returns thread ID

	if (!hInstanceThread)
	{
		Logger::LogError(L"Unable to start new thread");
		ExitProcess(2);
		delete waitObj;
		return;
	}
	waitObj->threadId = dwThreadId;
	waitObj->threadInstance = hInstanceThread;
}

DWORD WINAPI CDirWatcher::ThreadStart(LPVOID lpvParam)
{
	WaitObject *waitObj = (WaitObject*)lpvParam;

	const HANDLE handles[] = { waitObj->me->m_readDirChanges.GetWaitHandle() };
	DWORD retVal = ::MsgWaitForMultipleObjectsEx(
		_countof(handles),
		handles,
		INFINITE,
		QS_ALLINPUT,
		MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
	switch (retVal)
	{
	case WAIT_OBJECT_0 + 0:
		// We've received a notification in the queue.
		// Remove the item from the list.
	{
		CriticalSection lock(waitObj->me->m_critSec);
		auto result = std::find(waitObj->me->m_watchedPaths.begin(), waitObj->me->m_watchedPaths.end(), waitObj->gs);
		if (result == waitObj->me->m_watchedPaths.end())
		{
			wstring_convert<codecvt_utf8<wchar_t>> converter;
			Logger::LogError(converter.from_bytes(__FUNCTION__ "Unable to get the cached object.. probably something went wrong"));
			break;
		}
		waitObj->me->m_watchedPaths.erase(result);
		lock.Unlock();

		// Wait for load to complete before destroying to avoid race conditions.
		waitObj->gs->Load();

		CriticalSection lock1(waitObj->me->m_critSec);
		delete waitObj->gs;
		lock1.Unlock();
	}
		break;
	case WAIT_OBJECT_0 + _countof(handles):
		// Get and dispatch message
		break;
	case WAIT_IO_COMPLETION:
		// APC complete.No action needed.
		break;
	}
	delete waitObj;
	return 0;
}

void CDirWatcher::RemovePath(const wstring& path)
{
}

void CDirWatcher::Start()
{
	this->m_readDirChanges.Init();
}

void CDirWatcher::Stop()
{
	this->m_readDirChanges.Terminate();
}

CGitStatus* CDirWatcher::GetStatus(const wstring& path)
{
	CriticalSection lock(this->m_critSec);
	wstring repoRoot;
	CGitStatus::GetRepoRoot(path, repoRoot);
	CGitStatus *gs = TryGetFromCache(repoRoot);

	if (gs == nullptr)
	{
		gs = new CGitStatus(repoRoot);
		this->m_readDirChanges.AddDirectory(repoRoot.c_str(), true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME);
		this->m_watchedPaths.push_back(gs);
		this->BeginWatch(gs);
	}

	lock.Unlock();
	gs->Load();
	return gs;
}
