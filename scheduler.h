#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "coroutine.h"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <iostream>

class TaskQueue {       // 线程安全的任务队列
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
    std::queue<Coroutine*> m_queue;     // 协程队列
    std::mutex m_taskMutex;     // 队列锁
};


class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    class Worker {      // 线程入口仿函数
    public:
        Worker(Scheduler* sc);
        void operator()();
    private:
        Scheduler* m_scheduler;
        int worker_id;
        CallerCoroutine::ptr m_caller=nullptr;
    };
protected:
    virtual void idle();        // 阻塞线程
    virtual void tickle();      // 线程唤醒
public:
    Scheduler(int cnt);     // 指定线程数量
    ~Scheduler();
    void addTask(Coroutine* tk);
    // void schedule();
    void init();        // 创建线程并启动
    void shutdown();        // 关闭线程池
protected:
    TaskQueue *m_tasks;     // 当前调度器的任务队列
    std::vector<std::thread> m_threads;     // 线程池
    std::mutex m_mutex;     
    int thread_cnt;
    bool is_shutdown;       // 是否关闭线程池
};

#endif
