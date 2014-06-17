#pragma once
#include "git2.h"

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
	// State specifying any operation in progress one of git_status_t
	DWORD state;

	DWORD n_addedIndex;
	DWORD n_modifiedIndex;
	DWORD n_deletedIndex;

	DWORD n_addedWorkDir;
	DWORD n_modifiedWorkDir;
	DWORD n_deletedWorkDir;

	WCHAR branch[MAX_PATH + 1];
} CacheServiceResponse;


