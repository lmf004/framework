#include "headers.hpp"
#include "rpc.hpp"

namespace j_rpc {

  namespace protocol {
    
    shared_ptr<raw_buf> 
    generate_worker_info_request(std::vector<std::string> const & roles)
    {
      scope();
      Head head("worker_info");
      WorkerInfoReq req; req.a.d = roles;
      return utils::generate_request(head, req);
    }
  
    std::map<std::string, ip_port_t> 
    parse_worker_info_response(shared_ptr<raw_buf> buf)
    {
      scope();
      std::map<std::string, ip_port_t> m;
      Head head; WorkerInfoRsp rsp;
      if(SUCC != utils::parse_response(buf, head, rsp)) return m;
      for(auto & item : rsp.a.d){
	m[item.first] = std::pair<std::string, int>(item.second.ip, item.second.port);
      }
      return m;
    }

    bool is_rsp_succ(shared_ptr<raw_buf> rsp_buf)
    {
      scope();
      Head head; 
      if(SUCC == utils::extract_head(rsp_buf, head))
	return 0 == head.retcode;
      return false;
    }
    
  } // namespace protocol

  namespace utils {
    
    using namespace protocol;
    int split_head_body(char* buf, Head & head, std::string & body) {
      scope();
      int len_size = 2;
      int len = *(unsigned short*)buf;
      if(buf[len] != 0x00) buf[len] = 0x00;
    
      std::vector<std::string> v;
      std::string str((char*)buf+len_size);
      boost::split(v, str, boost::is_any_of("?"));
      if(v.size() != 2) { g_warning("split_head_body failed! @1"); return 1; }
      if(v[0].empty())  { g_warning("split_head_body failed! @2"); return 2; }
      head.load(v[0]); body = v[1]; 
      return 0;
    }
    
    int split_head_body(shared_ptr<raw_buf> buf, Head & head, std::string & body) {
      return split_head_body(buf->data(), head, body);
    }
  
    int join_head_body(char* buf, int size, Head const & head, std::string body) {
      scope();
      std::string headstr; head.save(headstr);
      int len = 2 + headstr.length() + 1 + body.length() + 1;
      if(size < len) return 1;
      *(unsigned short *)buf = len;
      snprintf((char*)buf+2, len-2, "%s?%s", headstr.c_str(), body.c_str());
      buf[len-1] = 0x00;
      return 0;
    }
    
    int join_head_body(shared_ptr<raw_buf> buf, Head const & head, std::string body) {
      return join_head_body(buf->data(), buf->size(), head, body);
    }  
  
    int extract_head(shared_ptr<raw_buf> buf, Head & head) {
      std::string body;
      return utils::split_head_body(buf, head, body);
    }
    
  } // namespace utils

  int Role::do_register(std::string const & ip, int port){
    scope();
    using namespace protocol;
    auto running = make_shared<bool>(true);
    SyncClient cli(ip, port, "dispatcher");

    auto f = cli.start(running);
    
    dot("0");

    wait(cli.role()+"connect", 0, running);
    
    dot("1");

    RoleIntroduceReq req; RoleIntroduceRsp rsp; 
    req._role = _role_name; req._ip = _ip; req._port = _port;
    
    dot("2");

    auto call = [&](){ 
      sub_scope("call");
      return operation::remote_call("role_introduce", cli, req, rsp, running); 
    };
    
    REPEAT(call(), 3000);

    dot("3");
    
    cli.stop();
    
    dot("4");
    return 0;
  }

  std::map<std::string, ip_port_t>
  ClientManager::get_role_ip_from_dispatcher(std::vector<std::string> const & roles, shared_ptr<bool> running)
  {
    scope();
    std::string dispatcher_ip; int dispatcher_port;
    Manager::get_dispatcher_ip_port(dispatcher_ip, dispatcher_port);

    SyncClient cli(dispatcher_ip, dispatcher_port, "dispatcher");
    auto f = cli.start(running);
    wait(cli.role()+"connect", 0, running);
    shared_ptr<raw_buf> rsp_buf;
    auto worker_info = [&](){
      sub_scope("worker_info");
      REPEAT(cli.send_msg(protocol::generate_worker_info_request(roles)), 3000);
      if(0 != timed_wait(cli.role()+"worker_info", 0, 3000, rsp_buf, running)) 
	return 1;
      dot("1");
      loghex("worker_info: ", rsp_buf->data());
      if(!protocol::is_rsp_succ(rsp_buf)) 
	return 2;
      dot("2");
      return 0;
    };
    REPEAT(worker_info(), 3000);
    cli.stop();
    return protocol::parse_worker_info_response(rsp_buf);
  }

  void Role::run(shared_ptr<bool> running) {
    scope();
    std::string dispatcher_ip; int dispatcher_port;
    Manager::get_dispatcher_ip_port(dispatcher_ip, dispatcher_port);
    do_register(dispatcher_ip, dispatcher_port);
    dot("h");
    threadsafe_queue<std::shared_ptr<Message<> > > queue;
    Server<> serv(_port);
    auto fs = serv.start(running, std::ref(queue));
    while(*running) {
      auto p = queue.try_pop();
      if(!p) continue;
      auto msg = *p;
      loghex("role received msg: ", msg->_msg->data());
      auto buf = handle_request(msg->_msg);
      if(buf) msg->_src->send_msg(buf);
    }      
  }

  void Dispatcher::run(shared_ptr<bool> running) {
    scope();
    std::string dispatcher_ip; int dispatcher_port;
    Manager::get_dispatcher_ip_port(dispatcher_ip, dispatcher_port);
    
    threadsafe_queue<std::shared_ptr<Message<> > > queue;
    Server<> serv(dispatcher_port);
    auto fs = serv.start(running, std::ref(queue));
    while(*running) {
      auto p = queue.timed_pop();
      less_dot("1");
      if(!p) continue;
      auto msg = *p;
      dot("2");
      loghex("dispatcher received: ", msg->_msg->data());
      auto buf = handle_request(msg->_msg);
      dot("3");
      if(buf) msg->_src->send_msg(buf);
    }      
  }

  raw_buf_ptr Dispatcher::handle_request(raw_buf_ptr buf) { 
    scope();
    using namespace protocol;
    Head head; std::string body;
    if(SUCC != utils::split_head_body(buf, head, body))
      return shared_ptr<raw_buf>();
    
    auto workinfo_hdr = [this](Head const& req_head, Head & rsp_head, WorkerInfoReq const & req, WorkerInfoRsp & rsp){
      sub_scope("workinfo_hdr");
      rsp_head = req_head;
      for(std::string const & role : req.a.d)
	if(_roles.find(role) != _roles.end())
	  rsp.a.d[role] = IpPort(_roles[role].first, _roles[role].second);
      return 0;
    };
    
    auto roleintroduce_hdr = [this](Head const& req_head, Head & rsp_head, RoleIntroduceReq const & req, RoleIntroduceRsp & rsp){
      sub_scope("roleintroduce_hdr");
      rsp_head = req_head;
      _roles[req._role] = ip_port_t(req._ip, req._port);
      return 0;
    };
      
    if(head.func == "worker_info") {
      return utils::handle_reqeust<WorkerInfoReq, WorkerInfoRsp>(head, body, workinfo_hdr); }
      
    if(head.func == "role_introduce") {
      return utils::handle_reqeust<RoleIntroduceReq, RoleIntroduceRsp>(head, body, roleintroduce_hdr); }

    return shared_ptr<raw_buf>();
  }
  
} // namespace j_rpc
