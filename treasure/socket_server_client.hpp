#ifndef __TREASURE_SOCKET_SERVER_CLIENT_HPP__
#define __TREASURE_SOCKET_SERVER_CLIENT_HPP__

struct EpollClient
{
  EpollClient(int fd) : _fd(fd) {}
  int send_msg(std::shared_ptr<raw_buf> buf){
    scope();
    if(_fd <= 0) { g_print("e1"); return 1; }
    std::lock_guard<std::mutex> lock(_send_mtx);
    auto p = buf->data();
    int cnt = SocketHelper::send_buffer(_fd, (unsigned char*)p, *(unsigned short*)p, NULL);
    if(cnt != *(unsigned short*)p) g_warning("send_msg failed");
    return cnt == *(unsigned short*)p ? 0 : 1;
  }
  void close(){ 
    if(_fd > 0) { ::close(_fd); _fd = 0; }
  }
  int fd() { return _fd; }
  int _fd;
  std::mutex _send_mtx;
};

template<class Client = EpollClient>
struct Message
{
  Message(shared_ptr<Client> src, std::shared_ptr<raw_buf> msg)
    : _src(src), _msg(msg)
  {}
  shared_ptr<Client> _src;
  std::shared_ptr<raw_buf> _msg;
};

template<class Client = EpollClient>
struct Server : Thread< Server<Client> >
{
  Server(int port) 
    : _port(port)
  {}
  
  void run(std::shared_ptr<bool> running, threadsafe_queue<std::shared_ptr<Message<Client> > > & queue){
    scope();
    auto accept_hdr = [](int fd){ 
      NEW; return new shared_ptr<Client>(new Client(fd));
    };
    
    auto recv_hdr = [&queue](void* p, std::shared_ptr<raw_buf> buf, bool succ) {
      sub_scope("recv_hdr");
      if(succ) 
	queue.push(std::make_shared<Message<Client> >(*(shared_ptr<Client>*)p, buf));
      g_print("queue's size=%d\n", (int)queue.size());
    };
    
    auto fd_hdr = [](void* p){
      return (*(shared_ptr<Client>*)p)->fd();
    };
    
    auto del_hdr = [](void* p){
      DEL; delete (shared_ptr<Client>*)p;
    };
    
    SocketHelper::epoll_loop(_port, 10000, _sock, accept_hdr, recv_hdr, fd_hdr, del_hdr, running);
  }    
private:
  int _sock;
  int _port;
};

enum{ EVT_CONNECTED = 1, EVT_DISCONNECTED, EVT_JOB_DONE };
template<typename Derived>
struct Client : Thread< Client<Derived> >
{
  void run(std::shared_ptr<bool> running){
    Derived * d = static_cast<Derived *>(this);
    g_print("Client::run...\n");
  START:
    _fd = 0;
    while(*running && (_fd = SocketHelper::connect(_ip.c_str(), _port)) <= 0)
      std::this_thread::sleep_for(std::chrono::seconds(1));
    if(_fd > 0) d->notify(EVT_CONNECTED);
    while(*running) {
      auto buf = std::make_shared<raw_buf>();
      if(SocketHelper::recv_msg(_fd, (unsigned char*)buf->data(), buf->size(), NULL)){
	int ret = d->handle_received_message(buf);
	if(ret == EVT_JOB_DONE) { close(); return; }
      }
      else { close(); d->notify(EVT_DISCONNECTED); goto START; }
    }
    close();
  }
  int send_msg(std::shared_ptr<raw_buf> buf){
    scope();
    if(_fd <= 0) { g_print("e1"); return 1; }
    std::lock_guard<std::mutex> lock(_send_mtx);
    auto p = buf->data();
    int cnt = SocketHelper::send_buffer(_fd, (unsigned char*)p, *(unsigned short*)p, NULL);
    if(cnt != *(unsigned short*)p) g_warning("send_msg failed");
    return cnt == *(unsigned short*)p ? 0 : 1;
  }
  void close(){ 
    if(_fd > 0) { ::close(_fd); _fd = 0; }
  }
  void set_ip_port(std::string ip, int port) { _ip = ip; _port = port; }
protected: 
  int _fd;
  std::mutex _send_mtx;
  std::string _ip;
  int _port;
};

