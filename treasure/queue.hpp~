#ifndef __TREASURE_QUEUE_HPP__
#define __TREASURE_QUEUE_HPP__

struct message_queue
{
  message_queue(std::string name, int max_number = 1000, int max_size = 512) {
    try{
      _mq = make_shared<interprocess::message_queue>(interprocess::open_or_create, name.c_str(), max_number, max_size);
    }
    catch(interprocess::interprocess_exception & e){ std::cout << e.what() << std::endl; }
  }
  ~message_queue(){ }
  
  bool timed_push(shared_ptr<raw_buf> buf, int priority = 0) {
    bool ret = false;
    try{
      int cnt = *(unsigned short*)buf->data();
      if(cnt > buf->size() || cnt > _mq->get_max_msg_size()) {
	g_warning("too big message size=%d!", cnt); return false;
      }
      auto t = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(1);
      ret = _mq->timed_send(buf->data(), cnt, priority, t);
      //ret = _mq->timed_send(buf->data(), cnt, priority, boost::posix_time::from_time_t(time(NULL)+1));
    } 
    catch(interprocess::interprocess_exception & e){ std::cout << e.what() << std::endl; ret = false; }
    return ret;
  }
  
  bool timed_pop(shared_ptr<raw_buf> buf){
    bool ret = false;
    unsigned int priority;
    interprocess::message_queue::size_type recvd_size;
    try {
      auto t = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(1);
      ret = _mq->timed_receive(buf->data(), buf->size(), recvd_size, priority, t);
      //ret = _mq->timed_receive(buf->data(), buf->size(), recvd_size, priority, boost::posix_time::from_time_t(time(NULL)+1));
    } 
    catch(interprocess::interprocess_exception & e){ std::cout << e.what() << std::endl; ret = false; }
    return ret;
  }
  int get_num_msg() { return _mq->get_num_msg(); }
private:
  shared_ptr<interprocess::message_queue> _mq;
};


// void fill_msg(char* p, char* data, int len)
// {
//   static int i = 0;
//   auto idx = std::to_string(i++);
//   idx = "{" + idx + "}";
//   *(unsigned short*)p = 2 + idx.length() + len;
//   memcpy(p+2, idx.c_str(), idx.length());
//   memcpy(p+2+idx.length(), data, len);
// }

// int main(int argc, char* argv[])
// {
//   bool is_reader = false;
//   bool is_writer = false;

//   if(argc == 1) is_writer = true;
//   if(argc >  1) is_reader = true;
  
//   if(is_writer) {
//     // system("./a.out 1 &");
//     // system("./a.out 2 &");
//     // system("./a.out 3 &");
//   }
  
//   message_queue q1("q1");
//   int cnt = 20;
//   g_print("----------002 %d\n", q1.get_num_msg());
  
//   if(is_writer){
//     //return 0;
//     auto buf = make_shared<raw_buf>();
//     char str[] = "hello world!";
//     for(int i=0; i<cnt; ++i) {
//       int priority = (i % 3) + 1;
    
//       fill_msg(buf->data(), str, strlen(str)+1);
//       g_print("-----001 %s\n", buf->data()+2);
//       if(q1.timed_push(buf, priority)) less_dot(",");
//       else dot("*");
//     }
//   }
  
//   if(is_reader){
//     for(int i=0; i<cnt; ++i){
//       auto buf = make_shared<raw_buf>();
//       if(q1.timed_pop(buf)) {
// 	g_print("%s\n", buf->data()+2);
// 	less_dot(".");
//       }
//       else dot("x");
//     }
//   }
  
//   //sleep(30);
// }

#define MAX_QUEUE_ITEM_COUNT 1024*1024

