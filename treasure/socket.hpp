#ifndef __TREASURE_SOCKET_HPP__
#define __TREASURE_SOCKET_HPP__

class chat_message;
class tcp_connection;
class tcp_client;

extern allocator<chat_message> chat_message_allocator;
extern allocator<tcp_connection> tcp_connection_allocator;
extern allocator<tcp_client> tcp_client_allocator;

class chat_message {
public:
  enum { header_length = 4 };
  enum { max_body_length = 512 };
  chat_message(): body_length_(0){ NEW; }
  chat_message(chat_message const& rhs){
    std::copy(rhs.data_, rhs.data_ + sizeof(rhs.data_)/sizeof(rhs.data_[0]), data_);
    body_length_ = rhs.body_length_;
    NEW; 
  }
  ~chat_message(){ DEL; }
  const char* data() const{ return data_; }
  char* data() { return data_; }
  size_t length() const { return header_length + body_length_; }
  const char* body() const { return data_ + header_length; }
  char* body() { return data_ + header_length; }
  size_t body_length() const { return body_length_; }
  void body_length(size_t new_length) {
    body_length_ = new_length;
    if (body_length_ > max_body_length) body_length_ = max_body_length;
  }
  bool decode_header() {
    using namespace std; // For strncat and atoi.
    char header[header_length + 1] = "";
    strncat(header, data_, header_length);
    body_length_ = atoi(header);
    if (body_length_ > max_body_length) { body_length_ = 0; return false; }
    return true;
  }
  void encode_header() {
    using namespace std; // For sprintf and memcpy.
    char header[header_length + 1] = "";
    sprintf(header, "%4d", static_cast<int>(body_length_));
    memcpy(data_, header, header_length);
  }
private:
  char data_[header_length + max_body_length];
  size_t body_length_;
};

typedef std::deque<boost::shared_ptr<chat_message>> chat_message_queue;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection> {
public:
  friend boost::object_pool<tcp_connection>;
  ~tcp_connection(){ dot('~'); DEL; }
  typedef boost::shared_ptr<tcp_connection> pointer;
  //static pointer create(boost::asio::io_service& io_service) { return pointer(new tcp_connection(io_service)); }
  static pointer create(boost::asio::io_service& io_service) { return tcp_connection_allocator.allocate(io_service); }
  tcp::socket& socket(){ return socket_; }
  void start(){
    BOOST_LOG_TRIVIAL(trace) << "tcp_connection::start ";
    boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), chat_message::header_length),
                            boost::bind(&tcp_connection::handle_read_header, shared_from_this(),
                                        boost::asio::placeholders::error));
  }
  void write(boost::shared_ptr<chat_message> msg) {
    io_service_.post(boost::bind(&tcp_connection::do_write, this, msg));
  }
  void close() { io_service_.post(boost::bind(&tcp_connection::do_close, this)); }
  void set_read_handler(boost::function<bool(chat_message&)> f) { read_handler_ = f; }
  void set_close_handler(boost::function<void()> f) { close_handler_ = f; }
  void start_write(){
    if(!write_msgs_.empty())
      boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front()->data(), write_msgs_.front()->length()),
    			       boost::bind(&tcp_connection::handle_write, this, boost::asio::placeholders::error));
  }
private:
  tcp_connection(boost::asio::io_service& io_service) : io_service_(io_service), socket_(io_service) { NEW; }
  void handle_read_header( const boost::system::error_code& error) {
    BOOST_LOG_TRIVIAL(trace) << "handle_read_header " << (bool)error;
    if (!error && read_msg_.decode_header()) {
      if(!socket_.is_open()) { BOOST_LOG_TRIVIAL(warning) << "socket closed at handle_read_header"; return; }
      boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                              boost::bind(&tcp_connection::handle_read_body, shared_from_this(), boost::asio::placeholders::error));
    }
    else { do_close(); }
  }
  void handle_read_body(const boost::system::error_code& error) {
    BOOST_LOG_TRIVIAL(trace) << "handle_read_body " << (bool)error;
    if (!error) {

      //if(read_handler_ && read_handler_(read_msg_)) { do_write(boost::make_shared<chat_message>(read_msg_)); }
      if(read_handler_ && read_handler_(read_msg_)) { do_write(chat_message_allocator.allocate(read_msg_)); }
      if(!socket_.is_open()) { BOOST_LOG_TRIVIAL(info) << "socket closed at handle_read_body"; return; }
      boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), chat_message::header_length),
                              boost::bind(&tcp_connection::handle_read_header, shared_from_this(), boost::asio::placeholders::error));
    }
    else { do_close(); }
  }
  void do_write(boost::shared_ptr<chat_message> msg) {
    BOOST_LOG_TRIVIAL(trace) << "do_write";
    bool write_in_progress = !write_msgs_.empty();
    BOOST_ASSERT(msg);
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
      if(!socket_.is_open()) { BOOST_LOG_TRIVIAL(info) << "socket closed at do_write"; return; }
      boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front()->data(), write_msgs_.front()->length()),
                               boost::bind(&tcp_connection::handle_write, this, boost::asio::placeholders::error));
    }
  }
  void handle_write(const boost::system::error_code& error) {
    BOOST_LOG_TRIVIAL(trace) << "handle_write " << (bool)error;
    if (!error) {
      if(write_msgs_.empty()) { BOOST_LOG_TRIVIAL(info) << "msg queue is empty at handle_write"; return; }
      write_msgs_.pop_front();
      if (!write_msgs_.empty()) {
	if(!socket_.is_open()) { BOOST_LOG_TRIVIAL(info) << "socket closed at handle_write"; return; }
        boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front()->data(), write_msgs_.front()->length()),
                                 boost::bind(&tcp_connection::handle_write, this, boost::asio::placeholders::error));
      }
    }
    else { do_close(); }
  }
  void do_close() { 
    if(socket_.is_open()){ socket_.close(); if(close_handler_) close_handler_(); }
  }
