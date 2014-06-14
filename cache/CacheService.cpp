#include "stdafx.h"
#include "CacheService.h"
#include "Utils\SmartHandle.h"
#include <memory>
#include "DebugLogger.h"

CCacheService::CCacheService()
{
}


CCacheService::~CCacheService()
{
}

wstring CCacheService::GetCachePipeName()
{
	return CACHE_NAME + GetCacheID();
}

wstring CCacheService::GetCacheMutexName()
{
	return CACHE_MUTEX_NAME + GetCacheID();
}

wstring CCacheService::GetCacheID()
{
	CString t;
	CAutoGeneralHandle token;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.GetPointer());
	if (result)
	{
		DWORD len = 0;
		GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
		if (len >= sizeof(TOKEN_STATISTICS))
		{
			std::unique_ptr<BYTE[]> data(new BYTE[len]);
			GetTokenInformation(token, TokenStatistics, data.get(), len, &len);
			LUID uid = ((PTOKEN_STATISTICS)data.get())->AuthenticationId;
			t.Format(_T("-%08x%08x"), uid.HighPart, uid.LowPart);
		}
	}
	return t;
}



void CCacheService::Start()
{

}

void CCacheService::Stop()
{

}