/******************************************************************************************
struct Ncp_Client : Client<Ncp_Client>
{
  Ncp_Client(std::string ip, int port)
  {
    _ip = ip; _port = port;
  }
  void notify(int evt) {
    scope();
    if(evt == 0) dot("{");
    if(evt == 1) dot("}");
  }
  void handle_received_message(unsigned char * buf)
  {
    std::cout << "ok, ncp_client got a message\n";
  }
};

std::shared_ptr<char> make_msg(char const * msg)
{
  std::shared_ptr<char> sp( new char[strlen(msg)+2+1], std::default_delete<char[]>() );
  char * p = sp.get();
  *(unsigned short*)p = strlen(msg) + 2 + 1;
  strcpy(p+2, msg);
  return sp;
}

int main()
  {
    auto running = std::make_shared<bool>(true);
    threadsafe_queue<std::shared_ptr<Message>> queue;
    
    Server server(60001);
    auto server_future = server.start(running, std::ref(queue));
    
    g_print("wait for server to accept ...\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    Ncp_Client cli("127.0.0.1", 60001);
    auto client_future = cli.start(running);
    
    g_print("wait for client to connect server ...\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    auto a = std::async(std::launch::async, [&](){
	g_print("client start sending test message...\n");
	for(int i = 0; i < 10000; i++) {
	  cli.send_msg(make_msg("123456abcdefj"));
	  if(0 == i%10) std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
      });
    
    std::cout << "----------002\n";
    
    auto f = std::async(std::launch::async, [&]{ 
	std::this_thread::sleep_for(std::chrono::seconds(30));
	*running = false;
      });

    std::cout << "----------003\n";
    return 0;
  }
******************************************************************************************/

// struct message {
//   shared_ptr<RcpClient> cli;
//   raw_buf buf;
// };
// RpcClient* accept(int fd){
//   return new RpcClient(fd);
//   return new shared_ptr<RcpClient>(new RpcClient(fd));
// }
// epoll_add(epfd, 0, accept(fd));

// epoll_recv:
// RpcClient* cli = events[i].data.ptr;
// n = recv(); // cli.close(); when del?
// if(0 == n) delete (shared_ptr<RcpClient>*)p;
// message msg(*(shared_ptr<RcpClient>*)p);
// queue.push_back(msg);

// void hdr_queue_msg(Message msg)
// {
//   hdr_req(msg);
//   msg.cli.send_msg(rsp);
//   msg.cli.close();
//   msg.cli.del_self();
// }

//////////////////////////////////////////////////////////////////////////////////////////////////////

// struct ClientDesc {
//   ClientDesc(int fd) : _fd(fd) 
//   {}
//   int fd() { return _fd; }
// private:
//   int _fd;
// };

// struct ClientManager
// {
//   static ClientManager * get_instance() {
//     static ClientManager mgr; return &mgr;
//   }
//   void add(std::shared_ptr<ClientDesc> client)
//   {
//     scope();
//     std::lock_guard<std::mutex> lock(_mutex);
//     _map_fd_2_client[client->fd()] = client;
//   }
//   void remove(int fd){
//     scope();
//     std::lock_guard<std::mutex> lock(_mutex); 
//     if(_map_fd_2_client.find(fd) != _map_fd_2_client.end())
//       _map_fd_2_client.erase(fd);
//   }
//   std::shared_ptr<ClientDesc> get(int fd){
//     std::lock_guard<std::mutex> lock(_mutex); 
//     return _map_fd_2_client[fd];
//   }
// private:
//   ClientManager(){}
//   std::mutex _mutex;
//   std::map<int, std::shared_ptr<ClientDesc> > _map_fd_2_client;
// };

//////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
