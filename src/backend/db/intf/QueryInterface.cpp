#include "QueryInterface.h"
#include <cassert>

QueryInterface::QueryInterface(SettingsDB & db) : m_Db(&db) {
	// TODO Auto-generated constructor stub
}

QueryInterface::~QueryInterface() {
	// TODO Auto-generated destructor stub
}

bool QueryInterface::TableExists(const std::string & tableName)
{
	bool exist = false;
	assert(m_Db->Query("SELECT COUNT(*) FROM sqlite_master "
	         "WHERE type = 'table' AND name = '" +
			 SettingsDB::FmtStr(tableName) + '\'',

		[&exist] (const SettingsDB::RowType & row)
		{
			assert(row.size() == 1);
			exist = row.begin()->second != "0";
			return true;
		}));
	return exist;
}

bool QueryInterface::CreateTable(const std::string & tableName,
		const std::string & columnSpec)
{
	return m_Db->Query(
			"CREATE TABLE " + tableName + " (" + columnSpec + ')');
}

bool QueryInterface::EnsureTableExists(const std::string & tableName,
		const std::string & columnSpec)
{
	bool rv = true;
	if (!TableExists(tableName))
	{
		rv = CreateTable(tableName, columnSpec);
	}
	return rv;
}
