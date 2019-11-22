// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <iostream>
#include <sstream>
#include <string>

class Logger {
public:
    enum Level
    {
        LDEBUG,
        LVERBOSE,
        LINFO,
        LWARNING,
        LERROR
    };

public:
    Logger(const char *func, const char *file, int line, enum Level level = LINFO);
    ~Logger();

    inline std::ostream &stream() { return m_ss; }

private:
    static void align(std::stringstream &ss);
	static void init();

private:
    std::ostringstream m_ss;

private:
    static size_t align_width;
};

#define DEBUG_LOG Logger(__func__, __FILE__, __LINE__, Logger::LDEBUG).stream()
#define LOG Logger(__func__, __FILE__, __LINE__, Logger::LINFO).stream()
#define WARN_LOG Logger(__func__, __FILE__, __LINE__, Logger::LWARNING).stream()
#define ERR_LOG Logger(__func__, __FILE__, __LINE__, Logger::LERROR).stream()

