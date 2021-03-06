#ifndef __TREASURE_RPC_HPP__
#define __TREASURE_RPC_HPP__

enum { TYPE_CLIENT, TYPE_WORKER, TYPE_DISPATCHER };

#define SUCC 0

namespace j_rpc {

  typedef std::pair<std::string, int>  ip_port_t;
  typedef shared_ptr<raw_buf> raw_buf_ptr;

  namespace protocol {
    
    struct IpPort : Serializable<',', IpPort>
    {
      IpPort(){}
      IpPort(std::string const & ip, int port)
	: ip(ip), port(port) 
      {}
      std::string ip; int port; SERIALIZE_MEMEBER(ip, port)
    };


    struct Head : Serializable<',', Head>
    {
      Head(std::string const & func) 
	: func(func), type(0), seq(0), retcode(0) 
      {}
      Head() 
	: type(0), seq(0), retcode(0) 
      {}
      std::string func;
      int type;
      int seq;
      int retcode;
      SERIALIZE_MEMEBER(func, type, seq, retcode)
      void dump(){
	g_print("Head: \n");
	g_print("  func:%s\n", func.c_str());
	g_print("  type:%d\n", type);
	g_print("  seq:%d\n", seq);
	g_print("  retcode:%d\n", retcode);
      }
    };

    struct NouseRsp : Serializable<' ', NouseRsp>
    {
      int a; SERIALIZE_MEMEBER(a)
    };
    
    struct WorkerInfoReq : Serializable<' ',  WorkerInfoReq>
    {
      Vector<std::string, ','> a; SERIALIZE_MEMEBER(a)
    };

    struct WorkerInfoRsp : Serializable<' ',  WorkerInfoRsp>
    {
      Map<std::string, IpPort, '/', ':'> a; SERIALIZE_MEMEBER(a)
    };

    struct RoleIntroduceReq : Serializable<' ', RoleIntroduceReq>
    {
      std::string _role;
      std::string _ip;
      int _port;
      SERIALIZE_MEMEBER(_role, _ip, _port)
    };
    
    struct RoleIntroduceRsp : Serializable<' ', RoleIntroduceRsp>
    {
      std::string _role;
      std::string _ip;
      int _port;
      SERIALIZE_MEMEBER(_role, _ip, _port)
    };


    shared_ptr<raw_buf> 
    generate_worker_info_request(std::vector<std::string> const & roles);
  
    std::map<std::string, ip_port_t> 
    parse_worker_info_response(shared_ptr<raw_buf> buf);

    bool is_rsp_succ(shared_ptr<raw_buf> rsp_buf);
    
  } // namespace protocol
  
  namespace utils {
    using namespace protocol;
    
    int split_head_body(char* buf, Head & head, std::string & body);
    int split_head_body(shared_ptr<raw_buf> buf, Head & head, std::string & body);
    int join_head_body(char* buf, int size, Head const & head, std::string body);
    int join_head_body(shared_ptr<raw_buf> buf, Head const & head, std::string body);
    
    template<typename Req, typename Rsp, typename Func>
    shared_ptr<raw_buf> handle_reqeust(Head const & head, std::string const & body, Func func)
    {
      scope();
      Req req; req.load(body); Rsp rsp; 
      Head head_rsp;
      if(SUCC != func(head, head_rsp, req, rsp))
	return shared_ptr<raw_buf>();
      dot("a");
      if(std::is_same<Rsp, NouseRsp>::value)
	return shared_ptr<raw_buf>();
      dot("b");
      std::string rsp_body;
      rsp.save(rsp_body);
      auto buf = make_shared<raw_buf>();
      if(SUCC != utils::join_head_body(buf, head_rsp, rsp_body))
	return shared_ptr<raw_buf>();
      dot("c");
      return buf;
    }
    template<typename Req, typename Rsp, typename Func>
    shared_ptr<raw_buf> handle_reqeust(shared_ptr<raw_buf> buf, Func func)
    {
      scope();
      Head head; std::string body;
      if(SUCC != utils::split_head_body(buf, head, body)) 
	return shared_ptr<raw_buf>();
      return handle_reqeust(head, body, func);
    }  
  
