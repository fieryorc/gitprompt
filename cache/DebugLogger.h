#pragma once

#include <string>
#include <cvt/wstring>

using namespace std;

class Logger
{
private:
	static void Log(const wstring& msg);
public:
	static void LogError(const wstring& msg);
	static void LogWarning(const wstring& msg);
	static void LogInfo(const wstring& msg);
	Logger();
	~Logger();
};

