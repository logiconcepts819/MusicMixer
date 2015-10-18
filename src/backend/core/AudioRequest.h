#ifndef SRC_CORE_AUDIOREQUEST_H_
#define SRC_CORE_AUDIOREQUEST_H_

#include <string>

class AudioRequest {
	std::string m_FileName;
public:
	AudioRequest(const std::string & fileName);
	virtual ~AudioRequest();

	std::string getFilename() const { return m_FileName; }
	void setFilename(const std::string & fileName) { m_FileName = fileName; }
};

#endif /* SRC_CORE_AUDIOREQUEST_H_ */
