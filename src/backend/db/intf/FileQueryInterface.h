#ifndef SRC_DB_INTF_FILEQUERYINTERFACE_H_
#define SRC_DB_INTF_FILEQUERYINTERFACE_H_

#include "QueryInterface.h"

class FileQueryInterface : public QueryInterface {
	static const std::string m_TableName;
	static const std::string m_Column1Name;
	static const std::string m_ColumnSpec;

	bool TableExists();
	bool CreateTable();
public:
	FileQueryInterface(SettingsDB & db);
	virtual ~FileQueryInterface();

	bool EnsureTableExists();
	bool UpdateDbWithFile(const std::string & filename);
};

#endif /* SRC_DB_INTF_FILEQUERYINTERFACE_H_ */
