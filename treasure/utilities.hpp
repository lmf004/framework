#ifndef __TREASURE_UTILITIES_HPP__
#define __TREASURE_UTILITIES_HPP__

void loghex(const std::string title, const unsigned char* p, int len = 0);
void loghex(const std::string title, const char* p, int len = 0);


std::string string_format(const char* format, ...);
//void print(const char* format, ...);
extern bool g_last_is_dot;

#define PRINT(format, ...)						\
  do {									\
    if(g_last_is_dot) g_print("\n");					\
    g_print(format, __VA_ARGS__);					\
    g_last_is_dot = false;						\
  } while(0)								\
    /**/

typedef std::lock_guard<std::mutex> lock_guard;
typedef std::unique_lock<std::mutex> unique_lock;

#define msleep(x)							\
  do {									\
    std::this_thread::sleep_for(std::chrono::milliseconds((x)));	\
  } while(0)								\
    /**/

#define std_lock(m1, m2)			\
  std::lock(m1, m2);				\
  lock_guard lock1(m1, std::adopt_lock);	\
  lock_guard lock2(m2, std::adopt_lock);	\
  /**/

//--------------------------------------------------------------------------------

std::string timestamp_2_string(guint32 stamp);
std::string timestamp_2_date(guint32 stamp);
std::string timestamp_2_time(guint32 stamp);
std::string date_2_week(std::string date);

//--------------------------------------------------------------------------------

struct scoper
{
  static int steps;
  std::string do_step(int n){
    std::string str;
    for(int i=0; i<n; ++i) str += "|_";
    return str;
  }
  std::string print_time(){
    time_t t; time(&t);
    return timestamp_2_time(t);
  }
  scoper(char const* file, int line, char const * func, char const * sub = NULL) : _file(file), _line(line), _func(func) {
    if(sub) _func = _func + "::" + sub;
    PRINT("%s %s( %s ) -->> %s %d\n", print_time().c_str(), do_step(steps).c_str(), _func.c_str(), _file, _line);
    steps++;
  }
  ~scoper() {
    steps--;
    PRINT("%s %s( %s ) <<-- %s %d\n", print_time().c_str(), do_step(steps).c_str(), _func.c_str(), _file, _line);
  }
  char const * _file;
  int _line;
  std::string _func;
};

#define DEBUG_MODE

#if defined DEBUG_MODE
#define scope() scoper slmf(__FILE__, __LINE__, __func__)
#define sub_scope(name) scoper slmf(__FILE__, __LINE__, __func__, name)
#else
#define scope() (void)0
#define sub_scopes(name) (void)0
#endif

#if defined DEBUG_MODE
#define dot(i) real_dot(i)
#else
#define dot(i) (void)0
#endif

#if defined DEBUG_MODE
#define less_dot(str) do { static int i = 0; if(0 == i++%100) real_dot(str); } while(0)
#define less_dot_ex(str, c) do { static int i = 0; if(0 == i++%c) real_dot(str); } while(0)
#else
#define less_dot(i) (void)0
#define less_dot_ex(str, c) (void)0
#endif

void real_dot(char const * str);
void real_dot(std::string const & str);

//--------------------------------------------------------------------------------
struct Tick
{
  static Tick * instance(){ static Tick t; return &t; }
  int tick() { return _tick; }
  void run(std::shared_ptr<bool> running){ while(*running) { sleep(1); _tick++; less_dot("."); } }
private:
  Tick() : _tick(0)  {}
  int _tick;
};
int tick();
// auto tick_future = std::async(std::launchL::async, &Tick::run, Tick::instance(), std::ref(running));

#define timer_exec(n, job)					\
  {								\
    int _now_ = tick();						\
    static int _last_ = _now_;					\
    if(_now_ - _last_ > (n)) { { job; } _last_ = _now_; }	\
  }								\
  /**/

#define count_exec(n, job)						\
  {									\
    static int _i_ = 0;							\
    if(0 == ++_i_ % n) { job; }						\
  }									\
  /**/

#define timer_count_exec(t, n, job)					\
  {									\
    static int _i_ = 0;							\
    int _now_ = tick();							\
    static int _last_ = _now_;						\
    ++_i_;								\
    if(_now_ - _last_ > (t) || 0 == _i_ % (n)) { { job; } _last_ = _now_; _i_=0; } \
  }									\
  /**/

/*********************************************************************************************
int main()
{
  auto running = std::make_shared<bool>(true);
  auto tick_future = std::async(std::launch::async, &Tick::run, Tick::instance(), running);
    
  for(int i=0; i<100000; ++i){
    //timer_exec(3, std::cout << "ok, i = " << i << '\n');
    //count_exec(100, std::cout << "hi, i = " << i << '\n');
    timer_count_exec(3, 10000, std::cout << "yo, i = " << i << '\n');
    //usleep(1*1000);
  }
  std::string str("i am good");
  std::cout << str << '\n';
    
  sleep(10);
  *running = false;
  return 0;
}
*********************************************************************************************/

//--------------------------------------------------------------------------------

void string_to_arithmetic(std::string str, char & n);
void string_to_arithmetic(std::string str, unsigned char & n);
void string_to_arithmetic(std::string str, short & n);
void string_to_arithmetic(std::string str, unsigned short & n);
void string_to_arithmetic(std::string str, int & n);
void string_to_arithmetic(std::string str, unsigned int & n);
void string_to_arithmetic(std::string str, long & n);
void string_to_arithmetic(std::string str, unsigned long & n);
void string_to_arithmetic(std::string str, long long & n);
void string_to_arithmetic(std::string str, unsigned long long & n);

