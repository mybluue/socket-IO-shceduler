#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "coroutine.h"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <iostream>

class TaskQueue {
public:
    void enQueue(Coroutine* co) {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        m_queue.emplace(co);
    }
    Coroutine* deQueue() {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        Coroutine * res = nullptr;
        while(!res && !m_queue.empty()) {
            res = m_queue.front();
            m_queue.pop();
        }
        return res;
    }
    int taskSize() {
        return m_queue.size();
    }

private:
    std::queue<Coroutine*> m_queue;
    std::mutex m_taskMutex;
};


class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    class Worker {
    public:
        Worker(Scheduler* sc);
        void operator()();
    private:
        Scheduler* m_scheduler;
        int worker_id;
        CallerCoroutine::ptr m_caller=nullptr;
    };
protected:
    virtual void idle();
    virtual void tickle();
public:
    Scheduler(int cnt);
    ~Scheduler();
    void addTask(Coroutine* tk);
    // void schedule();
    void init();        // 创建线程并启动
    void shutdown();
protected:
    TaskQueue *m_tasks;
    std::vector<std::thread> m_threads;
    std::mutex m_mutex;
    int thread_cnt;
    bool is_shutdown;
};




#endif