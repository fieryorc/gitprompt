#include "stdafx.h"
#include "ConsoleLogger.h"

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
	WriteColor(0, FGCOLOR_FG_RED, true, L"Error: " + msg);
}

void Logger::LogWarning(const wstring& msg)
{
	WriteColor(0, FGCOLOR_FG_YELLOW, true, L"Warning: " + msg);
}

void Logger::LogInfo(const wstring& msg)
{
	WriteColor(0, 0, true, L"Info: " + msg);
}

void Logger::WriteColor(int backColor, int foreColor, bool newLine, wstring text)
{
	bool hasColor = backColor != 0 || foreColor != 0;
	HANDLE hConsole = NULL;
	CONSOLE_SCREEN_BUFFER_INFO buffer;

	if (hasColor)
	{
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(hConsole, &buffer);

		// Set new colors.
		SetConsoleTextAttribute(hConsole, backColor << 4 | foreColor);
	}

	wprintf(_T("%s"), text.c_str());
	if (newLine)
		wprintf(_T("\n"));

	// Reset console colors.
	if (hasColor)
		SetConsoleTextAttribute(hConsole, buffer.wAttributes);
}
