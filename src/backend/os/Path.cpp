#include "Path.h"
#include "../program/ProgramInfo.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stack>

#ifdef WIN32
#include <malloc.h>
#include <windows.h>
#include <direct.h>
#include <cctype>
#define mkdir(d,p) _mkdir(d)
#define stat _stat
#define S_ISDIR(m) ((m) & _S_IFDIR != 0)
#define SEPS "/\\"
#else
#include <unistd.h>
static const std::string m_XdgConfigName = "CONFIG";
static const std::string m_XdgConfigDir = ".config";
#define SEPS "/"
#endif

std::string Path::GetSeparator()
{
#ifdef WIN32
	return "\\";
#else
	return "/";
#endif
}

std::string Path::GetApplicationPath()
{
	std::string thePath;

#ifdef WIN32
	wchar_t wdir[MAX_PATH];
	if (SHGetFolderPathW(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
			nullptr, SHGFP_TYPE_CURRENT, wdir) == S_OK)
	{
		thePath = FromWide(wdir);
	}
#else
	const char * path;
	if ((path = getenv(("XDG_" + m_XdgConfigName + "_HOME").c_str())))
	{
		thePath = path;
	}
	else if ((path = getenv("HOME")))
	{
		thePath = path + GetSeparator() + m_XdgConfigDir;
	}
#endif

	if (!thePath.empty())
	{
		thePath += GetSeparator() + ProgramInfo::GetProgramName();
	}
	return thePath;
}

std::string Path::GetSimplifiedPath(const std::string & path,
		bool keepLeadingDot, bool keepTrailingSlash)
{
	// In this function, we need to take extreme caution because there are a
	// LOT of corner cases to deal with.

	std::stack<size_t> slashPosStack;
	std::string output = path;
	size_t refPos = 0;
	size_t currPos;
	bool done = false;

	while (!done)
	{
		currPos = output.find_first_of(SEPS, refPos);
		if (currPos == std::string::npos)
		{
			currPos = output.size();
			done = true;
		}

		// Found slash at some position other than beginning of path string
		std::string dirSegment = output.substr(refPos, currPos - refPos);
		if (dirSegment.empty() || dirSegment == ".")
		{
			// Found blank path or unneeded dot.
			if (done)
			{
				// In this case, we finished processing our path string (i.e.,
				// the current position corresponds to the end of the string).
				if (refPos != 0)
				{
					// We're at the end of the string, and the reference
					// position corresponds to a slash (i.e., output has value
					// "<stuff>/" or "<stuff>/.").  In this case, we keep
					// our slash if the caller requested it.  In any case,
					// we're getting rid of the trailing period.
					if (keepTrailingSlash)
					{
						refPos++;
					}
					output =
						output.substr(0, refPos - 1) + output.substr(currPos);
				}
				// If we're at the end of the string and the reference position
				// corresponds to the start of the string (i.e., if output has
				// value "" or "."), we keep whatever we have so far.  We may
				// end up with a period returned, even if keepLeadingDot is set
				// to false.
			}
			else if (refPos == 0)
			{
				// In this case, the reference position corresponds to the
				// start of the string (i.e., the output has value "/<stuff>"
				// or "./<stuff>").  In this case, we check whether the
				// segment has value "" or ".".  If the segment
				// corresponds has value "." and we don't want to keep the
				// leading period, then we simplify our string so that it
				// becomes "<stuff>".  Otherwise, if the segment corresponds to
				// "", then we simply do nothing.
				if (dirSegment == "." && !keepLeadingDot)
				{
					if (currPos + 1 == output.size())
					{
						// In the special case that we have no stuff after the
						// slash, we must use whatever segment we extracted.
						output = dirSegment;
					}
					else
					{
						output = output.substr(currPos + 1);
					}
				}
				else
				{
					slashPosStack.push(refPos);
					refPos = currPos + 1;
				}
			}
			else
			{
				// In this case, the reference position corresponds to a slash
				// (i.e., output has value "<stuff>//<more stuff>" or
				// "<stuff>/./<more stuff>".  Therefore, we simplify our string
				// so that it reads "<stuff>/<more stuff>".
				output =
					output.substr(0, refPos - 1) + output.substr(currPos);
			}
		}
		else if (refPos != 0 && dirSegment == "..")
		{
			std::string prevSegment = output.substr(
					slashPosStack.top(),
					refPos - slashPosStack.top() - 1);
			if (prevSegment != "." && prevSegment != "..")
			{
				// Found <prevSegment>/.., where prevSegment is neither
				// "." nor "..".
				size_t oldRefPos = refPos;
				refPos = slashPosStack.top();
				if (refPos == 0)
				{
					if (oldRefPos == 1				  // slash is at position 1
#ifdef WIN32
						|| (
								oldRefPos == 3 &&	  // slash is at position 3
								isalpha(output[0]) && // drive letter
								output[1] == ':'      // colon after letter
							)
#endif
						)
					{
						// Need to make sure absolute paths are accounted for.
						// If we are in Windows, we need to detect any drive
						// letter specification at the beginning of our string,
						// which indicates an absolute path.  In this case, we
						// don't pop anything off the stack, since we need a
						// reference point at the start of the string for
						// future iterations

						// Set reference position back to what it was before
						refPos = oldRefPos;
					}
					else
					{
						// Using a relative path, so we can effectively
						// "restart" our algorithm
						slashPosStack.pop();
					}
				}
				else
				{
					// We know for a fact that the segment is not near the
					// start of our string, so we just simplify our string
					slashPosStack.pop();
				}

				// Simplify the string as appropriate
				output = output.substr(0, refPos) + output.substr(currPos + 1);
			}
			else
			{
				// Found <prevSegment>/.., where prevSegment is either
				// "." or "..".  We must keep our string the same in
				// this case.
				slashPosStack.push(refPos);
				refPos = currPos + 1;
			}
		}
		else
		{
			// Found segment that is ".." at the beginning, or else neither "",
			// ".", nor "..".  We must keep our string the same in all these
			// cases.
			slashPosStack.push(refPos);
			refPos = currPos + 1;
		}
	}

	return output;
}

bool Path::MakeDirectory(const std::string & path, bool simplifyPath)
{
	struct stat st;
	bool rv = true;
	size_t refPos = 0;
	size_t currPos;

	std::string pth = path;
	if (simplifyPath)
	{
		pth = GetSimplifiedPath(pth);
	}

	while (rv &&
			(currPos = pth.find_first_of(SEPS, refPos)) != std::string::npos)
	{
		if (currPos != refPos)
		{
			const std::string truncPath = pth.substr(0, currPos).c_str();
			if (stat(truncPath.c_str(), &st) != 0)
			{
				rv = mkdir(pth.substr(0, currPos).c_str(), 0755) == 0;
			}
			else if (!S_ISDIR(st.st_mode))
			{
				rv = false;
			}
		}
		refPos = currPos + 1;
	}
	if (rv && refPos != pth.size())
	{
		if (stat(pth.c_str(), &st) != 0)
		{
			rv = mkdir(pth.substr(0, currPos).c_str(), 0755) == 0;
		}
		else if (!S_ISDIR(st.st_mode))
		{
			rv = false;
		}
	}
	return rv;
}

bool Path::MakeApplicationPath()
{
	return MakeDirectory(GetApplicationPath(), false);
}

std::string Path::GetBaseName(const std::string & path)
{
	size_t slashPos = 0;
	size_t length = path.size();
	while (length > 0 && (slashPos = path.find_last_of(SEPS, length - 1)) == length - 1)
	{
		--length;
	}
	return length > 0 ? path.substr(slashPos + 1, length - (slashPos + 1)) : "";
}
