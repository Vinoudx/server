## fibers
每个线程一个root_fiber
对于所有fiber，只能从root_fiber切到fiber，fiber再切回root_fiber


## scheduler_thread
设想两个队列，一个就绪队列，一个阻塞队列
通过监听fiber的状态，将成为就绪的fiber加入就绪队列，将成为阻塞的fiber加入阻塞队列
对于线程池，计划给每个线程都设置一个就绪队列，通过轮询的方法把任务加入线程
对于阻塞队列，每一个scheduler只有一个

scheduler thread tickle()    统一事件源   
                 idle()      陷入epoll_wait
                 run()       从就绪队列中拿出一个任务执行


## scheduler
scheduler管理所有scheduler thread
对外提供schedule(), addEvent(), delEvent(), delAllEvents(), addTimer()接口

## timer
基于红黑树的计时器
基于steady_clock，timerfd超时时间为当前时间到计时器超时的绝对时间差
注意向红黑树加入事件时，如果事件的超时事件为最早的要重置timerfd

## hook
采用静态函数形式的hook
先修改read/write 给tcp
     sendto/recvfrom 给udp/kcp


## timestamp
使用steady_clock产生一个不被系统时间影响的单调uint64时间戳

## socket
通过继承实现tcpsocket，udpsocket，kcpsocket
实现bind，listen, accept，相关read/write

## 细节
在swapcontext前要把这个函数里的所有shared_ptr都释放掉
在swapcontext后不保证执行流还会回到这里，所以应该在swapcontext前把该释放的都释放了