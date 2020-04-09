// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <sstream>

/**
 * Trim whitespaces string from start (in place)
 */
void ltrim(std::string& s);

/**
 * Trim whitespaces string from end (in place)
 */
void rtrim(std::string& s);

/**
 * Trim whitespaces string in place
 */
void trim(std::string& s);

/**
 * Convert string to lower
 */
std::string toLower(const std::string & s);

bool startsWith(const std::string & s, const std::string & postfix);

bool endsWith(const std::string & s, const std::string & postfix);

std::string replaceAll(std::string str, const std::string& search, const std::string& replace);

std::string bitname(int flags, int flagCount);

// from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size <= 0) { throw std::runtime_error("Error during formatting."); }
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

#define MAKE_STR(contents) (std::ostringstream() << contents).str()
