#include "FileQueryInterface.h"
#include <cassert>

const std::string FileQueryInterface::m_TableName = "files";
const std::string FileQueryInterface::m_Column1Name = "filename";
const std::string FileQueryInterface::m_ColumnSpec =
		FileQueryInterface::m_Column1Name + " varchar(255)";

FileQueryInterface::FileQueryInterface(SettingsDB & db)
: QueryInterface(db) {
	// TODO Auto-generated constructor stub
}

FileQueryInterface::~FileQueryInterface() {
	// TODO Auto-generated destructor stub
}

bool FileQueryInterface::TableExists()
{
	return QueryInterface::TableExists(m_TableName);
}

bool FileQueryInterface::CreateTable()
{
	return QueryInterface::CreateTable(m_TableName, m_ColumnSpec);
}

bool FileQueryInterface::EnsureTableExists()
{
	return QueryInterface::EnsureTableExists(m_TableName, m_ColumnSpec);
}

bool FileQueryInterface::UpdateDbWithFile(const std::string & filename)
{
	bool exist = false;
	bool rv = false;
	assert(m_Db->Query("SELECT EXISTS (SELECT * FROM " + m_TableName
			+ " WHERE " + m_Column1Name + "='" +
			SettingsDB::FmtStr(filename) + "')",
		[&exist] (const SettingsDB::RowType & row)
		{
			assert(row.size() == 1);
			exist = row.begin()->second != "0";
			return true;
		}));
	if (!exist)
	{
		rv = m_Db->Query("INSERT INTO " + m_TableName + " (" + m_Column1Name +
				") VALUES ('" + SettingsDB::FmtStr(filename) + "')");
	}
	return rv;
}
