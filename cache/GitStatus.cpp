#include "stdafx.h"
#include <codecvt>
#include "GitStatus.h"
#include "DebugLogger.h"
#include "git2.h"

CGitStatus::CGitStatus(const wstring& startDir)
			: m_startDir(startDir),
			m_status(GS_UNINITIALIZED),
			m_added(0),
			m_modified(0),
			m_deleted(0),
			m_repoState(GIT_REPOSITORY_STATE_NONE)
{

}


CGitStatus::~CGitStatus()
{
}

void CGitStatus::SetStatus(CGitStatus::GitStatus status)
{
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

	git_buf buf;
	SecureZeroMemory(&buf, sizeof(git_buf));
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	string cpath = converter.to_bytes(this->m_startDir);
	if (git_repository_discover(&buf, cpath.c_str(), 0, NULL))
	{
		Logger::LogWarning(_T("Unable to find git repository"));
		SetStatus(GS_ERROR);
		return;
	}

	git_repository  *repo = NULL;
	if (git_repository_open(&repo, buf.ptr))
	{
		Logger::LogError(_T("Unable to open repository"));
		SetStatus(GS_ERROR);
		return;
	}

	int state = git_repository_state(repo);

	git_reference *ref;
	if (git_repository_head(&ref, repo))
	{
		Logger::LogError(_T("Unable to retrieve branch"));
		SetStatus(GS_ERROR);
		return;
	}

	const char* headName = git_reference_shorthand(ref);
	this->m_branch = converter.from_bytes(headName);
	
	git_status_foreach(repo, &GitStatus_Callack, this);

	this->m_repoState = (GitRepoState)state;
	this->m_repoPath = converter.from_bytes(buf.ptr);
	git_buf_free(&buf);
	this->SetStatus(GS_LOADED);
	return;
}

bool CGitStatus::GetRepoRoot(const wstring& path, wstring& repoRoot_out)
{
	git_buf buf;
	SecureZeroMemory(&buf, sizeof(git_buf));
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	string cpath = converter.to_bytes(path);
	if (git_repository_discover(&buf, cpath.c_str(), 0, NULL))
	{
		Logger::LogWarning(_T("Unable to find git repository"));
		return false;
	}
	repoRoot_out = converter.from_bytes(buf.ptr);
	git_buf_free(&buf);
	return true;
}

CGitStatus::GitStatus CGitStatus::GetStatus()
{
	return this->m_status;
}

wstring& CGitStatus::GetPath()
{
	return this->m_repoPath;
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