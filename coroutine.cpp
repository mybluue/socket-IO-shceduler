#include "coroutine.h"
#include <thread>
#include <assert.h>
#include <iostream>
#include <errno.h>

static int s_corou_id = 0;      // 用于初始化协程实例的id

thread_local CallerCoroutine::ptr t_thread_corou(new CallerCoroutine);      // 当前线程的调度协程，也即主协程

thread_local Coroutine* t_running_corou = nullptr;      // 当前线程正在执行的协程

Coroutine::Coroutine() {
    // m_state = RUNNING;
    getcontext(&m_ctx);
}

Coroutine::Coroutine(std::function<void()> func)
    :m_cb(func)
    ,m_id(++s_corou_id) {
    getcontext(&m_ctx);
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = DEFAULT_SIZE;
    m_state = READY;
    makecontext(&m_ctx, &Coroutine::corouFunc, 0);
}


void Coroutine::corouFunc() {
    assert(t_running_corou);
    Coroutine::ptr cur = getThis();
    // std::cout << cur->m_state << ", " << READY << std::endl;
    assert(cur && cur->m_state==RUNNING);
    
    cur->m_cb();
    cur->m_state = TERM;

    cur->yield();
}

void Coroutine::yield() {
    assert(m_state==RUNNING || m_state==TERM);
    if(m_state==RUNNING)
        m_state = HOLD;
    swapcontext(&m_ctx, &t_thread_corou->caller_ctx);
}

void Coroutine::resume() {
    setThis(this);
    assert(t_running_corou);
    
    assert(m_state==HOLD || m_state==READY);
    m_state = RUNNING;
    assert(t_thread_corou);
    
    if(swapcontext(&t_thread_corou->caller_ctx, &m_ctx)==-1) {
        perror("swapcontext error");
    }
    // std::cout << "resume success" << std::endl;
}

void Coroutine::setThis(Coroutine* cptr) {
    t_running_corou = cptr;
}

Coroutine::ptr Coroutine::getThis() {
    assert(t_running_corou);
    return t_running_corou->shared_from_this();
}

