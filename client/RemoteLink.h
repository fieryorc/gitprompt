#pragma once
#include "stdafx.h"
#include "..\cache\Message.h"

#define CACHE_PROCESS_NAME L"GITPROMPTCACHE.EXE"


class CRemoteLink
{
public:
	CRemoteLink();
	~CRemoteLink();

public:
	bool GetStatus(const wstring& path, CacheServiceResponse& response);

private:
	const int PIPE_TIMEOUT = 60 * 1000;
	bool InternalEnsurePipeOpen(HANDLE& hPipe, const wstring& pipeName) const;

	bool EnsurePipeOpen();
	void ClosePipe();

	DWORD GetProcessIntegrityLevel() const;
	bool EnsureCacheServiceProcess();
	wstring CRemoteLink::GetCachePipeName();
	wstring CRemoteLink::GetCacheMutexName();
	wstring CRemoteLink::GetCacheID();

private:
	HANDLE m_hPipe;
	OVERLAPPED m_Overlapped;
	HANDLE m_hEvent;
};
