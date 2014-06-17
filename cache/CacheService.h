#pragma once
#include "git2.h"
#include "GitStatusList.h"
#include "Message.h"

#define CACHE_NAME				L"\\\\.\\pipe\\GitPrompt"
#define CACHE_MUTEX_NAME		L"GitPromptMutex"

class CCacheService
{
private:
	const int BUFSIZE = 4096;

	static volatile LONG nThreadCount;
	static CCacheService* s_instance;



	CGitStatusList m_gitStatusList;
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

