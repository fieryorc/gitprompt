#pragma once
#include "git2.h"
#include "DirWatcher.h"

/**
 * Request structure.
 */
typedef struct type_CacheServiceRequest
{
	DWORD request;
	WCHAR path[MAX_PATH + 1];
} CacheServiceRequest;

// CustomActions will use this header but does not need nor understand the SVN types ...

/**
 * The structure returned as a response
 */
typedef struct type_CacheServiceResponse
{
	DWORD isSuccess;
	// State specifying any operation in progress
	DWORD state;
	DWORD repoStatus;
	DWORD n_added;
	DWORD n_modified;
	DWORD n_deleted;
	WCHAR branch[MAX_PATH + 1];
} CacheServiceResponse;


