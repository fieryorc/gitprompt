#include "stdafx.h"
#include "GitStatusList.h"
#include "DebugLogger.h"

#include <algorithm>
#include <string>

CGitStatusList::CGitStatusList()
{
	this->m_critSec.Init();
}


CGitStatusList::~CGitStatusList()
{
	m_critSec.Term();
}

CGitStatus* CGitStatusList::GetFromCache(const wstring& repoRootDir)
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

CGitStatus* CGitStatusList::GetStatus(const wstring& path)
{
	wstring repoRoot;
	if (!CGitStatus::GetRepoRoot(path, repoRoot))
		return nullptr;
	CGitStatus *gs = GetFromCache(repoRoot);
		
	return gs;
}
