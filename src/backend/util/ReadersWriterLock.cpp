#include "ReadersWriterLock.h"

ReadersWriterLock::ReadersWriterLock() : m_NumReaders(0), m_HaveWriter(false)
{
	// TODO Auto-generated constructor stub
}

ReadersWriterLock::~ReadersWriterLock() {
	// TODO Auto-generated destructor stub
}

void ReadersWriterLock::ReadLock()
{
	std::unique_lock<std::mutex> lck(m_Mutex);
	m_Unlocked.wait(lck, [this] { return !m_HaveWriter; });
	m_NumReaders++;
}

void ReadersWriterLock::ReadUnlock()
{
	std::unique_lock<std::mutex> lck(m_Mutex);
	m_NumReaders--;
	if (m_NumReaders == 0)
	{
		m_Unlocked.notify_all();
	}
}

void ReadersWriterLock::WriteLock()
{
	std::unique_lock<std::mutex> lck(m_Mutex);
	m_Unlocked.wait(lck,
			[this] { return !m_HaveWriter && m_NumReaders == 0; });
	m_HaveWriter = true;
}

void ReadersWriterLock::WriteUnlock()
{
	std::unique_lock<std::mutex> lck(m_Mutex);
	m_HaveWriter = false;
	m_Unlocked.notify_all();
}
