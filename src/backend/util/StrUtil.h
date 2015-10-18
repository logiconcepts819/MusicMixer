#ifndef SRC_UTIL_STRUTIL_H_
#define SRC_UTIL_STRUTIL_H_

#include <string>
#include <cstdarg>

// See http://stackoverflow.com/questions/69738

namespace StrUtil
{
	std::string format(const char *fmt, ...);
	std::string vformat(const char *fmt, va_list ap);
}

#endif /* SRC_UTIL_STRUTIL_H_ */
