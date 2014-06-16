// client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <codecvt>
#include <algorithm>
#include "git2.h"
#include "ConsoleLogger.h"
#include "RemoteLink.h"
#include <TlHelp32.h>
#include <WinBase.h>
#include <process.h>

void killProcessByName(const wstring& name)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	wstring nameToMatch = name;
	std::transform(nameToMatch.begin(), nameToMatch.end(), nameToMatch.begin(), ::toupper);

	while (hRes)
	{
		wstring procName = pEntry.szExeFile;
		std::transform(procName.begin(), procName.end(), procName.begin(), ::toupper);

		if (procName == nameToMatch)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
				(DWORD)pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				TerminateProcess(hProcess, 9);
				CloseHandle(hProcess);
			}
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
}

int _tmain(int argc, _TCHAR* argv[])
{
	CRemoteLink remoteLink;
	CacheServiceRequest request;
	wchar_t* currentDir = new wchar_t[MAX_PATH];
	int index = 1;
	if (argc == 2)
	{
		wcscpy(currentDir, argv[index++]);
	}
	else
	{
		if (GetCurrentDirectory(MAX_PATH, currentDir) <= 0)
		{
			Logger::LogError(_T("Unable to get current directory."));
			return 2;
		}
	}

	CacheServiceResponse response;
	remoteLink.GetStatus(currentDir, response);
	if (response.isSuccess)
	{
		wprintf(L"(%s) [+%d, -%d, ~%d]", response.branch, response.n_added, response.n_deleted, response.n_modified);
		return 0;
	}
	return 1;
}

