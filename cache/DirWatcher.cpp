#include "stdafx.h"
#include "DirWatcher.h"
#include <algorithm>
#include <string>

CDirWatcher::CDirWatcher()
	: m_readDirChanges(),
	  m_watchedPaths()
{

}


CDirWatcher::~CDirWatcher()
{
}

CGitStatus* CDirWatcher::TryGetFromCache(const wstring& path)
{
	auto result = std::find(this->m_watchedPaths.begin(), this->m_watchedPaths.end(), [path](CGitStatus* gs) {
		return path.compare(gs->GetPath()) == 0;
	});

	return *result;
}

CGitStatus* CDirWatcher::AddPath(const wstring& path)
{
	this->m_readDirChanges.AddDirectory(path.c_str(), true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME);
	CGitStatus* gs = new CGitStatus(path);
	gs->Load();
	this->m_watchedPaths.push_back(gs);		
}

void CDirWatcher::RemovePath(const wstring& path)
{
	// TODO:
}

void CDirWatcher::Start()
{
	this->m_readDirChanges.Init();
}

void CDirWatcher::Stop()
{
	this->m_readDirChanges.Terminate();
}

CGitStatus& CDirWatcher::GetStatus(const wstring& path)
{
	CGitStatus *gs = TryGetFromCache(path);
	if (gs != nullptr)
		return *gs;

	gs = AddPath(path);
	return *gs;
}
