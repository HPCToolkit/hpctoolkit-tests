#include <thread>
#include <future>

void
fib_thread(int n, std::promise<int> &&result) 
{
  if (n<2) {
    // waiting loop
    for(int i=0; i<32767; i++)
      for(int j=0; j<3276; j++) {}

    result.set_value(1);
  } else {
    std::promise<int> p, q;
    auto pr = p.get_future();
    auto pq = q.get_future();

    std::thread t1(fib_thread, n-1, std::move(p));
    std::thread t2(fib_thread, n-2, std::move(q));
    
    t1.join();
    t2.join();

    int tot = pr.get() + pq.get();
    result.set_value(tot);
  }
}
