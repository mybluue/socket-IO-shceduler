#include "scheduler.h"
#include <atomic>
#include <unistd.h>
#include <iostream>
#include <assert.h>

extern thread_local CallerCoroutine::ptr t_thread_corou;
extern thread_local Coroutine* t_running_corou;

static std::atomic<int> thread_id {0};

Scheduler::Worker::Worker(Scheduler* sc) 
    :worker_id(++thread_id)
    ,m_scheduler(sc) {
    m_caller =  std::shared_ptr<CallerCoroutine>(new CallerCoroutine);
}

void Scheduler::Worker::operator()() {
    t_thread_corou = m_caller;
    Coroutine *curco = nullptr;
    while(!m_scheduler->is_shutdown) {
        std::unique_lock<std::mutex> lock(m_scheduler->m_mutex);
        curco = m_scheduler->m_tasks->deQueue();
        if(!curco) {
            m_scheduler->idle();
            continue;
        }
        std::cout << "thread: " << worker_id << " is working!" << std::endl;
        t_thread_corou->resume(curco);
        if(curco->m_state!=Coroutine::TERM) {
            m_scheduler->addTask(curco);
        }
    }
}


Scheduler::Scheduler(int cnt)
    :thread_cnt(cnt)
    ,is_shutdown(false) {
    m_tasks = new TaskQueue();
}

Scheduler::~Scheduler() {
    delete m_tasks;
}

void Scheduler::tickle() {

}

void Scheduler::idle() {

}

void Scheduler::addTask(Coroutine *tk) {
    m_tasks->enQueue(tk);
    tickle();
}


void Scheduler::init() {
    m_threads.resize(thread_cnt);
    for(int i=0; i<thread_cnt; ++i) {
        m_threads.at(i) = std::thread(Worker(this));
    }
}

void Scheduler::shutdown() {
    tickle();
    sleep(5);
    is_shutdown = true;
    for(int i=0; i<thread_cnt; ++i) {
        if(m_threads.at(i).joinable())
            m_threads.at(i).join();
    }
}

