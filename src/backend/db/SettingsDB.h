#ifndef SRC_DB_SETTINGSDB_H_
#define SRC_DB_SETTINGSDB_H_

#include <sqlite3.h>
#include <string>
#include <map>
#include <functional>

class SettingsDB {
public:
	// We use a map to represent a row of cells.  The disadvantage of this
	// approach, though, is that we can no longer tell how columns are ordered.
	typedef std::map<std::string, std::string> RowType; // (colname, text)
	typedef RowType::value_type RowItemType;

private:
	sqlite3 * m_Db;
	std::string m_PreviousQueryError;

	static const std::string & m_DbFilename;

	bool m_IsOpen;
public:
	SettingsDB();
	virtual ~SettingsDB();

	static std::string GetDBFilename();
	bool Open();
	void Close();

	bool Query(const std::string & statement);

	template <class QueryCBFunc>
	bool Query(const std::string & statement, QueryCBFunc cbFunc);

	bool isOpen() const { return m_IsOpen; }

	std::string GetPreviousQueryError() const { return m_PreviousQueryError; }
	static std::string FmtStr(const std::string & theStr);
};

// Format of QueryCBFunc:  bool cb(const RowType & row)
template <class QueryCBFunc>
bool SettingsDB::Query(const std::string & statement, QueryCBFunc cbFunc)
{
	bool rv = false;
	if (m_IsOpen)
	{
		char * zErrMsg = nullptr;
		rv = sqlite3_exec(m_Db, statement.c_str(),
			[] (void *param, int argc, char **argv, char **azColName) -> int {
				RowType row;
				for (int k = 0; k < argc; ++k)
				{
					row.insert(RowItemType(azColName[k], argv[k]));
				}
				return (*(QueryCBFunc *) param)(row) ? 0 : 1;
			}, &cbFunc, &zErrMsg) == SQLITE_OK;
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

#endif /* SRC_DB_SETTINGSDB_H_ */
