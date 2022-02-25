#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <ucontext.h>
#include <functional>
#include <memory>

#define DEFAULT_SIZE (1024*1024)

class Scheduler;

class Coroutine :public std::enable_shared_from_this<Coroutine> {
    friend class Scheduler;
public:
    typedef std::shared_ptr<Coroutine> ptr;
    enum State {
    READY = 1,
    RUNNING,
    HOLD,
    TERM
    };

public:
    Coroutine();
    Coroutine(std::function<void()> func);
    int getId() {return m_id;}
    State getState() {return m_state;}
    void yield();
    void resume();
    static void corouFunc();
    static void setThis(Coroutine* cptr);      // 将this指针设置为正在运行的协程指针
    static Coroutine::ptr getThis();        // 获取正在运行的协程的this指针
    static void initCallerCorou();
private:
    ucontext_t m_ctx;
    char m_stack[DEFAULT_SIZE];
    State m_state;
    int m_id;
    std::function<void()> m_cb;
};

class CallerCoroutine {
public:
    typedef std::shared_ptr<CallerCoroutine> ptr;
public:
    void resume(Coroutine::ptr co) {
        co->resume();
    }
    void resume(Coroutine* co) {
        co->resume();
    }
    ucontext_t caller_ctx;
};




#endif