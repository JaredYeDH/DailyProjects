#ifndef UTILS_H
#define UTILS_H

#include <exception>
#include <utility>
#include <functional>
#include <sstream>
//////////////////////////////
extern string format(const char *fmt, ...);

#define _TO_STRING(a) #a
#define TO_STRING(a) _TO_STRING(a)
#define _CONN(a, b) a##b
#define CONN(a, b) _CONN(a, b)

#define DISABLE_COPY(type) type(const type&) = delete; type& operator = (const type&) = delete
#define ENABLE_COPY(type) 

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define CLOSE(fd) if (fd == -1); else { P_ENSURE(::close(fd) == 0); fd = -1; }
#define DELETE(p) { delete p; p = nullptr; }
//////////////////////////////
#define EXCEPTION_CTX_OPERATOR(type) template<typename T>\
    type& operator << (pair<const char*, T> varInfo) {\
        ostringstream os;\
        os << "\t\t" << varInfo.first << ':' << varInfo.second << '\n';\
        mWhat += os.str();\
        return *this;\
    }\
    type& operator << (const char *s) { return *this; }

class Throwable: public exception {
    ENABLE_COPY(Throwable)
public:
    const char* what() const throw() { return mWhat.c_str(); }
protected:
    Throwable(const char *what): mWhat(what){}
protected:
    string mWhat;
};

class LogicError: public Throwable {
    ENABLE_COPY(LogicError)
public:
    LogicError(const char *describ, const char *file, int line);
    EXCEPTION_CTX_OPERATOR(LogicError);
private:
    string constructErrorMsg(const char *describ, const char *file, int line);
};

class RuntimeException: public Throwable {
    ENABLE_COPY(RuntimeException)
protected:
    RuntimeException(const char *what): Throwable(what){}
};

class PosixException: public RuntimeException {
    ENABLE_COPY(PosixException)
public:
    PosixException(int err, const char *describ, const char *file, int line);
    EXCEPTION_CTX_OPERATOR(PosixException);
private:
    string constructErrorMsg(int err, const char *describ, const char *file, int line);
};

#undef EXCEPTION_CTX_OPERATOR

extern const char *_EXCEPTION_CTX_A;
extern const char *_EXCEPTION_CTX_B;
#define _EXCEPTION_CTX_A(v) make_pair(#v, v) << _EXCEPTION_CTX_B
#define _EXCEPTION_CTX_B(v) make_pair(#v, v) << _EXCEPTION_CTX_A
#define P_ENSURE_ERR(b, err) if (b); else throw PosixException(err, #b, __FILE__, __LINE__) << _EXCEPTION_CTX_A
#define P_ENSURE(b) P_ENSURE_ERR(b, errno)
#define P_ENSURE_R(exp) for (int err = exp; err != 0; err = 0) throw PosixException(err, #exp, __FILE__, __LINE__) << _EXCEPTION_CTX_A
#define ASSERT(b) if (b); else throw LogicError(#b, __FILE__, __LINE__) << _EXCEPTION_CTX_A

//////////////////////////////
class ScopeGuard {
    DISABLE_COPY(ScopeGuard);
public:
    ScopeGuard(function<void()> f): mF(f) {}
    ~ScopeGuard() { if (mF) mF(); }
    void dismiss() { mF = nullptr; }
private:
    function<void()> mF;
};

#define ON_EXIT_SCOPE(f) ScopeGuard CONN(__scopeVar_, __LINE__)(f)
//////////////////////////////
extern string trimString(const char *str);
extern string cmdOpenAndRetrieve(const char **args, const char *input);
extern int getCpuCount();
extern bool readFile(const char *path, vector<char> &buf);
extern string traceStack(int skipFrame);

typedef void(*SigHandlerT)(int);
extern SigHandlerT setSignalHandler(int signum, SigHandlerT handler);
//////////////////////////////
class ILogger {
    DISABLE_COPY(ILogger);
public:
    ILogger(){}
    static ILogger* instance();
    virtual void suppressLog(bool b) = 0;
    virtual void log(const char *msg) = 0;
    virtual void logErr(const char *msg) = 0;
};

#define LOG(fmt, ...)  ILogger::instance()->log(format("%s(%d): " fmt, __FILE__, __LINE__, __VA_ARGS__).c_str())
#define LOG_ERR(fmt, ...) ILogger::instance()->logErr(format("%s(%d): " fmt, __FILE__, __LINE__, __VA_ARGS__).c_str())
#define LOG_MSG(msg)  LOG("%s", msg)
#define LOG_ERR_MSG(msg) LOG_ERR("%s", msg)
//////////////////////////////

template<int BLOCK_SIZE, int CHUNK_SIZE = 4 * (4 * 1024)>
class MemoryPool {
    DISABLE_COPY(MemoryPool);
public:
    void* malloc() {
        ++mMallocCount;
        if (mFreeList == nullptr) allocChunk();
        Block *b = mFreeList;
        mFreeList = b->next;
        return b;
    }
    void free(void *p) {
        --mMallocCount;
        Block *b = (Block*)p;
        b->next = mFreeList;
        mFreeList = b;
    }

    MemoryPool(): mFreeList(nullptr), mMallocCount(0) {
    }
    ~MemoryPool() {
        ASSERT(mMallocCount == 0);
        for (void *chunk : mChunks) ::free(chunk);
    }
private:
    void allocChunk() {
        Block *p = (Block*)::malloc(CHUNK_SIZE);
        mChunks.push_back(p);
        Block *pend = (Block*)((char*)p + CHUNK_SIZE);

        for (; p + 1 <= pend; ++p) {
            p->next = mFreeList;
            mFreeList = p;
        }
    }
private:
    struct Block {
        Block *next;
        char padding[BLOCK_SIZE - sizeof(Block*)];
    };
private:
    Block *mFreeList;
    vector<void*> mChunks;
    int mMallocCount;
};

#endif