template<typename T>
struct threadsafe_queue : private boost::noncopyable
{
  typedef T value_type;
  void push(T const & t) {
    bool full = false;
    {
      std::unique_lock<std::mutex> lock(_mutex);
      _queue.push(t); _cv.notify_one();
      if(_queue.size() > MAX_QUEUE_ITEM_COUNT) full = true;
    }
    if(full) sleep(3);
  }
  T pop() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [this]{ return !_queue.empty(); });
    T t = _queue.front(); _queue.pop(); return t;
  }
  boost::optional<T> try_pop(){
    {
      std::unique_lock<std::mutex> lock(_mutex);
      if(!_queue.empty()) 
	{ T t = _queue.front(); _queue.pop(); return boost::optional<T>(t); }
    }
    less_dot("!"); usleep(100*1000); return boost::optional<T>();
  }
  boost::optional<T> timed_pop(){
    std::unique_lock<std::mutex> lock(_mutex);
    if(_cv.wait_for(lock, std::chrono::milliseconds(1000), [this]{ return !_queue.empty(); }))
      { T t = _queue.front(); _queue.pop(); return boost::optional<T>(t); }
    return boost::optional<T>();	
  }
  size_t size() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _queue.size();
  }
private:
  std::condition_variable _cv;
  std::queue<T> _queue;;
  std::mutex _mutex;
};

template<class T>
struct async_queue
{
  async_queue(){
    _queue = g_async_queue_new();
  }
  ~async_queue(){
    while(auto p = g_async_queue_try_pop(_queue))
      delete (T*)p;
    g_async_queue_unref(_queue);
  }
  
  int size() { return g_async_queue_length(_queue); }

  //----------------------------------------------------
  void push(shared_ptr<T> ptr)
  {
    g_async_queue_push(_queue, (gpointer)new shared_ptr<T>(ptr));
  }
  shared_ptr<T> pop()
  {
    shared_ptr<T> * p = (shared_ptr<T> *)g_async_queue_pop(_queue);
    auto a = *p; delete p; return a;
  }
  shared_ptr<T> try_pop()
  {
    shared_ptr<T> * p = (shared_ptr<T> *)g_async_queue_try_pop(_queue);
    if(NULL == p) return shared_ptr<T>();
    auto a = *p; delete p; return a;
  }
  //----------------------------------------------------
  // void push(T const & t){
  //   g_async_queue_push(_queue, (gpointer)(new T(t)));
  // }
  // boost::optional<T> 
  // try_pop(){
  //   auto p = g_async_queue_try_pop(_queue);
  //   if(NULL == p) return boost::optional<T>();
  //   T t = *(T*)p; delete (T*)p; return boost::optional<T>(t);
  // }
  // T pop() {
  //   gpointer p = g_async_queue_pop(_queue);
  //   T t = *(T*)p; delete (T*)p; return t;
  // }
  //----------------------------------------------------
  // template<typename T, typename... Args>
  // void push(Args const & ... args)
  // {
  //   queue.push(new T(args...));
  // }
  // shared_ptr<T> pop(){
  //   return shared_ptr<T>((T*)queue_pop());
  // }
  //----------------------------------------------------

private:
  GAsyncQueue *_queue;
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
  
  unique_lock lock() { return unique_lock(_mtx); }
  
private:
  std::deque<T> _queue;
  std::mutex    _mtx;
  std::condition_variable _cv;
  std::atomic<int> _length{0};
};

// int main()
// {
//   async_queue<empty_object> q;
//   q.push(make_shared<empty_object>(1));
//   auto p = q.pop();
// }

/************************************************************************************************************************
template<typename T>
struct thread_safe_queue : private boost::noncopyable
{
  typedef T value_type;
  void push(T const & t) {
    bool need_slowdown = false;
    {
      boost::lock_guard<boost::mutex> lock(_mutex);
      _queue.push(t);
      if(_queue.size() > 10000) need_slowdown = true;
    } 
    if(need_slowdown) { dot("@"); std::this_thread::sleep_for(std::chrono::seconds(1)); }
  }
  T pop() {
    while(true){
      {
	boost::lock_guard<boost::mutex> lock(_mutex);
	if(!_queue.empty()){
	  T t = _queue.front();
	  _queue.pop();
	  return t;
	}
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
  }
  boost::optional<T> try_pop(){
    {
      boost::lock_guard<boost::mutex> lock(_mutex);
      if(!_queue.empty()){
	T t = _queue.front();
	_queue.pop();
	return boost::optional<T>(t);
      }
    }
    less_dot("#");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    return boost::optional<T>();
  }
  size_t size() {
    boost::lock_guard<boost::mutex> lock(_mutex);
    return _queue.size();
  }
private:
  std::queue<T> _queue;;
  boost::mutex _mutex;
};

************************************************************************************************************************/

#endif
