// client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <codecvt>
#include "git2.h"
#include "ConsoleLogger.h"
#include "RemoteLink.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CRemoteLink remoteLink;
	CacheServiceRequest request;
	wchar_t* currentDir = new wchar_t[MAX_PATH];
	if (GetCurrentDirectory(MAX_PATH, currentDir) <= 0)
	{
		Logger::LogError(_T("Unable to get current directory."));
		return 2;
	}

	CacheServiceResponse response;
	remoteLink.GetStatus(currentDir, response);
	
	wprintf(L"(%s) [+%d, -%d, ~%d]", response.branch, response.n_added, response.n_deleted, response.n_modified);
	
	return 0;
}
