#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

extern int max_thread_count;

struct ThreadPool
{
  //static const int max_thread_count = 20;
  static const int max_free_thread_count = 2;
  static std::atomic<int> thread_count;
  static std::atomic<int> free_thread_count;
  static QueueThreadSafe<ThreadPool*> free_threads;
  
  typedef std::function<void()>* task_t;
  
  void push(task_t task, bool locked = false)
  {
    if(locked) {
      _tasks.push_unlocked(task);
      return;
    }
    if(_tasks.length() >= 0) {
      if(free_threads.length() < 0)
	free_threads.push(this);
      else {
	if(thread_count < max_thread_count) {
	  std::thread thrd(ThreadPool::thread_proxy, this);
	  thrd.detach();
	}
      }
    }
    _tasks.push(task);
  }

  unique_lock tasks_lock() { return _tasks.lock(); }
  
private:
  static void thread_proxy(ThreadPool * pool)
  {
    std::cout << '.' << std::flush;
    PlusTemp tmp(thread_count);
    do{
      boost::optional<task_t> t;
      while(t = pool->_tasks.timed_pop(100))
	(*(*t))();
      if(thread_count > max_thread_count)
	break;
      PlusTemp tmp(free_thread_count);
      if(free_thread_count > max_free_thread_count)
	break;
      pool = free_threads.pop();
    } while(pool);
    std::cout << '*' << std::flush;
  }
  QueueThreadSafe<task_t> _tasks;
};



#endif

// int main(int argc, char* argv[])
// {
//   for(;;) {
//     int n;

//     std::cout << "input: " << std::flush;
//     std::cin >> n;

//     static std::vector<std::pair<std::string, double> > v_name_second;
//     v_name_second.clear();
//     max_thread_count = 4;
    
//     while(max_thread_count <= 40) {
//       static ThreadPool pool;
//       int i;
//       std::atomic<int> count{0};
      
//       char buf[128];
//       sprintf(buf, "core_%d_cnt_%d", max_thread_count, n+40);
//       timer t(buf, v_name_second);
    
//       auto a =[&]() {
// 	int c = 0;
// 	for(int j=i; j<i+1024; ++j)
// 	  c += j;
// 	count++;
// 	if(count >= (n+40)) t.stop();
//       };
//       static std::function<void()> f(a);
    
//       {
// 	auto l = pool.tasks_lock();
// 	for(i=0; i<n; ++i)
// 	  pool.push(&f, true);
//       }

//       for(i=0; i<40; ++i) 
// 	pool.push(&f);

//       for(;;) {
// 	if(count >= (n+40)) break;
// 	printf("\n{thrd:%d,free_thrd:%d,cnt:%d}\n",
// 	       int(ThreadPool::thread_count),  int(ThreadPool::free_thread_count), int(count));
// 	std::cout << std::flush;
// 	msleep(1000);
//       }
//       msleep(100);
//       max_thread_count += 4;
//     }

//     for(auto b : v_name_second) 
//       printf("\n%30s : %f\n", b.first.c_str(), b.second);
//   }
  
//   return 0;
// }
