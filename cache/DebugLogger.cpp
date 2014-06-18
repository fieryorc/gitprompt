#include "stdafx.h"
#include "DebugLogger.h"

#include <cvt\wstring>
using namespace std;


Logger::Logger()
{
}


Logger::~Logger()
{
		
}

void Logger::LogError(const wstring& msg)
{
	Log(L"Error: " + msg);
}

void Logger::LogWarning(const wstring& msg)
{
	Log(L"Warning: " + msg);
}

void Logger::LogInfo(const wstring& msg)
{
	Log(L"Info: " + msg);
}

void Logger::Log(const wstring& msg)
{
	wstring out = msg + L"\n";
	OutputDebugString(out.c_str());
}
