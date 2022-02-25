#ifndef __IOMANAGER_H__
#define __IOMANAGER_H__

#include "scheduler.h"
#include <memory>
#include <unordered_map>
#include <unistd.h>
#include <stdio.h>

#define MAX_EVENTS 256      // epoll_wait最多检测的描述符数量
#define MAX_TIMEOUT 5000

enum Event {
    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x4,
};
class SocketContext {
public:
    int fd;
    Event events;
    Coroutine* sockCoro; 
    SocketContext(int x)
        :fd(x)
        ,events(NONE)
        ,sockCoro(nullptr) {
    }
};

class IOManager : public Scheduler {
public:
    std::shared_ptr<IOManager> ptr;
    int addEvent(int fd, Event event, std::function<void()> cb);        // 同时修改m_sockContexts和epoll监控的描述符事件
    int delEvent(int fd, Event event);
    void stop() {
        for(auto p:m_sockContexts) {
            tickle();
            close(p.first);
        }
    }
public:
    IOManager(int cnt);     // 设置管道，并将其读端加入epoll监听
protected:
    virtual void idle();
    virtual void tickle();
private:
    int m_epfd = 0;     // epoll描述符
    int m_tickleFds[2];     // 管道
    std::unordered_map<int, SocketContext*> m_sockContexts;
    std::mutex m_mtx;       // 保证添加删除事件是线程安全的
};

#endif