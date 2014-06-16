
#include "..\stdafx.h"

#pragma once

class CMutex
{
private:
	HANDLE m_mutex;
	bool m_isLocked;

	void Lock()
	{
		WaitForSingleObject(this->m_mutex, INFINITE);
	}

	void Unlock()
	{
		if (this->m_isLocked)
		{
			this->m_isLocked = false;
			ReleaseMutex(this->m_mutex);
		}
	}

public:
	CMutex()
	{
		this->m_mutex = CreateMutex(NULL, FALSE, NULL);
	}

	~CMutex()
	{
		CloseHandle(this->m_mutex);
	}

	friend class CMutexLock;
};

class CMutexLock
{
private:
	CMutex* m_mutexObj;
public:

	CMutexLock(CMutex* mutex)
	{
		this->m_mutexObj = mutex;
		this->m_mutexObj->Lock();
	}

	~CMutexLock()
	{
		this->m_mutexObj->Unlock();
	}
};
