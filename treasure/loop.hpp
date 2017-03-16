#ifndef __TREASURE_LOOP_HPP__
#define __TREASURE_LOOP_HPP__

struct Event{
  Event(std::string name, std::string content) 
    : _name(name), _content(content) {}
  std::string name() { return _name; }
  std::string content() { return _content; }
private:
  std::string _name;
  std::string _type;
  int _priority;
  std::string _content;
};

struct EventHandler
{
  typedef std::shared_ptr<Event> evt_ptr; 
  typedef std::function<void(evt_ptr)> func_t;
  EventHandler(std::string name, func_t func)
    : _evt_name(name), _handler(func) {}
  void operator()(evt_ptr evt) { if(_handler) _handler(evt); }
  std::string event_name() { return _evt_name; } 
private:
  std::string _evt_name;
  func_t _handler;
};

struct EventLoop : Thread<EventLoop>
{
  EventLoop(){
  }
  void run(std::shared_ptr<bool> running){
    while(*running){
      auto pevt = _events.try_pop(); 
      if(!pevt) continue;
      auto evt = *pevt;
      if(!evt) continue;
      read_lock_guard lk(_hdr_mtx);
      if(_handlers.find(evt->name()) != _handlers.end())
	(*(_handlers[evt->name()]))(evt);
      else { 
	less_dot( (boost::format("warning: event %1% has not been registered") % evt->name()).str() ); 
      }
    }
  }
  void trigger_event(std::shared_ptr<Event> evt){
    _events.push(evt);
  }
  void register_event_handler(std::shared_ptr<EventHandler> hdr){
    write_lock_guard lk(_hdr_mtx);
    _handlers[hdr->event_name()] = hdr;
  }
  size_t pending_events_count(){ return _events.size(); }
private:
  threadsafe_queue<std::shared_ptr<Event> > _events;
  std::map<std::string, std::shared_ptr<EventHandler> > _handlers;
  read_write_lock _hdr_mtx;
};

/*************************************************************************************************************************
struct EventLoop
{
  EventLoop(){
    _this_thread_id = std::this_thread::get_id();
    _register_forbidden = false;
  }
  void run_loop(volatile bool & running){
    
    {
      std::lock_guard<std::mutex> lock(_mtx);
      _register_forbidden = true;
    }
    //sleep(1); // sleep to avoid the concurrent visit of _handlers;
    if(std::this_thread::get_id() != _this_thread_id){
      g_warning("run_loop failed: you can't run loop in others thread."); return;
    }
    while(running){
      auto pevt = _events.try_pop(); 
      if(!pevt) continue;
      auto evt = *pevt;
      if(evt && _handlers.find(evt->name()) != _handlers.end())
	(*(_handlers[evt->name()]))(evt);
    }
  }
  void trigger_event(std::shared_ptr<Event> evt){
    _events.push(evt);
  }
  void register_event_handler(std::shared_ptr<EventHandler> hdr){
    std::lock_guard<std::mutex> lock(_mtx);
    if(_register_forbidden) {
      g_warning("register_event_handler failed: loop has run, you can't do register when loop is running");
      return;
    }
    if(std::this_thread::get_id() == _this_thread_id){
      _handlers[hdr->event_name()] = hdr;
    } else g_warning("register_event_handler failed: you can't register event handler in others thread.");
  }
private:
  threadsafe_queue<std::shared_ptr<Event> > _events;
  std::map<std::string, std::shared_ptr<EventHandler> > _handlers;
  std::thread::id _this_thread_id;
  bool _register_forbidden;
  std::mutex _mtx;
};

void event_hander1(std::shared_ptr<Event> evt){
  //g_print("ok, event1 %s (%s) has been handled!\n", evt->name().c_str(), evt->content().c_str());
}

void event_hander2(std::shared_ptr<Event> evt){
  //g_print("ok, event2 %s (%s) has been handled!\n", evt->name().c_str(), evt->content().c_str());
}

int main()
{
    auto running = std::make_shared<bool>(true);
    auto f1 = std::async(std::launch::async, [&]{ sleep(15); *running = false; });
    
    EventLoop loop;
    loop.register_event_handler(std::make_shared<EventHandler>("sell", &event_hander1));
    loop.register_event_handler(std::make_shared<EventHandler>("pay", &event_hander1));
    loop.register_event_handler(std::make_shared<EventHandler>("scanner_status", &event_hander2));
    loop.register_event_handler(std::make_shared<EventHandler>("printer_status", &event_hander2));

    auto f2 = std::async(std::launch::async, [&loop]{ 
	for(int i=0; i<100000; ++i){
	  loop.trigger_event(std::make_shared<Event>("sell", std::to_string(i) + "th event!!!")); 
	  //usleep(1*1000);
	}
	for(int i=0; i<100000; ++i) {
	  loop.trigger_event(std::make_shared<Event>("scanner_status", std::to_string(i) + "th event!!!")); 
	  //usleep(1*1000);
	}
	g_print("done!");
      });
    loop.run(running);
    
    return 0;
}
**************************************************************************************************************************/



#endif
