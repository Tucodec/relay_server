#ifndef SIMEPLE_LOCKS_H_43589435
#define SIMEPLE_LOCKS_H_43589435

#include <string>
#include "uv.h"
#include "SimpleUVExport.h"

// 互斥量，配合CUVAutoLock使用更方便
class SUV_EXPORT CUVMutex
{
public:
	CUVMutex()
	{
		uv_mutex_init(&mut_);
	}
	~CUVMutex(void)
	{
		uv_mutex_destroy(&mut_);
	}
	void Lock()
	{
		uv_mutex_lock(&mut_);
	}
	void UnLock()
	{
		uv_mutex_unlock(&mut_);
	}
	bool TryLock()
	{
		return uv_mutex_trylock(&mut_) == 0;
	}
private:
	uv_mutex_t mut_;
	friend class CUVCond;
	friend class CUVAutoLock;
private:  // private中，禁止复制和赋值
	CUVMutex(const CUVMutex&);             // 不实现
	CUVMutex& operator =(const CUVMutex&); // 不实现
};

class SUV_EXPORT CUVAutoLock
{
public:
	CUVAutoLock(uv_mutex_t* mut): mut_(mut)
	{
		uv_mutex_lock(mut_);
	}
	CUVAutoLock(CUVMutex* mut): mut_(&mut->mut_)
	{
		uv_mutex_lock(mut_);
	}
	~CUVAutoLock(void)
	{
		uv_mutex_unlock(mut_);
	}
private:
	uv_mutex_t* mut_;
private:                                   // private中，禁止复制和赋值
	CUVAutoLock(const CUVAutoLock&);       // 不实现
	CUVAutoLock& operator =(const CUVAutoLock&);  // 不实现
};

//条件变量
class SUV_EXPORT CUVCond
{
public:
	CUVCond()
	{
		uv_cond_init(&cond_);
	}
	~CUVCond(void)
	{
		uv_cond_destroy(&cond_);
	}
	void Signal()
	{
		uv_cond_signal(&cond_);
	}
	void BroadCast()
	{
		uv_cond_broadcast(&cond_);
	}
	void Wait(CUVMutex* mutex)
	{
		uv_cond_wait(&cond_, &mutex->mut_);
	}
	void Wait(uv_mutex_t* mutex)
	{
		uv_cond_wait(&cond_, mutex);
	}
	int Wait(CUVMutex* mutex, uint64_t timeout)
	{
		return uv_cond_timedwait(&cond_, &mutex->mut_, timeout);
	}
	int Wait(uv_mutex_t* mutex, uint64_t timeout)
	{
		return uv_cond_timedwait(&cond_, mutex, timeout);
	}
private:
	uv_cond_t cond_;
private:                                   // private中，禁止复制和赋值
	CUVCond(const CUVCond&);               // 不实现
	CUVCond& operator =(const CUVCond&);   // 不实现
};

// 信号量
class SUV_EXPORT CUVSem
{
public:
	explicit CUVSem(int initValue = 0)
	{
		uv_sem_init(&sem_, initValue);
	}
	~CUVSem(void)
	{
		uv_sem_destroy(&sem_);
	}
	void Post()
	{
		uv_sem_post(&sem_);
	}
	void Wait()
	{
		uv_sem_wait(&sem_);
	}
	bool TryWait()
	{
		return uv_sem_trywait(&sem_) == 0;
	}
private:
	uv_sem_t sem_;
private:                               // private中，禁止复制和赋值
	CUVSem(const CUVSem&);             // 不实现
	CUVSem& operator =(const CUVSem&); // 不实现
};

// 读写锁
class SUV_EXPORT CUVRWLock
{
public:
	CUVRWLock()
	{
		uv_rwlock_init(&rwlock_);
	}
	~CUVRWLock(void)
	{
		uv_rwlock_destroy(&rwlock_);
	}
	void ReadLock()
	{
		uv_rwlock_rdlock(&rwlock_);
	}
	void ReadUnLock()
	{
		uv_rwlock_rdunlock(&rwlock_);
	}
	bool ReadTryLock()
	{
		return uv_rwlock_tryrdlock(&rwlock_) == 0;
	}
	void WriteLock()
	{
		uv_rwlock_wrlock(&rwlock_);
	}
	void WriteUnLock()
	{
		uv_rwlock_wrunlock(&rwlock_);
	}
	bool WriteTryLock()
	{
		return uv_rwlock_trywrlock(&rwlock_) == 0;
	}
private:
	uv_rwlock_t rwlock_;
private://private中，禁止复制和赋值
	CUVRWLock(const CUVRWLock&);//不实现
	CUVRWLock& operator =(const CUVRWLock&);//不实现
};


class SUV_EXPORT CUVBarrier
{
public:
	CUVBarrier(int count)
	{
		uv_barrier_init(&barrier_, count);
	}
	~CUVBarrier(void)
	{
		uv_barrier_destroy(&barrier_);
	}
	void Wait()
	{
		uv_barrier_wait(&barrier_);
	}
private:
	uv_barrier_t barrier_;
private://private中，禁止复制和赋值
	CUVBarrier(const CUVBarrier&);//不实现
	CUVBarrier& operator =(const CUVBarrier&);//不实现
};

#endif