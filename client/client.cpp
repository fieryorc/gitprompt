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

const wstring GetState(const CacheServiceResponse& response)
{
	wstring result;
	DWORD status = response.state;
	switch (response.state)
	{
	case GIT_REPOSITORY_STATE_NONE:
		result = L"";
		break;
	case GIT_REPOSITORY_STATE_MERGE:
		result = L"Merge";
		break;
	case GIT_REPOSITORY_STATE_REVERT:
		result = L"Revert";
		break;
	case GIT_REPOSITORY_STATE_CHERRY_PICK:
		result = L"Cherry-pick";
		break;
	case GIT_REPOSITORY_STATE_BISECT:
		result = L"Bisect";
		break;
	case GIT_REPOSITORY_STATE_REBASE:
		result = L"Rebase";
		break;
	case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
		result = L"Rebase-i";
		break;
	case GIT_REPOSITORY_STATE_REBASE_MERGE:
		result = L"Rebase-Merge";
		break;
	case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
		result = L"Apply-Mailbox";
		break;
	case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
		result = L"Apply-Mailbox/Rebase";
		break;
	default:
		result = L"unknown";
		break;
	}
	return result;		
}

int _tmain(int argc, _TCHAR* argv[])
{
	CRemoteLink remoteLink;
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
	wstring_convert<codecvt_utf8<wchar_t>> converter;
	wstring branch(response.branch);
	if (response.isSuccess)
	{
		UINT codePage = 65001; // UTF-8
		UINT oldCodePage = GetConsoleOutputCP();
		if (IsValidCodePage(codePage))
			SetConsoleOutputCP(codePage);
		wprintf(L"(%S) i[+%d, -%d, ~%d] w[+%d, -%d, ~%d] (%s)", converter.to_bytes(branch).c_str(), response.n_addedIndex, response.n_deletedIndex, response.n_modifiedIndex, response.n_addedWorkDir, response.n_deletedWorkDir, response.n_modifiedWorkDir, GetState(response).c_str());
		SetConsoleOutputCP(oldCodePage);
		return 0;
	}
	return 1;
}

