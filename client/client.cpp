// client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <codecvt>
#include "git2.h"
#include "Logger.h"


extern int status_cb(const char *path, unsigned int status_flags, void *payload);



int _tmain(int argc, _TCHAR* argv[])
{
	char* currentDir = new char[MAX_PATH];
	if (GetCurrentDirectoryA(MAX_PATH, currentDir) <= 0)
	{
		Logger::LogError(_T("Unable to get current directory."));
		return 2;
	}

	git_buf buf;
	SecureZeroMemory(&buf, sizeof(git_buf));
	if (git_repository_discover(&buf, currentDir, 0, NULL))
	{
		Logger::LogWarning(_T("Unable to find git repository"));
		return 1;
	}

	git_repository  *repo = NULL;
	if (git_repository_open(&repo, buf.ptr))
	{
		Logger::LogError(_T("Unable to open repository"));
		return 1;
	}
	
	int state = git_repository_state(repo);
	
	git_status_foreach(repo, status_cb, &repo);
	
	git_buf_free(&buf);
	return 0;
}

int status_cb(const char *path, unsigned int status_flags, void *payload)
{
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	Logger::LogMessage(converter.from_bytes(path));
	return 0;
}
