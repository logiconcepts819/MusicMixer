#ifndef SRC_DB_INTF_QUERYINTERFACE_H_
#define SRC_DB_INTF_QUERYINTERFACE_H_

#include "../SettingsDB.h"

class QueryInterface {
protected:
	SettingsDB * m_Db;
public:
	QueryInterface(SettingsDB & db);
	virtual ~QueryInterface();

	bool EnsureTableExists(const std::string & tableName,
			const std::string & columnSpec);
protected:
	bool TableExists(const std::string & tableName);
	bool CreateTable(const std::string & tableName,
			const std::string & columnSpec);
};

#endif /* SRC_DB_INTF_QUERYINTERFACE_H_ */