//--------------------------------------------------------------------------------
#define FORBIDDEN_MULTI_THREAD(func)						\
  do {									\
       if(_thread_id != std::this_thread::get_id())				\
         { g_warning("%s faild, you can't call %s in other thread!", func, func); return; } \
  }while(0)								\
    /**/
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

struct empty_object
{
  empty_object() : i(0) { g_print("{%d", i); }
  empty_object(int i) : i(i){ g_print("{i%d", i); }
  empty_object(empty_object const & rhs){ g_print("c[%d,%d]", i, rhs.i); }
  empty_object(empty_object && rhs){ g_print("r[%d,%d]", i, rhs.i); }
  ~empty_object(){ g_print("%d}", i); }
  empty_object & operator=(empty_object const & rhs) { g_print("=[%d,%d]", i, rhs.i); return *this; }
  empty_object & operator=(empty_object && rhs) { g_print("=r[%d, %d]", i, rhs.i); return *this; }
  int i;
};

//--------------------------------------------------------------------------------

#define REPEAT(x, t) while(*running) { if(0 == (x)) break; dot("R"); usleep(t*1000); } 

//--------------------------------------------------------------------------------

struct raw_buf;

struct wait_event
{
  static wait_event & instance(){ static wait_event a; return a; }
  void notify(std::string evt, int seq, shared_ptr<raw_buf> data){
    auto id = evt + std::to_string(seq); 
    std::lock_guard<std::mutex> lock(_mtx);
    evts[id] = std::make_pair(true, data);
    dot("notify:"+id+"\n");
  }
  shared_ptr<raw_buf> wait(std::string evt, int seq, shared_ptr<bool> running) {
    auto id = evt + std::to_string(seq); 
    dot("wait:"+id+"\n");
    
    while(*running) {
      {
	std::lock_guard<std::mutex> lock(_mtx);
	if(evts.find(id) != evts.end() && evts[id].first) { 
	  evts[id].first = false; return evts[id].second; 
	}
      }
      usleep(100*1000);
    }
    return shared_ptr<raw_buf>();
  }
  int timed_wait(std::string evt, int seq, int millisecond, shared_ptr<raw_buf> & data, shared_ptr<bool> running) {
    auto id = evt + std::to_string(seq); 
    dot("wait:"+id+"\n");
    int cnt = millisecond / 100;
    while(*running && cnt--) {
      {
	std::lock_guard<std::mutex> lock(_mtx);
	if(evts.find(id) != evts.end() && evts[id].first) { 
	  evts[id].first = false; data = evts[id].second; return 0;
	}
      }
      usleep(100*1000);
    }
    return 1;
  }
private:
  std::map<std::string, std::pair<bool, shared_ptr<raw_buf> > > evts;
  std::mutex _mtx;
};

shared_ptr<raw_buf> wait(std::string evt, int seq, shared_ptr<bool> running);
int timed_wait(std::string evt, int seq, int millisecond, shared_ptr<raw_buf> & data, shared_ptr<bool> running);
void notify(std::string evt, int seq, shared_ptr<raw_buf> data = shared_ptr<raw_buf>());

// int main(int argc, char* argv[])
// {
//   g_print("------001\n");
//   auto running = make_shared<bool>(true);
//   int a;
//   for(int i=0; i<100; ++i)
//     notify("connect", &a+i, "good" + std::to_string(i));
//   auto f = std::async(std::launch::async, [&](){
//       g_print("------002\n");
//       sleep(10);
//       for(int i=0; i<100; ++i)
// 	notify("connect", &a+i, "good" + std::to_string(i));
//       g_print("------003\n");
//     });
//   for(int i=0; i<100; ++i) {
//     auto data = wait("connect", &a+i, running);
//     std::cout << "---------" << i << " " << data << '\n';
//   }  
//   for(int i=0; i<100; ++i) {
//     auto data = wait("connect", &a+i, running);
//     std::cout << "---------" << i << " " << data << '\n';
//   }  
//   return 0;
// }
//--------------------------------------------------------------------------------

std::string get_localhost_ip();
std::vector<std::string> get_localhost_iplist();

//---------------------------------------------------------------------------------
struct MinusTemp
{
  MinusTemp(std::atomic<int> & count, int n = 1)
    : _count(count), _n(n), _commit(false)
  { this->_count -= _n; }
  ~MinusTemp() {
    if(!_commit) this->_count += _n;
  }
  void commit() { _commit = true; }
  std::atomic<int> & _count;
  int _n;
  bool _commit;
};

struct PlusTemp
{
  PlusTemp(std::atomic<int> & count, int n = 1)
    : _count(count), _n(n), _commit(false)
  { this->_count += _n; }
  ~PlusTemp() {
    if(!_commit) this->_count -= _n;
  }
  void commit() { _commit = true; }
  std::atomic<int> & _count;
  int _n;
  bool _commit;
};

struct timer
{
  typedef std::vector<std::pair<std::string, double> > times_type;
  typedef decltype(chrono::system_clock::now()) now_type ;
  timer(std::string name, times_type & v, bool delay = false)
    : name(name), v(v) {
    if(!delay) do_start();
  }
  ~timer() {
    if(started && !stoped) do_stop();
  }
  void start() {
    do_start();
  }
  void stop() {
    if(started) {
      do_stop();
      stoped = true;
    }
  }
private:
  void do_start() {
    begin = chrono::system_clock::now();
    started = true;
  }
  void do_stop() {
    auto end = chrono::system_clock::now();
    double seconds = double(chrono::duration_cast<chrono::microseconds>(end-begin).count())/(1000*1000);
    v.push_back(std::pair<std::string, double>(name, seconds));
    stoped = true;
  }
  std::string name;
  now_type begin;
  bool started{false};
  bool stoped{false};
  times_type & v;
};

//---------------------------------------------------------------------------------

#endif
