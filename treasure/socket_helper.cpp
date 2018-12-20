#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/param.h> 
#include <time.h>
#include <sys/time.h>
#include <iconv.h>
#include <ctype.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "socket_helper.hpp"

int SocketHelper::send_buffer(int sock, unsigned char* buffer, int buflen, gboolean * shutdown)
{
  if(shutdown) *shutdown = FALSE;
  int left = buflen;
  int total = 0;
  while(left > 0){
    ssize_t cnt = ::send(sock, buffer+total, left, 0);
    if(0 == cnt) { if(shutdown) *shutdown = TRUE; return total; }
    if(cnt < 0)
      {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
          { return total; }
        else
          { if(shutdown) *shutdown = TRUE; return total; }
      }
    if(cnt > 0)  { left -= cnt; total += cnt; }
  }
  return total;
}

bool SocketHelper::recv_msg(int sock, unsigned char* buffer, int buflen, gboolean * shutdown)
{
  int total = 0;

  int cnt = SocketHelper::recv_buffer(sock, buffer, 2, shutdown);
  if(cnt <=0) return false;

  total += cnt;

  if(total != 2) return false;

  if(*(unsigned short*)buffer > buflen)
    {
      g_warning("too large message with %02x %02x", buffer[0], buffer[1]);
      if(shutdown) *shutdown = TRUE;
      return false;
    }

  if(*(unsigned short*)buffer <= 2)
    {
      g_warning("too small message");
      if(shutdown) *shutdown = TRUE;
      return false;
    }

  cnt = SocketHelper::recv_buffer(sock, buffer+2, *(unsigned short*)buffer - 2, shutdown);

  if(cnt <=0) return false;

  total += cnt;

  return total == *(unsigned short*)buffer;
}

int SocketHelper::recv_buffer(int sock, unsigned char* buffer, int buflen, gboolean * shutdown)
{
  if(shutdown) *shutdown = FALSE;
  int left = buflen;
  int total = 0;

  while(left > 0){
    ssize_t cnt = ::recv(sock, buffer+total, left, 0);
    if(0 == cnt) { if(shutdown) *shutdown = TRUE; return total; }
    if(cnt < 0)
      {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
          { return total; }
        else
          { if(shutdown) *shutdown = TRUE; return total; }
      }
    if(cnt > 0)  { left -= cnt; total += cnt; }
  }
  return total;
}

void SocketHelper::set_non_blocking(int sock)
{
  g_return_if_fail(sock > 0);

  int opts;
  opts=fcntl(sock,F_GETFL);
  if(opts<0)
    {
      perror("fcntl(sock,GETFL)");
      //exit(1);
    }
  opts = opts|O_NONBLOCK;
  if(fcntl(sock,F_SETFL,opts)<0)
    {
      perror("fcntl(sock,SETFL,opts)");
      //exit(1);
    }
}

int getpeermac(int sockfd, char *buf){ 
  int ret =0; 
  struct arpreq arpreq; 
  struct sockaddr_in dstadd_in; 

  socklen_t len = sizeof(struct sockaddr_in); 
  memset(&arpreq, 0, sizeof(struct arpreq)); 
  memset(&dstadd_in, 0, sizeof( struct sockaddr_in)); 

  if(getpeername(sockfd, (struct sockaddr*)&dstadd_in, &len) < 0) 
    perror("getpeername()"); 
  else { 
      memcpy( &arpreq.arp_pa, &dstadd_in, sizeof( struct sockaddr_in )); 
      strcpy(arpreq.arp_dev, "eth0"); 
      arpreq.arp_pa.sa_family = AF_INET; 
      arpreq.arp_ha.sa_family = AF_UNSPEC; 
      if(ioctl(sockfd, SIOCGARP, &arpreq) < 0) 
	perror("ioctl SIOCGARP"); 
      else { 
	  unsigned char* ptr = (unsigned char *)arpreq.arp_ha.sa_data; 
	  ret = sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5)); 
      } 
  } 
  return ret; 
} 

int SocketHelper::accept(int sock)
{
  struct sockaddr_in clientaddr;
  socklen_t length = sizeof(clientaddr);
  int connfd = ::accept(sock,(sockaddr *)&clientaddr, &length);
  if(connfd < 0)
    {
      perror("accept");
      g_usleep(1*1000*1000);
    }
  else if(connfd == 0)
    {
      g_warning("accept return 0");
      g_usleep(1*1000*1000);
    }
  else
    {
      // char buf[400]; memset(buf, 0, sizeof(buf));
      // if(getpeermac(connfd, buf))
      //   g_print("\nclient mac addr: %s\n", buf);
    }
  return connfd;
}

