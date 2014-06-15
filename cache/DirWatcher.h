#pragma once
#include "stdafx.h"

#include "GitStatus.h"
#include "Watcher\ReadDirectoryChanges.h"

class CDirWatcher
{
private:
	vector<CGitStatus *> m_watchedPaths;
	CReadDirectoryChanges m_readDirChanges;

	CGitStatus* TryGetFromCache(const wstring& path);

	void BeginWatch(CGitStatus *gs);
	static DWORD WINAPI ThreadStart(LPVOID lpvParam);
	CComCriticalSection m_critSec;

	struct WaitObject
	{
		CDirWatcher *me;
		CGitStatus *gs;
		HANDLE threadInstance;
		DWORD threadId;
	};

public:
	CDirWatcher();
	~CDirWatcher();


	/**
	* Remove from watch.
	*/
	void RemovePath(const wstring& path);

	/**
	* Start the watcher.
	*/
	void Start();

	/**
	 * Gets the status of the given path.
	 */
	CGitStatus* GetStatus(const wstring& path);

	/**
	* Stop the watcher.
	*/
	void Stop();
};

