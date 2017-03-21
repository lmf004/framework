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
