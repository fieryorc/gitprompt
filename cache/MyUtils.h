#pragma once

#include "stdafx.h"
#include "DebugLogger.h"

class MyUtils
{
public:
	static const wstring NormalizePath(const wstring &path)
	{
		TCHAR normalizedPath[MAX_PATH + 1];
		if (GetFullPathName(path.c_str(), MAX_PATH, normalizedPath, NULL) <= 0)
		{
			Logger::LogError(L"Unable to convert the path using GetFullPathName : " + path);
			return normalizedPath;
		}
		wstring result(normalizedPath);
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}
};