#ifndef __QUEUE_THREAD_SAFE_HPP__
#define __QUEUE_THREAD_SAFE_HPP__

#define msleep(x)							\
  do {									\
    std::this_thread::sleep_for(std::chrono::milliseconds((x)));	\
  } while(0)								\
    /**/

typedef std::lock_guard<std::mutex> lock_guard;
typedef std::unique_lock<std::mutex> unique_lock;

struct MinusTemp
{
  MinusTemp(std::atomic<int> & count)
    : _count(count), _commit(false)
  { this->_count--; }
  ~MinusTemp() {
    if(!_commit) this->_count++;
  }
  void commit() { _commit = true; }
  std::atomic<int> & _count;
  bool _commit;
};

template<typename T>
struct QueueThreadSafe
{
  void push(T const & t) {
    unique_lock lk(_mtx);
    _queue.emplace_back(t);
    lk.unlock();
    _cv.notify_one();
    _length++;
  }

  void push_unlocked(T const & t) {
    _queue.emplace_back(t);
    _length++;
  }
  
  T pop() {
    _length--;
    unique_lock lk(_mtx);
    _cv.wait(lk, [this]{ return !_queue.empty(); } );
    auto a = _queue.front();
    _queue.pop_front();
    return a;
  }

  T pop_unlocked() {
    _length--;
    while(_queue.empty())
      msleep(10);
    auto a = _queue.front();
    _queue.pop_front();
    return a;
  }
  
  boost::optional<T>
  try_pop() {
    MinusTemp tmp(_length);
    unique_lock lk(_mtx);
    if(_queue.empty())
      return boost::optional<T>();
    tmp.commit();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T>(a);
  }

  boost::optional<T>
  try_pop_unlocked() {
    MinusTemp tmp(_length);
    if(_queue.empty())
      return boost::optional<T>();
    tmp.commit();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T>(a);
  }

  boost::optional<T>
  timed_pop(int t) {
    MinusTemp tmp(_length);
    unique_lock lk(_mtx);
    if(!_cv.wait_for(lk, std::chrono::milliseconds(t), [this]{ return !_queue.empty(); } ) )
      return boost::optional<T>();
    tmp.commit();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T>(a);
  }  

  boost::optional<T>
  timed_pop_unlocked(int t) {
    MinusTemp tmp(_length);
    t /= 10;
    while(_queue.empty()) { 
      msleep(10);
      if(--t <= 0) break; 
    }
    if(_queue.empty())
      return boost::optional<T>();
    tmp.commit();
    auto a = _queue.front();
    _queue.pop_front();
    return boost::optional<T>(a);
  }  
  
  int length() const { return _length; }
  
  unique_lock lock() {
    return unique_lock(_mtx);
  }
  
private:
  std::deque<T> _queue;
  std::mutex    _mtx;
  std::condition_variable _cv;
  std::atomic<int> _length{0};
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

