#include "SettingsDB.h"
#include "../os/Path.h"

const std::string & SettingsDB::m_DbFilename = "settings.db";

SettingsDB::SettingsDB() : m_Db(nullptr), m_IsOpen(false)
{
}

SettingsDB::~SettingsDB()
{
	Close();
}

std::string SettingsDB::GetDBFilename()
{
	return Path::GetApplicationPath() + Path::GetSeparator() + m_DbFilename;
}

bool SettingsDB::Open()
{
	bool rv = true;
	if (!m_IsOpen)
	{
		if (Path::MakeApplicationPath())
		{
			m_IsOpen = rv =
					sqlite3_open(GetDBFilename().c_str(), &m_Db) == SQLITE_OK;
		}
		else
		{
			rv = false;
		}
	}
	return rv;
}

void SettingsDB::Close()
{
	if (m_IsOpen)
	{
		sqlite3_close(m_Db);
		m_IsOpen = false;
	}
}

bool SettingsDB::Query(const std::string & statement)
{
	bool rv = false;
	if (m_IsOpen)
	{
		char * zErrMsg = nullptr;
		rv = sqlite3_exec(m_Db, statement.c_str(), nullptr,
				nullptr, &zErrMsg) == SQLITE_OK;
		m_PreviousQueryError.clear();
		if (zErrMsg)
		{
			m_PreviousQueryError = zErrMsg;
			sqlite3_free(zErrMsg);
		}
	}
	else
	{
		m_PreviousQueryError = "Settings database is not open";
	}
	return rv;
}

std::string SettingsDB::FmtStr(const std::string & theStr)
{
	std::string output = theStr;

	size_t addSize = 0;
	size_t refPos = 0;
	size_t curPos;

	while ((curPos = theStr.find('\'', refPos)) != std::string::npos)
	{
		size_t split = curPos + addSize++;
		output = output.substr(0, split) + '\'' + output.substr(split);
		refPos = curPos + 1;
	}

	return output;
}