int SocketHelper::listen(int port)
{
  int listenfd = 0;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  int yes = 1;

  if((listenfd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror("socket"); goto ERROR;
    }

  if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt"); goto ERROR;
    }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

  if(::bind(listenfd, (struct sockaddr *)&(server_addr), sizeof(server_addr)) == -1)
    {
      perror("bind"); goto ERROR;
    }

  if(::listen(listenfd, 10000) == -1)
    {
      perror("listen"); goto ERROR;
    }
  return listenfd;

 ERROR:
  if(listenfd)
    {
      ::close(listenfd);
    }
  return 0;
}

int SocketHelper::connect(const char * serverip, const int serverport)
{
  int sock = 0;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t socklen = sizeof(servaddr);

  if((sock = ::socket(AF_INET,SOCK_STREAM,0)) < 0)
    { ::perror("socket"); g_usleep(1*1000*1000); return -1; }

  bzero(&cliaddr,sizeof(cliaddr));
  cliaddr.sin_family = AF_INET;
  cliaddr.sin_port = htons(0);
  cliaddr.sin_addr.s_addr = htons(INADDR_ANY);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  inet_aton(serverip, &servaddr.sin_addr);
  servaddr.sin_port = htons(serverport);

  if(::bind(sock,(struct sockaddr*)&cliaddr,sizeof(cliaddr))<0)
    { ::perror("bind"); ::close(sock); g_usleep(1*1000*1000); return -1; }

  if(::connect(sock,(struct sockaddr*)&servaddr, socklen) < 0)
    { ::perror("connect"); ::close(sock); g_usleep(1*1000*1000); return -1; }

  struct timeval timeout = {60, 0};		//超时时间为设置为30秒
  ::setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
  return sock;
}

int SocketHelper::epoll_create(int sock, struct epoll_event** epoll_events, int client_cnt)
{
  struct epoll_event ev;
  struct epoll_event * events = (struct epoll_event *)g_malloc0((client_cnt+1) * sizeof(struct epoll_event));

  g_return_val_if_fail(events, 0);

  int epfd = ::epoll_create(client_cnt+1);
  if(epfd <= 0)
    {
      g_error("epoll_create: %s", g_strerror(errno));
    }

  g_return_val_if_fail(epfd > 0, epfd);

  ev.data.fd=sock;
  ev.events=EPOLLIN;

  int ret = ::epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
  if(0 != ret)
    {
      g_error("epoll_ctl: %s", g_strerror(errno));
    }

  *epoll_events = events;

  return epfd;
}

int SocketHelper::epoll_wait(int epfd, struct epoll_event* events, int client_cnt)
{
  int nfds= ::epoll_wait(epfd, events, client_cnt+1, 1000);
  if(nfds < 0)
    {
      g_error("epoll_ctl: %s", g_strerror(errno));
    }
  return nfds;
}

void SocketHelper::epoll_add(int epfd, int connfd, void* ptr)
{
  struct epoll_event ev;

  if(ptr)
    ev.data.ptr = ptr;
  else
    ev.data.fd = connfd;
  ev.events = EPOLLIN;

  int ret = epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);

  if(ret < 0) { g_error("epoll_ctl: %s", g_strerror(errno)); }
  return;
}


void SocketHelper::epoll_loop(int port, int client_cnt, int& sock,
                               std::function<void*(int)> accept_handler, 
			       std::function<void(void*, std::shared_ptr<raw_buf>, bool)> recv_handler,
			       std::function<int(void*)> get_fd_handler,
			       std::function<void(void*)> del_handler,
			       std::shared_ptr<bool> running
)
{
  scope();
  struct epoll_event * events = NULL;
  sock = SocketHelper::listen(port);
  SocketHelper::set_non_blocking(sock);
  int epfd = SocketHelper::epoll_create(sock, &events, client_cnt);
  g_print("epoll_loop is ready for epoll_wait\n");
  while(*running) {
    int nfds =  SocketHelper::epoll_wait(epfd, events, client_cnt);
    for(int i=0; i<nfds; ++i) {
      if(sock == events[i].data.fd) {
	int connfd = SocketHelper::accept(sock);
	if(connfd > 0) {
	  SocketHelper::set_non_blocking(connfd);
	  void * p = accept_handler(connfd);
	  SocketHelper::epoll_add(epfd, connfd, p);
	}
      } else {
	void* p = events[i].data.ptr;
	int connfd = get_fd_handler(p);
	auto buf = std::make_shared<raw_buf>();
	bool succ = SocketHelper::recv_msg(connfd, (unsigned char*)buf->data(), buf->size(), NULL);
	if(!succ) { 
	  ::epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, 0); ::close(connfd); 
	  del_handler(p);
	  continue; 
	}
	recv_handler(p, buf, succ);
      }
    }
  }
  g_free(events);
}

