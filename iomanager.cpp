#include "iomanager.h"
#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


IOManager::IOManager(int cnt)
    :Scheduler(cnt) {
    m_epfd = epoll_create(5000);
    assert(m_epfd>0);

    int ret = pipe(m_tickleFds);
    assert(!ret);

    epoll_event ev;
    memset(&ev, 0, sizeof(epoll_event));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_tickleFds[0];

    ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    assert(!ret);

    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ev);
    assert(!ret);
}

void IOManager::tickle() {
    write(m_tickleFds[1], "P", 1);
}

void IOManager::idle() {
    epoll_event *events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *rawptr){delete [] rawptr;});
    while(!is_shutdown) {
        int ret = epoll_wait(m_epfd, events, MAX_EVENTS, MAX_TIMEOUT);
        std::cout << ret << std::endl;
        if(ret<0) {
            if(errno == EINTR)
                continue;
            break;
        }

        for(int i=0; i<ret; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                char dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy))>0);
                continue;
            }
            SocketContext *sock_ctx = (SocketContext*)event.data.ptr;
            this->addTask(sock_ctx->sockCoro);
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    std::cout << "addEvent call" << std::endl;
    std::unique_lock<std::mutex> lock(m_mtx);   
    if(!m_sockContexts.count(fd)) {
        m_sockContexts.insert(std::make_pair(fd, new SocketContext(fd)));       // 添加fd对应的socket上下文
    }
    SocketContext* sock_ctx = m_sockContexts[fd];
    if(sock_ctx->sockCoro)
        delete sock_ctx;
    sock_ctx->sockCoro = new Coroutine(cb);     // 指定fd对应的协程处理函数

    int op = sock_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;      // 决定epoll是修改事件还是添加新事件
    epoll_event nw_event;
    nw_event.events = EPOLLET | sock_ctx->events | event;       // 添加一个事件event
    nw_event.data.ptr = sock_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &nw_event);
    tickle();
    return rt;
}

int IOManager::delEvent(int fd, Event event) {
    SocketContext *sock_ctx = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if(!m_sockContexts.count(fd))
            return -1;
        sock_ctx = m_sockContexts[fd];
    }
    Event cur_event = (Event)(sock_ctx->events&~event);
    int op = cur_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    
    epoll_event new_event;
    new_event.events = EPOLLET | cur_event;
    new_event.data.ptr = sock_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt)  // 删除错误
        return -1;
    if(!cur_event) {        // fd对应的事件为空
        m_sockContexts.erase(fd);
        return -1;
    }

    // 更新sock上下文信息
    sock_ctx->events = cur_event;
    return 1;
}