private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  boost::function<bool(chat_message&)> read_handler_;
  boost::function<void()> close_handler_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_service& io_service, int port, boost::function<bool(chat_message&)> f)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port), true), read_handler_(f){
    start_accept();
  }
private:
  void start_accept() {
    BOOST_LOG_TRIVIAL(trace) << "start_accept... ";
    tcp_connection::pointer new_connection = tcp_connection::create(acceptor_.get_io_service());
    if(read_handler_) new_connection->set_read_handler(read_handler_);
    acceptor_.async_accept(new_connection->socket(), boost::bind(&tcp_server::handle_accept, this, new_connection, boost::asio::placeholders::error));
  }
  void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error) {
    if (!error) 
      new_connection->start(); 
    else BOOST_LOG_TRIVIAL(warning) << "handle_accept: " << error.message() ;
    start_accept();
  }
  tcp::acceptor acceptor_;
  boost::function<bool(chat_message&)> read_handler_;
};

struct ip_port { std::string ip; int port; };
  
class tcp_client{
public:
  tcp_client(boost::asio::io_service& io_service, ip_port ip, boost::function<bool(chat_message&)> f)
    : deadline_timer_(io_service) {
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(ip.ip, boost::lexical_cast<std::string>(ip.port));
    init(io_service, resolver.resolve(query), f);
  }
  tcp_client(boost::asio::io_service& io_service, tcp::resolver::iterator endpoint_iterator, boost::function<bool(chat_message&)> f)
    : deadline_timer_(io_service, boost::posix_time::seconds(3)) {
    init(io_service, endpoint_iterator, f);
  }
  void write(boost::shared_ptr<chat_message> msg){ CHECK_MAX_NEW_CNT;  connection_->write(msg); }
private:
  void init(boost::asio::io_service& io_service, tcp::resolver::iterator endpoint_iterator, boost::function<bool(chat_message&)> f){
    endpoint_iterator_ = endpoint_iterator;
    connection_ = tcp_connection::create(io_service);
    connection_->set_close_handler(boost::bind(&tcp_client::reconnect, this));
    if(f) connection_->set_read_handler(f);
    BOOST_ASSERT(false == connection_->socket().is_open());
    boost::asio::async_connect(connection_->socket(), endpoint_iterator, boost::bind(&tcp_client::handle_connect, this, boost::asio::placeholders::error));
  }
  void reconnect(){
    BOOST_LOG_TRIVIAL(trace) << "reconnect";
    deadline_timer_.expires_from_now(boost::posix_time::seconds(3));
    deadline_timer_.async_wait(boost::bind(&tcp_client::do_connect, this, boost::asio::placeholders::error));
  }
  void do_connect(const boost::system::error_code& error){
    BOOST_LOG_TRIVIAL(trace) << "do_connect " << (bool)error;
    if(!error){
      BOOST_ASSERT(false == connection_->socket().is_open());
      boost::asio::async_connect(connection_->socket(), endpoint_iterator_,
                                 boost::bind(&tcp_client::handle_connect, this, boost::asio::placeholders::error));
      connection_->start_write();
    } else { BOOST_LOG_TRIVIAL(warning) << "do_connect: " << error.message(); }
  }
  void handle_connect(const boost::system::error_code& error) {
    BOOST_LOG_TRIVIAL(trace) << "handle_connect " << (bool)error;
    if (!error) {
      connection_->start();
    }
    else { reconnect(); }
  }
  tcp_connection::pointer connection_;
  tcp::resolver::iterator endpoint_iterator_;
  boost::asio::deadline_timer deadline_timer_;
  boost::function<bool(chat_message&)> read_handler_;
};

void print_new_cnt(boost::asio::deadline_timer & d, boost::system::error_code const & error);


#endif
