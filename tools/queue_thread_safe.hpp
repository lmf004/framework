#ifndef __QUEUE_THREAD_SAFE_HPP__
#define __QUEUE_THREAD_SAFE_HPP__

#define msleep(x)							\
  do {									\
    std::this_thread::sleep_for(std::chrono::milliseconds((x)));	\
  } while(0)								\
    /**/

typedef std::lock_guard<std::mutex> lock_guard;
typedef std::unique_lock<std::mutex> unique_lock;

template<typename T>
struct QueueThreadSafe
{
  void push(T const & t) {
    unique_lock lk(_mtx);
    _queue.emplace_back(t);
    lk.unlock();
    _cv.notify_one();
  }

  void push_unlocked(T const & t) {
    _queue.emplace_back(t);
  }
  
  T & pop() {
    unique_lock lk(_mtx);
    _cv.wait(lk, [this]{ return !_queue.empty(); } );
    auto a = _queue.front();
    _queue.pop_front();
    return a;
  }

  T & pop_unlocked() {
    while(_queue.empty())
      msleep(10);
    auto a = _queue.front();
    _queue.pop_front();
    return a;
  }

  boost::optional<T&>
  try_pop() {
    unique_lock lk(_mtx);
    if(_queue.empty())
      return boost::optional<T&>();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T&>(a);
  }

  boost::optional<T&>
  try_pop_unlocked() {
    if(_queue.empty())
      return boost::optional<T&>();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T&>(a);
  }

  boost::optional<T&>
  timed_pop(int t) {
    unique_lock lk(_mtx);
    if(!_cv.wait_for(lk, std::chrono::milliseconds(t), [this]{ return !_queue.empty(); } ) )
      return boost::optional<T&>();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T&>(a);
  }  

  boost::optional<T&>
  timed_pop_unlocked(int t) {
    t /= 10;
    while(_queue.empty() && t-- > 0) 
      msleep(10);
    if(_queue.empty())
      return boost::optional<T&>();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T&>(a);
  }  

  unique_lock lock() {
    return unique_lock(_mtx);
  }

private:
  std::deque<T> _queue;
  std::mutex    _mtx;
  std::condition_variable _cv;
};

#endif


// typedef std::lock_guard<std::mutex> lock_guard;

// #define msleep(x)							\
//   do {									\
//     std::this_thread::sleep_for(std::chrono::milliseconds((x)));	\
//   } while(0)								\
//     /**/

// #define std_lock(m1, m2)			\
//   std::lock(m1, m2);				\
//   lock_guard lock1(m1, std::adopt_lock);	\
//   lock_guard lock2(m2, std::adopt_lock);	\
//   /**/