    template<class Req>
    shared_ptr<raw_buf> generate_request(Head const & head, Req const & req){
      scope();
      auto buf = make_shared<raw_buf>();
      std::string body; 
      req.save(body);
      if(SUCC == utils::join_head_body(buf, head, body)){
	return buf;
      }
      return shared_ptr<raw_buf>();
    }
  
    template<class Rsp>
    int parse_response(shared_ptr<raw_buf> buf, Head & head, Rsp & rsp)
    {
      scope();
      std::string body;
      if(SUCC != utils::split_head_body(buf, head, body)) 
	return 1;
      rsp.load(body);
      return SUCC;
    }
  
    int extract_head(shared_ptr<raw_buf> buf, Head & head);

  } // namespace utils

  struct AsyncClient: Client<AsyncClient>, std::enable_shared_from_this<AsyncClient> 
  {
    typedef Message<AsyncClient> msg_t;
    typedef shared_ptr<msg_t> msg_ptr;
    AsyncClient(){}
    AsyncClient(std::string ip, int port, std::string role, shared_ptr<threadsafe_queue<msg_ptr> > queue)
    {
      set_ip_port(ip, port);
      _role = role;
      _queue = queue;
    }
    void notify(int evt) {
      if(evt == EVT_CONNECTED) ::notify(_role+"connect", 0);
    }
    int handle_received_message(shared_ptr<raw_buf> buf)
    {
      scope();
      _queue->push(make_shared<msg_t>(shared_from_this(), buf));
      return 0;
    }
    std::string role() { return _role; }
    shared_ptr<threadsafe_queue<msg_ptr> > queue() { return _queue; }
  private:
    std::string _role;
    shared_ptr<threadsafe_queue<msg_ptr> > _queue;
  };
  
  struct SyncClient : Client<SyncClient>
  {
    SyncClient(){}
    SyncClient(std::string ip, int port, std::string role)
    {
      set_ip_port(ip, port);
      _role = role;
    }
    void notify(int evt) {
      if(evt == EVT_CONNECTED) ::notify(_role+"connect", 0);
    }
    int handle_received_message(shared_ptr<raw_buf> buf)
    {
      scope();
      using namespace protocol;
      Head head; 
      if(SUCC != utils::extract_head(buf, head)) 
	return 1;
      ::notify(_role+head.func, head.seq, buf);
      return 0;
    }
    std::string role() { return _role; }
  private:
    std::string _role;
  };

  struct Role : Thread<Role>
  {
    typedef std::function<raw_buf_ptr(protocol::Head const &, std::string const &)> request_hander_t;

    Role(std::string const & role_name, request_hander_t hdr, std::string const & ip, int port) 
      : _role_name(role_name), _request_handler(hdr), _ip(ip), _port(port)
    {}
    
    void run(shared_ptr<bool> running);
    int do_register(std::string const & ip, int port);

    raw_buf_ptr handle_request(raw_buf_ptr buf) { 
      scope();
      using namespace protocol;
      Head head; std::string body;
      if(SUCC != utils::split_head_body(buf, head, body))
	return shared_ptr<raw_buf>();
      return _request_handler(head, body); 
    }
    
    void set_ip_port(std::string ip, int port) {
      _ip = ip; _port = port;
    }
    void get_ip_port(std::string & ip, int & port) {
      ip = _ip; port = _port;
    }
  private:
    std::string _ip; int _port;
    std::string _role_name;
    request_hander_t _request_handler;
  };

  struct ClientVector
  {
    static ClientVector & instance() { static ClientVector a; return a; }
    std::map<std::string, ip_port_t> _roleips;
    std::map<std::string, std::future<void> > _cliFutures;
    std::map<std::string, shared_ptr<SyncClient> > _roleClients;   
  
    shared_ptr<SyncClient> find_client_for_role(std::string const & role){
      scope();
      if(_roleClients.find(role) != _roleClients.end())
	return _roleClients[role];
      return shared_ptr<SyncClient>();
    }
  };
  
