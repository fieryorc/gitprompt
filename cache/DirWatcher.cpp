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

CGitStatus* CDirWatcher::GetFromCache(const wstring& repoRootDir)
{
	CriticalSection lock(this->m_critSec);
	auto result = std::find_if(this->m_watchedPaths.begin(), this->m_watchedPaths.end(), [repoRootDir](CGitStatus* gs) {
		return repoRootDir.compare(gs->GetPath()) == 0;
	});

	if (result != this->m_watchedPaths.end())
		return *result;

	CGitStatus *gs = new CGitStatus(repoRootDir);
	this->m_watchedPaths.push_back(gs);

	return gs;
}

CGitStatus* CDirWatcher::GetStatus(const wstring& path)
{
	wstring repoRoot;
	CGitStatus::GetRepoRoot(path, repoRoot);
	CGitStatus *gs = GetFromCache(repoRoot);

	gs->Load();
	return gs;
}
