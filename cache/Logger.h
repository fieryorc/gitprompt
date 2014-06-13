#pragma once

#include <string>
#include <cvt/wstring>

using namespace std;

class Logger
{
private:
	static void WriteColorEx(int backColor, int foreColor, bool newLine, wstring text);
	static void WriteColor(int foreColor, wstring text);

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
	static void LogError(wstring msg);
	static void LogWarning(wstring msg);
	static void LogMessage(wstring msg);
	Logger();
	~Logger();
};

