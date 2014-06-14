#pragma once

class CCacheService
{
private:
	const wstring CACHE_NAME = L"\\\\.\\pipe\\GitPrompt";
	const wstring CACHE_MUTEX_NAME = L"GitPromptMutex";
	const int BUFSIZE = 4096;

	wstring CCacheService::GetCachePipeName();
	wstring CCacheService::GetCacheCommandPipeName();
	wstring CCacheService::GetCacheMutexName();
	wstring CCacheService::GetCacheID();
	void CCacheService::PipeThread();

public:
	CCacheService();
	~CCacheService();

	void Start();

	void Stop();
};

