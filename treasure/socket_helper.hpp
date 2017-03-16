#ifndef __SOCKET_HELPER_HPP__
#define __SOCKET_HELPER_HPP__

#define MAX_MSG_SIZE 256

// typedef gpointer (*pfn_accept_handler)(int connfd, gpointer);
// typedef void (*pfn_recv_handler)(int connfd, gpointer ptr, guint8* buffer, bool succ, gpointer);

struct Socket_Helper
{
  static int  send_buffer(int sock, unsigned char* buffer, int buflen, gboolean * shutdown);
  static int  recv_buffer(int sock, unsigned char* buffer, int buflen, gboolean * shutdown);
  static bool recv_msg(int sock, unsigned char* buffer, int buflen, gboolean * shutdown);
  static void set_non_blocking(int sock);
  static int  accept(int sock);
  static int  listen(int port);
  static int  connect(const char * serverip, const int serverport);
  static int  epoll_create(int sock, struct epoll_event** epoll_events, int client_cnt);
  static int  epoll_wait(int epfd, struct epoll_event* events, int client_cnt);
  static void epoll_add(int epfd, int connfd, void* ptr);
  static void epoll_loop(int port, int client_cnt, int& sock,
			 std::function<void*(int)> accept_handler, 
			 std::function<void(void*, std::shared_ptr<raw_buf>, bool)> recv_handler,
			 std::function<int(void*)> get_fd_handler,
			 std::function<void(void*)> del_handler,
			 std::shared_ptr<bool> running
  );

};


#endif
