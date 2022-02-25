#include "scheduler.h"
#include "coroutine.h"
#include <iostream>
#include <unistd.h>
#include <vector>

extern thread_local CallerCoroutine::ptr t_thread_corou;
extern thread_local Coroutine* t_running_corou;

static int seq = 0;

void coroFunc() {
    int cur = ++seq;
    std::cout << cur << " first call func" << cur << std::endl;
    sleep(1);
    t_running_corou->yield();
    std::cout << cur << " second call func" << cur << std::endl;
    sleep(1);
    t_running_corou->yield();
    std::cout << cur << " third call func" << cur << std::endl;
}

int main() {
    Scheduler *sc = new Scheduler(3);
    Coroutine *tk = nullptr;
    std::vector<Coroutine::ptr> allCorou(100);
    for(int i=0; i<allCorou.size(); ++i) {
        allCorou.at(i) = std::make_shared<Coroutine>(coroFunc);
        tk = allCorou[i].get();
        sc->addTask(tk);
    }

    sc->init();
    sleep(20);
    sc->shutdown();

    delete sc;
    return 0;
}