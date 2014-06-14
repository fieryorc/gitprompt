#pragma once

#include <string>
#include <cvt/wstring>

using namespace std;

class Logger
{
private:
	static void WriteColor(int backColor, int foreColor, bool newLine, wstring text);

	enum {
		FGCOLOR_FG_DARKBLUE = 1,
		FGCOLOR_FG_DARKGREEN,
		FGCOLOR_FG_DARKCYAN,
		FGCOLOR_FG_DARKRED,
		FGCOLOR_FG_DARKMEGENTA,
		FGCOLOR_FG_DARKYELLOW,
		FGCOLOR_FG_GRAY,
		FGCOLOR_FG_DARKGRAY,
		FGCOLOR_FG_BLUE,
		FGCOLOR_FG_GREEN,
		FGCOLOR_FG_CYAN,
		FGCOLOR_FG_RED,
		FGCOLOR_FG_MEGENTA,
		FGCOLOR_FG_YELLOW,
		FGCOLOR_FG_WHITE
	};

public:
	static void LogError(const wstring& msg);
	static void LogWarning(const wstring& msg);
	static void LogInfo(const wstring& msg);
	Logger();
	~Logger();
};

