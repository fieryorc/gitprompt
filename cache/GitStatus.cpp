#include "stdafx.h"
#include <algorithm>
#include <codecvt>
#include "GitStatus.h"
#include "DebugLogger.h"
#include "git2.h"
#include "MyUtils.h"

CMutex CGitStatus::m_mutex;

CGitStatus::CGitStatus(const wstring& startDir)
	: m_startDir(startDir)
{
	this->m_status = GS_NOTLOADED;
	this->m_dirMonitor = nullptr;
	this->m_dirMonitorGitDir = nullptr;
	this->InitState();
	this->m_critSec.Init();	
}


CGitStatus::~CGitStatus()
{
	this->m_critSec.Term();
	this->m_waitHandle.CloseHandle();
	CleanupDirectoryMonitors();
}

void CGitStatus::InitState()
{
	this->m_addedIndex = 0;
	this->m_modifiedIndex = 0;
	this->m_deletedIndex = 0;

	this->m_addedWorkDir = 0;
	this->m_modifiedWorkDir= 0;
	this->m_deletedWorkDir= 0;

	this->m_branch.clear();
	this->m_fileList.clear();
	this->m_repoState = GIT_REPOSITORY_STATE_NONE;
}

void CGitStatus::SetStatus(CGitStatus::GitStatus status)
{
	CriticalSection lock(this->m_critSec);
	this->m_status = status;
}

int CGitStatus::GitStatus_Callack(const char *path, unsigned int status_flags, void *payload)
{
	CGitStatus* that = (CGitStatus*)payload;
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	CGitFileStatus status(converter.from_bytes(path), status_flags);
	that->m_fileList.push_back(status);

	if (status_flags & GIT_STATUS_INDEX_NEW)
		that->m_addedIndex++;
	if (status_flags & GIT_STATUS_WT_NEW)
		that->m_addedWorkDir++;

	if (status_flags & GIT_STATUS_INDEX_MODIFIED)
		that->m_modifiedIndex++;
	if (status_flags & GIT_STATUS_WT_MODIFIED)
		that->m_modifiedWorkDir++;

	if (status_flags & GIT_STATUS_INDEX_DELETED)
		that->m_deletedIndex++;
	if (status_flags & GIT_STATUS_WT_DELETED)
		that->m_deletedWorkDir++;

	return 0;
}

void CGitStatus::Load()
{
	GitStatus status;

	CriticalSection lock(this->m_critSec);
	status = this->GetStatus();
	if (status == GS_NOTLOADED || status == GS_INVALIDATED)
	{
		this->m_waitHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
		SetStatus(GS_LOADING);
	}
	lock.Unlock();

	if (status == GS_LOADED || status == GS_ERROR)
	{
		return;
	}

	// If it is loading just wait for it to complete.
	if (status == GS_LOADING)
	{
		_ASSERT(this->m_waitHandle.IsValid());
		WaitForSingleObject(this->m_waitHandle, INFINITE);
		return;
	}

	git_buf buf;
	git_repository *repo = NULL;
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	InitState();

	if (!GetRepoRootInternal(this->m_startDir, this->m_repoRoot, buf, repo))
	{
		Logger::LogWarning(_T("Unable to open git repository"));
		SetStatus(GS_ERROR);
		::SetEvent(this->m_waitHandle);
		return;
	}

	// This is temporary.
	CMutexLock mutexLock(&m_mutex);

	this->m_gitDir = MyUtils::NormalizePath(converter.from_bytes(buf.ptr));

	// Get current state of repo. (merge/rebase in progress etc)
	this->m_repoState = git_repository_state(repo);

	// Get branch info.
	git_reference *ref;
	if (git_repository_head(&ref, repo))
	{
		Logger::LogError(_T("Unable to retrieve branch"));
		SetStatus(GS_ERROR);
		::SetEvent(this->m_waitHandle);
		return;
	}
	const char* headName = git_reference_shorthand(ref);
	this->m_branch = converter.from_bytes(headName);

	// Get the status of the repo.
	git_status_options opts;
	git_status_init_options(&opts, GIT_STATUS_OPTIONS_VERSION);
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_EXCLUDE_SUBMODULES;
	git_status_foreach_ext(repo, &opts, &GitStatus_Callack, this);

	// Free stuff
	git_buf_free(&buf);
	git_repository_free(repo);

	this->SetStatus(GS_LOADED);
	::SetEvent(this->m_waitHandle);

	// Start monitoring for changes.
	this->MonitorForChanges();
	return;
}

