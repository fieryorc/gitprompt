#pragma once
#include "stdafx.h"

#include "GitStatus.h"

class CGitStatusList
{
private:
	CComCriticalSection m_critSec;
	vector<CGitStatus *> m_watchedPaths;

	CGitStatus* GetFromCache(const wstring& path);

public:
	CGitStatusList();
	~CGitStatusList();

	/**
	 * Gets the status of the given path.
	 */
	CGitStatus* GetStatus(const wstring& path);
};

