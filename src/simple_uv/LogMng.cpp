#include "LogMng.h"
#include "log4z.h"

CLogMng* CLogMng::m_pMng = nullptr;
CUVMutex CLogMng::m_Mutex;

CLogMng* CLogMng::GetInstance()
{
	if (!m_pMng)
	{
		m_Mutex.Lock();

		if (!m_pMng)
		{
			m_pMng = new CLogMng;
		}

		m_Mutex.UnLock();
	}

	return m_pMng;
}

CLogMng::CLogMng(void)
{
	 zsummer::log4z::ILog4zManager::getInstance()->setLoggerMonthdir(LOG4Z_MAIN_LOGGER_ID, true);
    zsummer::log4z::ILog4zManager::getInstance()->setLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, false);
    zsummer::log4z::ILog4zManager::getInstance()->setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_DEBUG);
    zsummer::log4z::ILog4zManager::getInstance()->setLoggerLimitsize(LOG4Z_MAIN_LOGGER_ID, 100);
    /*if (logpath) {
        zsummer::log4z::ILog4zManager::getInstance()->setLoggerPath(LOG4Z_MAIN_LOGGER_ID, logpath);
    }*/
    zsummer::log4z::ILog4zManager::getInstance()->start();
}


CLogMng::~CLogMng(void)
{
}

void CLogMng::StopLog()
{
	zsummer::log4z::ILog4zManager::getInstance()->stop();
}

