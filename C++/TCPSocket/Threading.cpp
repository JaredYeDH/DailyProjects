#include "pch.h"

#include <exception>

#include <poll.h>

#include "Threading.h"
#include "Utils.h"

Mutex::Mutex() {
    P_ENSURE_R(::pthread_mutex_init(&mMutex, nullptr));
}
Mutex::~Mutex() {
    P_ENSURE_R(::pthread_mutex_destroy(&mMutex));
}
void Mutex::lock() {
    P_ENSURE_R(::pthread_mutex_lock(&mMutex));
}
void Mutex::unlock() {
    P_ENSURE_R(::pthread_mutex_unlock(&mMutex));
}

CondVar::CondVar() {
    P_ENSURE_R(::pthread_cond_init(&mCond, nullptr));
}
CondVar::~CondVar() {
    P_ENSURE_R(::pthread_cond_destroy(&mCond));
}
void CondVar::signal() {
    P_ENSURE_R(::pthread_cond_signal(&mCond));
}
void CondVar::broadcast() {
    P_ENSURE_R(::pthread_cond_broadcast(&mCond));
}
void CondVar::wait(Mutex *mutex) {
    P_ENSURE_R(::pthread_cond_wait(&mCond, mutex->getInternal()));
}

Semaphore::Semaphore(int value) {
    P_ENSURE(::sem_init(&mSem, 0, value) == 0);
}
Semaphore::~Semaphore() {
    P_ENSURE(::sem_destroy(&mSem) == 0);
}
void Semaphore::wait() {
    P_ENSURE(::sem_wait(&mSem) == 0);
}
void Semaphore::post() {
    P_ENSURE(::sem_post(&mSem) == 0);
}

RWLock::RWLock() {
    P_ENSURE_R(::pthread_rwlock_init(&mRWLock, nullptr));
}
RWLock::~RWLock() {
    P_ENSURE_R(::pthread_rwlock_destroy(&mRWLock));
}
void RWLock::rlock() {
    P_ENSURE_R(::pthread_rwlock_rdlock(&mRWLock));
}
void RWLock::wlock() {
    P_ENSURE_R(::pthread_rwlock_wrlock(&mRWLock));
}
void RWLock::unlock() {
    P_ENSURE_R(::pthread_rwlock_unlock(&mRWLock));
}

Thread::Thread(pthread_t tid): mTid(tid) {
}
void* Thread::threadProc(void *p) {
    ThreadCtx *ctx = (ThreadCtx*)p;
    function<void()> f = ctx->f;
    DELETE(ctx);

    try {
        f();
    } catch(const RuntimeException& e) {
        LOG_ERR("Uncaught runtime excpetion:\n %s", e.what());
    } catch(const LogicError& e) {
        LOG_ERR("Uncaught logic error:\n %s", e.what());
        exit(1);
    } catch(const exception& e) {
        LOG_ERR("Uncaught unkown exception:\n %s", e.what());
        exit(2);
    }
    return nullptr;
}
Thread Thread::create(function<void()> f, int stackSize) {
    pthread_attr_t attr, *pattr = nullptr;
    if (stackSize != 0) {
        P_ENSURE_R(::pthread_attr_init(&attr));
        P_ENSURE_R(::pthread_attr_setstacksize(&attr, stackSize));
        pattr = &attr;
    }
    
    pthread_t tid;

    {
        ThreadCtx *ctx = new ThreadCtx{ f };
        ON_EXIT_SCOPE([&ctx](){ DELETE(ctx); });

        P_ENSURE_R(::pthread_create(&tid, pattr, &Thread::threadProc, ctx));

        ctx = nullptr;
    }

    return Thread(tid);
}
Thread Thread::current() {
    return Thread(::pthread_self());
}
void Thread::sleep(int us) {
    P_ENSURE(::poll(nullptr, 0, us) == 0);
}
void Thread::join() {
    P_ENSURE_R(::pthread_join(mTid, nullptr));
}
void Thread::cancel() {
    P_ENSURE_R(::pthread_cancel(mTid));
}
void Thread::detach() {
    P_ENSURE_R(::pthread_detach(mTid));
}
void Thread::setCpuAffinity(int mask) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < 32; ++i) {
        if ((mask >> i) & 1) {
            CPU_SET(i, &cpuset);
        }
    }

    P_ENSURE(::pthread_setaffinity_np(mTid, sizeof(cpuset), &cpuset) == 0);
}

Waiter::Waiter(): mSignaled(false) {
}
void Waiter::wait() {
    LockGuard guard(mMutex);
    while (!mSignaled) mCond.wait(&mMutex);
}
void Waiter::signal() {
    LockGuard guard(mMutex);
    mSignaled = true;
    mCond.broadcast();
}
void Waiter::resetSignal() {
    LockGuard guard(mMutex);
    mSignaled = false;
    mCond.broadcast();
}
