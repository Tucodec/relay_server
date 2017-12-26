#ifndef LOG_MNG_H_234234
#define LOG_MNG_H_234234
#include "SimpleUVExport.h"
#include "SimpleLocks.h"
#include "log4z.h"

class CLogMng
{
public:
	static SUV_EXPORT CLogMng* GetInstance();
	CLogMng(void);
	~CLogMng(void);

	SUV_EXPORT void StopLog();

	static CLogMng* m_pMng;
	static CUVMutex      m_Mutex;
};


#endif