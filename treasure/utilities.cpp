#include "headers.hpp"

bool g_last_is_dot = false;

struct CharsDict{
  static std::string hexat(int i){ return s_hex[i]; }
  static char charat(int i) { return s_chars[i]; }
  static void init(){
    for(int i=0; i<256; ++i) { s_hex.push_back(boost::str(boost::format("%02x") % i)); s_chars.push_back(isprint(i) ? (char)i : '.'); }
    inited = true;
    g_print("CharsDict init success\n");
  }
  static std::vector<std::string> s_hex;
  static std::vector<char> s_chars;
  static bool inited;
};

std::vector<std::string> CharsDict::s_hex;
std::vector<char> CharsDict::s_chars;
bool CharsDict::inited = false;

void loghex(const std::string title, const unsigned char* p, int len){
  if(false == CharsDict::inited) CharsDict::init();
  if(0 == len) len = *(unsigned short*)p;
  int lines = len/16 + (len%16 ? 1 : 0);  std::string strs;
  strs += title + "\n";
  for(int i=0; i<lines; ++i){
    strs += boost::str(boost::format("%04d  ") % (i*16));
    const unsigned char* s = p+i*16;
    if(i == lines-1 && len%16 != 0){
      for(int j=0; j<16; j++) { 
	(j< len%16) ? (strs += CharsDict::hexat(s[j]) + " ") : (strs += "   ");
	if(j == 7) strs += "  ";
      }
      strs += "  ";
      for(int j=0; j<len%16; j++) strs += CharsDict::charat(s[j]);
    }else{
      for(int j=0; j<16; ++j) { 
	strs += CharsDict::hexat(s[j]) + " ";
	if(j == 7) strs += "  ";
      }
      strs += "  ";
      for(int j=0; j<16; ++j) strs += CharsDict::charat(s[j]);
    }
    strs += "\n";
  }
  std::cout << strs;
}
void loghex(const std::string title, const char* p, int len) { loghex(title, (const unsigned char*)p, len); }

//--------------------------------------------------------------------------------------------------------

int scoper::steps = 0;

void real_dot(char const * str)
{
  std::cout << str << std::flush;
  g_last_is_dot = true;
}
void real_dot(std::string const & str){
  real_dot(str.c_str());
}
//---------------------------------------------------------------------------------------------------------

std::string timestamp_2_string(guint32 stamp)
{
  struct tm tformat = {0};
  time_t tt(stamp);
  localtime_r(&tt, &tformat); 
  char buf[80]; memset(buf, 0, sizeof(buf));
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tformat); 
  std::string ret = buf;
  return ret;
}
std::string timestamp_2_time(guint32 stamp)
{
  struct tm tformat = {0};
  time_t tt(stamp);
  localtime_r(&tt, &tformat); 
  char buf[80]; memset(buf, 0, sizeof(buf));
  strftime(buf, sizeof(buf), "%H:%M:%S", &tformat); 
  return std::string(buf);
}

std::string timestamp_2_date(guint32 stamp)
{
  struct tm tformat = {0};
  time_t tt(stamp);
  localtime_r(&tt, &tformat); 
  char buf[80]; memset(buf, 0, sizeof(buf));
  strftime(buf, sizeof(buf), "%Y/%m/%d", &tformat); 
  std::string ret = buf;
  return ret;
}

