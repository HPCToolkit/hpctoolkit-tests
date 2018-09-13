#ifndef __FIB_THREAD_H__
#define __FIB_THREAD_H__

#include <thread>
#include <future>

void
fib_thread(int n, std::promise<int> &&result); 

#endif
