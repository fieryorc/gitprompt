#pragma once

#include <string>
#include <cvt/wstring>

using namespace std;

class Logger
{
private:

public:
	static void LogError(wstring msg);
	static void LogWarning(wstring msg);
	static void LogMessage(wstring msg);
	Logger();
	~Logger();
};

