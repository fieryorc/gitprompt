#include "stdafx.h"
#include "Logger.h"

#include <cvt\wstring>
using namespace std;


Logger::Logger()
{
}


Logger::~Logger()
{
		
}

void Logger::LogError(wstring msg)
{
	WriteColorEx(0, FGCOLOR_FG_RED, true, msg);
}

void Logger::LogWarning(wstring msg)
{
	WriteColorEx(0, FGCOLOR_FG_YELLOW, true, msg);
}

void Logger::LogMessage(wstring msg)
{
	WriteColorEx(0, 0, true, msg);
}

void Logger::WriteColorEx(int backColor, int foreColor, bool newLine, wstring text)
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

void Logger::WriteColor(int foreColor, wstring text)
{
	WriteColorEx(0, foreColor, false, text);
}



