#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <ucontext.h>
#include <functional>
#include <memory>

#define DEFAULT_SIZE (1024*1024)        // 协程栈默认大小

class Scheduler;        // 声明Scheduler，Coroutine中声明其为友元类，避免交叉include

class Coroutine :public std::enable_shared_from_this<Coroutine> {       // 协程实例由智能指针管理时，安全地将this指针传递给外部
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
    static void corouFunc();        // 协程入口函数，包裹回调函数m_cb
    static void setThis(Coroutine* cptr);      // 将this指针设置为正在运行的协程指针
    static Coroutine::ptr getThis();        // 获取正在运行的协程的指针，即当前实例的this指针
    static void initCallerCorou();
private:
    ucontext_t m_ctx;       // 协程山下文
    char m_stack[DEFAULT_SIZE];     // 协程栈
    State m_state;      // 协程状态
    int m_id;       // 协程id
    std::function<void()> m_cb;     // 回调函数
};


class CallerCoroutine {     // 线程的主协程--调度协程
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
