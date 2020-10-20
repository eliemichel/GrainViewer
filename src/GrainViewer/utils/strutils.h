/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

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
