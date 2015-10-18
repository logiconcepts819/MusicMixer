#ifndef SRC_UTIL_CPP14MEMORY_H_
#define SRC_UTIL_CPP14MEMORY_H_

// Once C++14 becomes a widely acceptable standard, we will no longer
// need this file in this project, which means all instances of
// #include "util/CPP14Memory.h" will be replaced with #include <memory>.

#include <memory>

#if __cplusplus < 201402L
namespace std
{
	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args)
	{
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
}
#endif

#endif /* SRC_UTIL_CPP14MEMORY_H_ */