  namespace operation
  {
    using namespace protocol;
    using namespace utils;
    
    template<typename Req, typename Rsp>
    int remote_call(Head & head_req, Head & head_rsp, SyncClient & cli, Req const & req, Rsp & rsp, shared_ptr<bool> running)
    {
      scope();
      static std::atomic_int gseq(0);
      int seq = gseq++;
    
      {
	head_req.seq = seq;
	auto buf = utils::generate_request(head_req, req);
	if(!buf) return 1;
	cli.send_msg(buf);
      }
      dot("1");
      {
	auto buf = wait(cli.role()+head_req.func, seq, running);
	if(SUCC != utils::parse_response(buf, head_rsp, rsp)) return 2;
      }
    
      return 0;
    }
  
    template<typename Req, typename Rsp>
    int remote_call(std::string func, SyncClient & cli, Req const & req, Rsp & rsp, shared_ptr<bool> running)
    {
      Head head_req(func), head_rsp;
      return remote_call(head_req, head_rsp, cli, req, rsp, running);
    }
    template<typename Req, typename Rsp>
    int remote_call(std::string const & role, std::string const & service, Req const & req, Rsp & rsp, shared_ptr<bool> running)
    {
      auto cli = ClientVector::instance().find_client_for_role(role);
      if(!cli) return 3;
      return remote_call(service, *cli, req, rsp, running);
    }
  } // namespace operation

  struct ClientManager
  {
    static ClientManager & instance() { static ClientManager a; return a; }
    void request_worker_info(std::vector<std::string> const & roles, shared_ptr<bool> running)
    {
      scope();
      _climgr._roleips = get_role_ip_from_dispatcher(roles, running);
      _climgr._roleClients = create_rpcclients_of_workers(_climgr._roleips, _climgr._cliFutures, running);
    }
  
  private:
    static std::map<std::string, shared_ptr<SyncClient> >
    create_rpcclients_of_workers(
      std::map<std::string, ip_port_t> const & roleips
      , std::map<std::string, std::future<void> > & cliFutures, shared_ptr<bool> running)
    {
      scope();
      std::map<std::string, shared_ptr<SyncClient> > roleClients;
      for(auto roleip : roleips) {
	auto cli = make_shared<SyncClient>(roleip.second.first, roleip.second.second, roleip.first);
	cliFutures[roleip.first] = std::move(cli->start(running));
	roleClients[roleip.first] = cli;
      }
      return roleClients;
    }
  
    static std::map<std::string, ip_port_t>
    get_role_ip_from_dispatcher(std::vector<std::string> const & roles, shared_ptr<bool> running);
    
    ClientVector _climgr;
  };

  struct Dispatcher : Thread<Dispatcher>
  {
    void run(shared_ptr<bool> running);
  private:
    raw_buf_ptr handle_request(raw_buf_ptr buf);
    std::map<std::string, ip_port_t> _roles;
  };

  struct Manager
  {
    static int get_dispatcher_ip_port(std::string & ip, int & port){
      ip = "127.0.0.1"; port = 60001; return 0;
    }
  };
} // namespace J_RPC


#endif

