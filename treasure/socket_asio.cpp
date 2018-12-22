#include "headers.hpp"

allocator<chat_message> chat_message_allocator;
allocator<tcp_connection> tcp_connection_allocator;
allocator<tcp_client> tcp_client_allocator;

void print_new_cnt(boost::asio::deadline_timer & d, boost::system::error_code const & error){
  BOOST_LOG_TRIVIAL(trace) << "print_new_cnt " << (bool)error;
  if(!error){
    PRINT_NEW_CNT;
    d.expires_from_now(boost::posix_time::seconds(5));
    d.async_wait(boost::bind(&print_new_cnt, boost::ref(d), _1));
  } else { BOOST_LOG_TRIVIAL(warning) << "print_new_cnt " << error.message(); }
}
