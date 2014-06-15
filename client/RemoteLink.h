#pragma once
#include "stdafx.h"
#include "..\cache\Message.h"

class CRemoteLink
{
public:
	CRemoteLink();
	~CRemoteLink();

public:
	bool GetStatus(const wstring& path, CacheServiceResponse& response);

private:
	bool InternalEnsurePipeOpen(HANDLE& hPipe, const wstring& pipeName) const;

	bool EnsurePipeOpen();
	void ClosePipe();

	DWORD GetProcessIntegrityLevel() const;
	bool RunCacheServiceProcess();
	wstring CRemoteLink::GetCachePipeName();
	wstring CRemoteLink::GetCacheMutexName();
	wstring CRemoteLink::GetCacheID();

private:
	HANDLE m_hPipe;
	OVERLAPPED m_Overlapped;
	HANDLE m_hEvent;
};
