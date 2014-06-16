#include "stdafx.h"
#include <codecvt>
#include "GitStatus.h"
#include "DebugLogger.h"
#include "git2.h"

CGitStatus::CGitStatus(const wstring& startDir)
	: m_startDir(startDir)
{
	this->m_status = GS_NOTLOADED;
	this->m_dirMonitor = nullptr;
	this->InitState();
	this->m_critSec.Init();	
}


CGitStatus::~CGitStatus()
{
	this->m_critSec.Term();
	this->m_waitHandle.CloseHandle();
	if (this->m_dirMonitor != nullptr)
		delete this->m_dirMonitor;
}

void CGitStatus::InitState()
{
	this->m_added = 0;
	this->m_modified = 0;
	this->m_deleted = 0;
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

	if (status_flags & GIT_STATUS_INDEX_NEW ||
		status_flags & GIT_STATUS_WT_NEW)
	{
		that->m_added++;
	}

	if (status_flags & GIT_STATUS_INDEX_MODIFIED ||
		status_flags & GIT_STATUS_WT_MODIFIED)
	{
		that->m_modified++;
	}

	if (status_flags & GIT_STATUS_INDEX_DELETED ||
		status_flags & GIT_STATUS_WT_DELETED)
	{
		that->m_deleted++;
	}

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
	this->m_gitDir = converter.from_bytes(buf.ptr);

	// Get current state of repo. (merge/rebase in progress etc)
	int state = git_repository_state(repo);

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

	this->m_repoState = (GitRepoState)state;

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
	if (this->m_dirMonitor != nullptr)
		delete this->m_dirMonitor;

	this->m_dirMonitor = new CDirectoryMonitor(this->m_repoRoot);
	this->m_dirMonitor->Monitor(&DirectoryChangedCallback, this);
}

void CGitStatus::DirectoryChangedCallback(CDirectoryMonitor::ChangeType type, void *context)
{
	CGitStatus *me = (CGitStatus*)context;
	CriticalSection(me->m_critSec);
	me->SetStatus(GS_INVALIDATED);
}

bool CGitStatus::GetRepoRootInternal(const wstring& path, wstring& repoRoot_out, git_buf &buf, git_repository *&repo)
{
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
	wstring repoRoot = converter.from_bytes(workDir);

	TCHAR repoRootNormalized[MAX_PATH + 1];
	if (GetFullPathName(repoRoot.c_str(), MAX_PATH, repoRootNormalized, NULL) <= 0)
	{
		Logger::LogError(L"Unable to convert the path using GetFullPathName : " + repoRoot);
		git_repository_free(repo);
		return false;
	}

	repoRoot_out = repoRootNormalized;
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

wstring& CGitStatus::GetBranch()
{
	return this->m_branch;
}

int CGitStatus::GetAddedFileCount()
{
	return this->m_added;
}

int CGitStatus::GetModifiedFileCount()
{
	return this->m_modified;
}

int CGitStatus::GetDeletedFileCount()
{
	return this->m_deleted;
}

CGitStatus::GitRepoState CGitStatus::GetRepoStatus()
{
	return this->m_repoState;
}