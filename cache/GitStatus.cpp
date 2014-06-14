#include "stdafx.h"
#include <codecvt>
#include "GitStatus.h"
#include "DebugLogger.h"
#include "git2.h"

CGitStatus::CGitStatus(const wstring& startDir)
			: m_startDir(startDir)
{

}


CGitStatus::~CGitStatus()
{
}

CGitStatus::git_status CGitStatus::GetStatus()
{
	return this->m_status;
}

void CGitStatus::SetStatus(CGitStatus::git_status status)
{
	this->m_status = status;
}

wstring& CGitStatus::GetPath()
{
	return this->m_repoPath;
}

int CGitStatus::GitStatus_Callack(const char *path, unsigned int status_flags, void *payload)
{
	CGitStatus* that = (CGitStatus*)payload;
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	CGitFileStatus status(converter.from_bytes(path), status_flags);
	that->m_fileList.push_back(status);
	return 0;
}

void CGitStatus::Load()
{
	char* currentDir = new char[MAX_PATH];
	if (GetCurrentDirectoryA(MAX_PATH, currentDir) <= 0)
	{
		Logger::LogError(_T("Unable to get current directory."));
		SetStatus(GS_ERROR);
		return;
	}

	git_buf buf;
	SecureZeroMemory(&buf, sizeof(git_buf));
	if (git_repository_discover(&buf, currentDir, 0, NULL))
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

	git_status_foreach(repo, &GitStatus_Callack, this);

	git_buf_free(&buf);
	
	return;
}