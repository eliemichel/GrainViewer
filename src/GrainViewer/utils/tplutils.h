#pragma once

// template utils (https://stackoverflow.com/questions/4484982/how-to-convert-typename-t-to-string-in-c)
template <typename T> struct TypeName { static const char* Get() { return "object"; } };
#define ENABLE_TYPENAME(A) template<> struct TypeName<A> { static const char *Get() { return #A; }};
ENABLE_TYPENAME(int)
ENABLE_TYPENAME(float)
ENABLE_TYPENAME(std::string)
//

