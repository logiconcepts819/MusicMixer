#ifndef SRC_DB_ENTRY_FILEQUERYENTRY_H_
#define SRC_DB_ENTRY_FILEQUERYENTRY_H_

#include "QueryEntry.h"
#include <string>

class FileQueryEntry : public QueryEntry {
	std::string m_Filename;
public:
	FileQueryEntry();
	virtual ~FileQueryEntry();

	std::string getFilename() const { return m_Filename; }
	void setFilename(const std::string & filename) { m_Filename = filename; }

};

#endif /* SRC_DB_ENTRY_FILEQUERYENTRY_H_ */
