#ifndef SRC_UTIL_READERSWRITERLOCK_H_
#define SRC_UTIL_READERSWRITERLOCK_H_

#include <mutex>
#include <condition_variable>

// See http://stackoverflow.com/questions/12033188

class ReadersWriterLock
{
	std::mutex m_Mutex;
	std::condition_variable m_Unlocked;

	int m_NumReaders;
	bool m_HaveWriter;

public:
	ReadersWriterLock();
	virtual ~ReadersWriterLock();

	void ReadLock();
	void ReadUnlock();

	void WriteLock();
	void WriteUnlock();
};

// RAII guards

class RWLockReadGuard
{
	ReadersWriterLock * m_RWLock;
public:
	RWLockReadGuard(ReadersWriterLock & rwlock) : m_RWLock(&rwlock)
	{
		m_RWLock->ReadLock();
	}

	virtual ~RWLockReadGuard()
	{
		m_RWLock->ReadUnlock();
	}
};

class RWLockWriteGuard
{
	ReadersWriterLock * m_RWLock;
public:
	RWLockWriteGuard(ReadersWriterLock & rwlock) : m_RWLock(&rwlock)
	{
		m_RWLock->WriteLock();
	}

	virtual ~RWLockWriteGuard()
	{
		m_RWLock->WriteUnlock();
	}
};

#endif /* SRC_UTIL_READERSWRITERLOCK_H_ */
