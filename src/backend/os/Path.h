#ifndef SRC_OS_PATH_H_
#define SRC_OS_PATH_H_

#include <string>

namespace Path
{
	std::string GetSeparator();
	std::string GetApplicationPath();
	std::string GetSimplifiedPath(const std::string & path,
			bool keepLeadingDot = false, bool keepTrailingSlash = false);
	bool MakeDirectory(const std::string & path, bool simplifyPath = false);
	bool MakeApplicationPath();
	std::string GetBaseName(const std::string & path);
}

#endif /* SRC_OS_PATH_H_ */
