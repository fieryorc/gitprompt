#pragma once
#include "git2.h"
#include "DirWatcher.h"

/**
 * Request structure.
 */
struct CacheServiceRequest
{
	DWORD request;
	WCHAR path[MAX_PATH + 1];
};

// CustomActions will use this header but does not need nor understand the SVN types ...

/**
 * The structure returned as a response
 */
struct CacheServiceResponse
{
	DWORD state;
	DWORD n_added;
	DWORD n_modified;
	DWORD n_deleted;
};


class CCacheService
{
private:
	const wstring CACHE_NAME = L"\\\\.\\pipe\\GitPrompt";
	const wstring CACHE_MUTEX_NAME = L"GitPromptMutex";
	const int BUFSIZE = 4096;

	static volatile LONG nThreadCount;
	static CCacheService* s_instance;


	CDirWatcher* m_dirWatcher;

	wstring CCacheService::GetCachePipeName();
	wstring CCacheService::GetCacheCommandPipeName();
	wstring CCacheService::GetCacheMutexName();
	wstring CCacheService::GetCacheID();
	static DWORD WINAPI CCacheService::ThreadStart(LPVOID lpvParam);

	static void ProcessRequest(CacheServiceRequest& request, CacheServiceResponse& response, int& length);

	static CCacheService* Instance();
public:
	CCacheService();
	~CCacheService();

	void Start();

	void Stop();
};

