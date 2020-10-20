// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "Logger.h"
#include "utils/fileutils.h"
#include <chrono>
#include <ctime>
#include <cstring>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

size_t Logger::align_width = 0;

bool EnableVTMode()
{
#ifdef _WIN32
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return false;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		return false;
	}
#endif
	return true;
}

namespace Color
{
#ifdef _WIN32
	static const std::string CSI = "\x1b[";
#else
	static const std::string CSI = "\033[";
#endif
    const std::string bold_grey      = CSI + "1;30m";
    const std::string bold_red       = CSI + "1;31m";
    const std::string bold_green     = CSI + "1;32m";
    const std::string bold_yellow    = CSI + "1;33m";
    const std::string bold_blue      = CSI + "1;34m";
    const std::string bold_purple    = CSI + "1;35m";
    const std::string bold_lightblue = CSI + "1;36m";
    const std::string bold_white     = CSI + "1;37m";

    const std::string black     = CSI + "0;30m";
    const std::string red       = CSI + "0;31m";
    const std::string green     = CSI + "0;32m";
    const std::string yellow    = CSI + "0;33m";
    const std::string blue      = CSI + "0;34m";
    const std::string purple    = CSI + "0;35m";
    const std::string lightblue = CSI + "0;36m";
    const std::string white     = CSI + "0;37m";

    const std::string nocolor   = CSI + "0m";
} // namespace Color

Logger::Logger(const char *func, const char *file, int line, Logger::Level level)
{
	if (Logger::align_width == 0) {
		Logger::init();
		Logger::align_width = 1;
	}

    {
        using namespace std::chrono;
        std::time_t now_c = system_clock::to_time_t(system_clock::now());
#ifdef _WIN32
		struct tm timeinfoData;
		struct tm * timeinfo = &timeinfoData;
		localtime_s(timeinfo, &now_c);
#else // _WIN32
		struct tm * timeinfo = std::localtime(&now_c);
#endif // _WIN32
		char buffer[26];
		strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
		m_ss << buffer << ": ";
    }

    switch(level)
    {
    case Logger::LDEBUG:
        m_ss << Color::bold_grey   << "DEBUG  " << Color::nocolor;
        break;
    case Logger::LVERBOSE:
        m_ss << Color::bold_white  << "VERBOSE" << Color::nocolor;
        break;
    case Logger::LINFO:
        m_ss << Color::bold_white  << "INFO   " << Color::nocolor;
        break;
    case Logger::LWARNING:
        m_ss << Color::bold_yellow << "WARNING" << Color::nocolor;
        break;
    case Logger::LERROR:
        m_ss << Color::bold_red    << "ERROR  " << Color::nocolor;
        break;
    }

    if( level != LINFO )
    {
        std::stringstream pos;
        pos << " (" << func << "():" << shortFileName(file) << ":" << line << ")";
        Logger::align(pos);
        m_ss << Color::blue << pos.str() << Color::nocolor;
    }

    m_ss << " ";

    switch( level) {
    case Logger::LDEBUG:   m_ss << Color::bold_grey;    break;
    case Logger::LVERBOSE: m_ss << Color::nocolor; break;
    case Logger::LINFO:    m_ss << Color::nocolor; break;
    case Logger::LWARNING: m_ss << Color::yellow;  break;
    case Logger::LERROR:   m_ss << Color::red;     break;
    }
}

Logger::~Logger()
{
    m_ss << Color::nocolor;
    std::string str = m_ss.str();
    // trim extra newlines
    while ( str.empty() == false && str[str.length() - 1] == '\n')
        str.resize(str.length() - 1);
    std::cerr << str << std::endl;
}

void Logger::align(std::stringstream &ss)
{
	size_t l = ss.str().length();
    Logger::align_width = ( Logger::align_width > l ) ? Logger::align_width : 1;
    ss << std::setw(Logger::align_width - l) << "";
}

void Logger::init() {
	if (!EnableVTMode()) {
		std::cerr << "Warning: Logger could not enable Virtual Terminal mode." << std::endl;
	}
}