/**********************************************************************************************

namespace GuiServer
{
  //using namespace j_rpc;
  using j_rpc::protocol::Head;
  using j_rpc::protocol::NouseRsp;
  struct SellTicketReq : Serializable<'&', SellTicketReq>
  {
    int game;
    int subtype;
    int bettype;
    std::string balls;
    SERIALIZE_MEMEBER(game, subtype, bettype, balls)
  };
  struct SellTicketRsp : Serializable<'&', SellTicketRsp>
  {
    int game;
    int subtype;
    int bettype;
    std::string balls;
    SERIALIZE_MEMEBER(game, subtype, bettype, balls)
  };
  struct PrinterErrorNotifyReq : Serializable<'&', PrinterErrorNotifyReq>
  {
    int errorcode;
    SERIALIZE_MEMEBER(errorcode)
  };
  
  int handle_sell_ticket(Head const& req_head, Head & rsp_head, SellTicketReq const & req, SellTicketRsp & rsp)
  {
    scope();
    rsp_head = req_head;
    rsp.game = req.game;
    rsp.subtype = req.subtype;
    rsp.bettype = req.bettype;
    rsp.balls = req.balls;
  }
  int handle_printer_error_notify(Head const& req_head, Head & rsp_head, PrinterErrorNotifyReq const & req, NouseRsp & rsp)
  {
    
  }
  
  shared_ptr<raw_buf>
  handle_request(Head const & head, std::string const & body)
  {
    scope();
    using namespace j_rpc;
    if(head.func == "print_sell_ticket") 
      return utils::handle_reqeust<SellTicketReq, SellTicketRsp>(head, body, handle_sell_ticket);
    if(head.func == "printer_error_notify") 
      return utils::handle_reqeust<PrinterErrorNotifyReq, NouseRsp>(head, body, handle_printer_error_notify);
    return shared_ptr<raw_buf>();
  }
}

int main(int argc, char* argv[])
{
  if(argc != 2) return 1;
  int id = atoi(argv[1]);
  auto running = make_shared<bool>(true);
  
  if(id == 0) {
    j_rpc::Dispatcher dispatcher; 
    dispatcher.run(running);
  }
  
  if(id == 1) {
    j_rpc::Role role("caipiao", GuiServer::handle_request, "127.0.0.1", 60002);
    role.run(running);
  }
  
  if(id == 2) {
    using namespace GuiServer;
    j_rpc::ClientManager::instance().request_worker_info({"caipiao", "printer", "scanner", "ncp"}, running);
    SellTicketReq ticketReq;
    SellTicketRsp ticketRsp;
    j_rpc::operation::remote_call("caipiao", "print_sell_ticket", ticketReq, ticketRsp, running);
  }
}

#if 0

struct role_info {
  string name;
  string ip; // 192.168.26.1:60001
};

struct dispatcher {
  void handle_received_message(msg){
    func = extract_func(msg);
    if(func == "role_info_register"){
      role_info info =  extract_body(msg);
      role_ips[info.name] = info.ip;
    }
    if(func == "role_info_inquery"){
      vector<string> roles = extract_body(msg);
      foreach(auto role : roles)
	rsp += (role + '@' + role_ips[role]) +'/';
      send_msg_2_client(rsp);
    }
  }
  map<string, string> role_ips;
};

do_sell(head1, head2, req, rsp){
  
}

struct Role{
  void handle_received_message(msg){
    head, body = extract_head_body(msg);
    rsp = handle_service_request(head, body);
    if(!rsp.empty()) send_msg_2_client(rsp);
  }
};

template<ReqType, RspType, Func>
do_service(Head const &head, string const & body, Func func){
  Head head2, string body2, ReqType req, RspType rsp;
  req.decode(body);
  func(head, head2, req, rsp);
  if(!is_same<RspType, no_rsp>)
    body2 = rsp.encode();
  return body2;
}
struct LotteryRole : Role{
  virtual handle_service_request(head, body){
    if(head.func == "sell") return do_service<SellReq, SellRsp>(head, body, do_sell);
    //...
  }
};

func(){
  Client cli;
  cli.connect("127.0.0.1", 60001);
  role_info_inquery msg("lottery,printer");
}
// 
// notify, sync, async

// queue: queue can hold much messages

// dispatcher, role, client
// serialize /sell?ssq&1,2,3,4,5,7&200
// client: /sell?ssq&1,2,3,4,5,7&200
// server: /sell?0&01020304050607080910
// role: lottery: (sell: service(SellReq => SellRsp)), (cancel: service(CancelReq => CancelRsp))
// role: register(lottery, [sell, cancel...]) => dispatcher  
// dispatcher: [(lottery: sell, cancel, payout)+(ip,port),  (printer: print, status)+(ip,port)]
// dispatcher: role_info([(lottery: 192.168.23.1,60001), (printer: 127.0.0.1,60002) ...]) => client
// client: role_info([lottery, printer...]) => dispatcher

#endif

**********************************************************************************************/


