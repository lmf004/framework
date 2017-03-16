#ifndef __TREASURE_TASK_HPP__
#define __TREASURE_TASK_HPP__

// remember: balance only for cpu compute, not for device(file, socket etc...), and queue helps a lot.

struct TaskContext
{
  friend class TaskManager;
  TaskContext(std::function<int()> count_func
	      , std::function<void(std::shared_ptr<bool>)> thread_func) 
    : _count_func(count_func), _thread_func(thread_func)
  {}

  ~TaskContext(){ for(auto a : _runnings) *a = false; }
  
  std::function<int()> _count_func;
  std::function<void(shared_ptr<bool>)> _thread_func;

  std::vector<std::shared_ptr<std::future<void> > > _futures;
  std::vector<std::shared_ptr<bool> > _runnings;

  void start() { this->_last_pending_job_cnt = pending_job_cnt(); increase_thread(); }
  bool increase_thread(){
    if(this->_thread_cnt >= _max_thread_cnt) return false;
    _runnings.push_back(std::make_shared<bool>(true));
    auto future = std::async(std::launch::async, _thread_func, _runnings.back());
    _futures.push_back(std::make_shared<std::future<void> >(std::move(future)));
    _thread_cnt++;
    return true;
  }
  bool decrease_thread(){
    if(_thread_cnt <= 1) return false;
    *(_runnings.back()) = false;
    _futures.pop_back();
    _runnings.pop_back();
    _thread_cnt--;
    return true;
  }
  int pending_job_cnt() { return _count_func(); }
  std::string _name;
  int _last_pending_job_cnt = 0;
  int _thread_cnt = 0;
  
  int _max_thread_cnt = 8;
  int _balance_speed_threshold = 100;
  int _balance_max_quantity_threshold = 1000;
  int _balance_min_quantity_threshold = 100;
};

struct TaskManager
{
  std::thread::id _thread_id;
  TaskManager(){
    _thread_id = std::this_thread::get_id();
  }
  void dump(){
    //FORBIDDEN_MULTI_THREAD("TaskManager::dump");
    int i = 0;
    for(auto & a : _tasks){
      g_print("\nthread idx: %d, thread_cnt: %d\n", i++, a._thread_cnt);
    }
  }
  void add(std::function<int()> count_func,  std::function<void(std::shared_ptr<bool>)> thread_func)
  {
    FORBIDDEN_MULTI_THREAD("TaskManager::add");
    _tasks.push_back(TaskContext(count_func, thread_func));
    //_tasks.emplace_back(count_func, thread_func);
  }
  void run(std::shared_ptr<bool> running){
    FORBIDDEN_MULTI_THREAD("TaskManager::run");
    
    for(auto & a : _tasks) a.start();

    typedef typename decltype(_tasks)::value_type task_t;
    auto balance = [this](task_t & task){
      int curr_pending_job_cnt = task.pending_job_cnt();
      int distance = curr_pending_job_cnt - task._last_pending_job_cnt;
      int speed = std::abs(distance)/_checkpoint_interval;
      
      bool done = false;
      auto make_done = [&](function<bool()> func) {
	if(!done && func()) done = true; };
      auto increase = [&]{ make_done([&]{ return task.increase_thread(); }); };
      auto decrease = [&]{ make_done([&]{ return task.decrease_thread(); }); };
      // auto increase = [&](){ if(!done && task.increase_thread()) done = true; };
      // auto decrease = [&](){ if(!done && task.decrease_thread()) done = true; };
      
      if(speed > task._balance_speed_threshold ) {
	if(distance > 0) increase();
	if(distance < 0) decrease();
      }
      
      if(curr_pending_job_cnt > task._balance_max_quantity_threshold) 
	increase();
      if(curr_pending_job_cnt < task._balance_min_quantity_threshold) 
	decrease();
      
      task._last_pending_job_cnt = curr_pending_job_cnt;
    };
    
    while(*running) {
      for(auto & task : _tasks) balance(task); 
      sleep(_checkpoint_interval);
    }
  }
  std::vector<TaskContext> _tasks;
  int _checkpoint_interval = 8;
};

/*************************************************************************************

int main()
  {
    auto running  = make_shared<bool>(true);
    auto tick_future = std::async(std::launch::async, &Tick::run, Tick::instance(), running);

    auto q1 = make_shared<threadsafe_queue<std::string>>();
    auto q2 = make_shared<threadsafe_queue<std::string>>();

    auto gen = std::async(std::launch::async, [&]{ 
	static int i = 0;
	while(*running && i < 100*1000){
	//q1->push((boost::format("*{%1%}") % i++).str());
	//q2->push((boost::format("-{%1%}") % i++).str());
	  q1->push("*");
	  q2->push("-");
	  i++;
	  auto a = [&](){
	    q1->push((boost::format("*{%1%}") % i++).str());
	    q2->push((boost::format("-{%1%}") % i++).str());
	  }; 
	  timer_exec(3, a());
	  usleep(20*1000);
	}
      });
    
    auto c1 = [&](){ return q1->size(); };
    auto c2 = [&](){ return q2->size(); };

    auto f1 = [&](shared_ptr<bool> running){
      while(*running) { 
	auto a = q1->try_pop(); 
	if(a) dot(*a);
	usleep(80*1000); 
      }
    };
    int spd = 100;
    int direction = -1;
    auto f2 = [&](shared_ptr<bool> running){
      while(*running){ 
	auto a = q2->try_pop(); 
	if(a) dot(*a); 
	usleep((spd > 0 ? spd : 1)*1000); 
      }
    };
    
    TaskManager mgr;
    mgr.add(c1, f1);
    mgr.add(c2, f2);
    
    auto timer = std::async(std::launch::async, [&](){
	while(*running){
	  timer_exec(3, mgr.dump());
	  auto f = [&](){
	    spd += 10*direction;
	    if(spd < 0) direction *=  -1;
	    if(spd > 100) direction *=  -1;
	  };
	  timer_exec(3, f());
	  sleep(1);
	}
      }); 
    
    std::cout << "--------001\n";
    auto a = std::async(std::launch::async, [&]{ sleep(240); *running = false; });

    std::cout << "--------002\n";
    mgr.run(running);
    std::cout << "--------003\n";
    return 0;
  }

*************************************************************************************/

#endif