void CGitStatus::MonitorForChanges()
{
	CleanupDirectoryMonitors();

	this->m_dirMonitor = new CDirectoryMonitor(this->m_repoRoot);
	this->m_dirMonitor->Monitor(&DirectoryChangedCallback, this);

	// If git dir is under repo root, we don't have to monitor it.
	
	wstring expectedGitDir = MyUtils::NormalizePath(this->m_repoRoot + L".git\\");
	if (expectedGitDir.compare(this->m_gitDir) != 0)
	{
		this->m_dirMonitorGitDir = new CDirectoryMonitor(this->m_gitDir);
		this->m_dirMonitorGitDir->Monitor(&DirectoryChangedCallback, this);
	}
}

void CGitStatus::DirectoryChangedCallback(CDirectoryMonitor::ChangeType type, void *context)
{
	CGitStatus *me = (CGitStatus*)context;
	CriticalSection lock(me->m_critSec);
	me->SetStatus(GS_INVALIDATED);

	me->CleanupDirectoryMonitors();
}

bool CGitStatus::GetRepoRootInternal(const wstring& path, wstring& repoRoot_out, git_buf &buf, git_repository *&repo)
{
	// This is temporary.
	CMutexLock mutexLock(&m_mutex);

	SecureZeroMemory(&buf, sizeof(git_buf));
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	string cpath = converter.to_bytes(path);

	if (git_repository_discover(&buf, cpath.c_str(), 0, NULL))
	{
		Logger::LogWarning(_T("Unable to find git repository"));
		return false;
	}

	if (git_repository_open(&repo, buf.ptr))
	{
		Logger::LogError(_T("Unable to open repository"));
		git_buf_free(&buf);
		return false;
	}

	const char* workDir = git_repository_workdir(repo);
	if (workDir == nullptr)
	{
		Logger::LogWarning(_T("No working directory. Probably bare repository."));
		git_buf_free(&buf);
		return false;
	}
	repoRoot_out = MyUtils::NormalizePath(converter.from_bytes(workDir));
	
	return true;
}

bool CGitStatus::GetRepoRoot(const wstring& path, wstring& repoRoot_out)
{
	git_buf buf;
	git_repository *repo = NULL;
	if (GetRepoRootInternal(path, repoRoot_out, buf, repo))
	{
		git_buf_free(&buf);
		git_repository_free(repo);
		return true;
	}
	return false;
}

CGitStatus::GitStatus CGitStatus::GetStatus()
{
	return this->m_status;
}

wstring& CGitStatus::GetPath()
{
	return this->m_repoRoot;
}

bool CGitStatus::GetRepoStatus(CacheServiceResponse& out)
{
	ZeroMemory(&out, sizeof(out));

	this->Load();
	this->m_branch.copy(out.branch, sizeof(out.branch), 0);
	bool isSuccess = this->GetStatus() == GS_LOADED || this->GetStatus() == GS_INVALIDATED;
	if (isSuccess)
	{
		out.n_addedIndex = this->m_addedIndex;
		out.n_addedWorkDir = this->m_addedWorkDir;
		out.n_deletedIndex = this->m_deletedIndex;
		out.n_deletedWorkDir = this->m_deletedWorkDir;
		out.n_modifiedIndex = this->m_modifiedIndex;
		out.n_modifiedWorkDir = this->m_modifiedWorkDir;
		out.state = this->m_repoState;
	}
	out.isSuccess = (BOOL)isSuccess;
	return isSuccess;
}