std::string date_2_week(std::string date)
{
  if(date.length() != 8) return "";
  std::string year = date.substr(0, 4);
  std::string month = date.substr(4, 2);
  std::string day = date.substr(6, 2);
  struct tm tm_now;
  memset(&tm_now, 0, sizeof(tm_now));
  tm_now.tm_year = atoi(year.c_str()) - 1900;
  tm_now.tm_mon = atoi(month.c_str())-1;
  tm_now.tm_mday = atoi(day.c_str());
  time_t new_time = mktime(&tm_now);
  struct tm *tm_now2;
  tm_now2 = localtime(&new_time);  
  char const* wday_descs[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  return wday_descs[tm_now2->tm_wday%7];
}

//-------------------------------------------------------------------------------

int tick() { return Tick::instance()->tick(); }

//----------------------------------------------------------------------------------------------------

void string_to_arithmetic(std::string str, char & n)               { n = (char)std::stoi(str); }
void string_to_arithmetic(std::string str, unsigned char & n)      { n = (unsigned char)std::stoi(str); }
void string_to_arithmetic(std::string str, short & n)              { n = (short)std::stoi(str); }
void string_to_arithmetic(std::string str, unsigned short & n)     { n = (unsigned short)std::stoi(str); }
void string_to_arithmetic(std::string str, int & n)                { n = (int)std::stoi(str); }
void string_to_arithmetic(std::string str, unsigned int & n)       { n = (unsigned int)std::stoi(str); }
void string_to_arithmetic(std::string str, long & n)               { n = std::stol(str); }
void string_to_arithmetic(std::string str, unsigned long & n)      { n = std::stoul(str); }
void string_to_arithmetic(std::string str, long long & n)          { n = std::stoll(str); }
void string_to_arithmetic(std::string str, unsigned long long & n) { n = std::stoull(str); }

//----------------------------------------------------------------------------------------------------

shared_ptr<raw_buf> wait(std::string evt, int seq, shared_ptr<bool> running) { 
  return wait_event::instance().wait(evt, seq, running); 
}
int timed_wait(std::string evt, int seq, int millisecond, shared_ptr<raw_buf> & data, shared_ptr<bool> running) { 
  return wait_event::instance().timed_wait(evt, seq, millisecond, data, running); 
}
void notify(std::string evt, int seq, shared_ptr<raw_buf> data) { 
  return wait_event::instance().notify(evt, seq, data); 
}

//----------------------------------------------------------------------------------------------------

std::string get_localhost_ip()
{
  static std::string ip;
  if(!ip.empty()) return ip;
  
  auto getip = [](const std::string &name) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
      g_warning("socket(AF_INET, SOCK_DGRAM, 0) failed");
      return std::string();
    }
    
    struct ifreq ifr;
    memset(ifr.ifr_name, 0x00, IFNAMSIZ);
    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
    
    if (ioctl(fd, SIOCGIFADDR, &ifr) != -1) {
      char buf[32] = "";
      inet_ntop(AF_INET, &(((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr), buf, 32);
    } else {
      perror("ioctl");
      g_warning("ioctl(fd, SIOCGIFADDR, &ifr) failed");
    }
    
    ::close(fd);
    return std::string(buf);
  };
  
  auto eths = {"eth0", "eth1", "eth2", "eth3", "eth4"};
  std::find_if(eths.begin(), eths.end(), 
	       [&](char const * eth) { return !(ip = getip(eth)).empty(); });
  // if(!(ip = getip("eth0")).empty()) return ip;
  // if(!(ip = getip("eth1")).empty()) return ip;
  // if(!(ip = getip("eth2")).empty()) return ip;
  // if(!(ip = getip("eth3")).empty()) return ip;
  // if(!(ip = getip("eth4")).empty()) return ip;
  return ip;
}

std::vector<std::string> get_localhost_iplist()
{
  std::vector<std::string> v;
  BOOST_SCOPE_EXIT(&v){
    if(v.empty()) printf("get_localhost_iplist failed!\n");
  } BOOST_SCOPE_EXIT_END;
      
  char host_name[255];
  if (gethostname(host_name, sizeof(host_name)) == -1) { 
    perror("gethostname"); return v;
  }
  
  struct hostent *phe = gethostbyname(host_name);
  if (phe == 0) { printf("gethostbyname failed"); return v; }
  
  for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
    struct in_addr addr;
    memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
    v.push_back( inet_ntoa(addr) );
  }
  
  return v;
}
//----------------------------------------------------------------------------------------------------

std::string string_format(const char* format, ...)
{
  char buffer[800]; memset(buffer, 0, sizeof(buffer));
  
  va_list vl;
  va_start(vl, format);
  vsprintf(buffer, format, vl);
  va_end(vl);
  
  std::string ret = buffer;
  return ret;
}

// void print(const char* format, ...)
// {
//   va_list vl;
//   va_start(vl, format);
//   g_print(format, vl);
//   va_end(vl);
// }

