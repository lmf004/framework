#ifndef __RAW_BUF_HPP__
#define __RAW_BUF_HPP__


struct raw_buf 
{
  raw_buf() 
  { 
    static const int default_size = 512;
    do_malloc(default_size);
  }
  
  ~raw_buf(){
    g_free(_data);
  }
  
  explicit raw_buf(int len) {
    do_malloc(len);
  }
  
  explicit raw_buf(char const * buf, int len = 0)
    : _data(NULL),_len(0)
  {
    if(0 == len) 
      len = strlen(buf)+1;
    do_malloc(len);
    if(_data)
      memcpy(_data, buf, _len);
  }
  
  void do_malloc(int len)
  {
    _len = 0;
    _data = (char *)g_malloc0(len);
    
    if(_data) 
      _len = len;
    else
      g_warning("g_malloc0(%d) failed!", len);
  }
  
  raw_buf(raw_buf && rhs) {
    g_warning("raw_buf's rvalue constructor should not be called!!!");
    // this funciton is for the following clone function to be compiled successfully;
  }
  
  void expand(int len)
  {
    if(len <= _len)
      return;
    
    char* data = (char*)g_malloc0(len);
    
    if(data == NULL) {
      g_warning("g_malloc0(%d) failed!", len);
      return;
    }

    memcpy(data, _data, _len);
    g_free(_data);
    _data = data;
    _len = len;
  }
  
  void reserve(int len)
  {
    if(len <= _len)
      return;
    
    char* data = (char*)g_malloc0(len);

    if(data == NULL) {
      g_warning("g_malloc0(%d) failed!", len);
      return;
    }

    g_free(_data);
    _data = data;
    _len = len;
  }
  
  raw_buf clone(){ return raw_buf(_data, _len); }
  
  raw_buf & assign(raw_buf const & rhs)
  { 
    if(rhs._len <= _len){
      memcpy(_data, rhs._data, rhs._len); 
      _len = rhs._len;
    } else
      g_warning("raw_buf::assign: can't not assign a bigger raw_buf to a small one!");
    return *this; 
  }
  
  char* data() { return _data; }
  int size() { return _len; }
  
private:
  
  raw_buf(const raw_buf & rhs);
  char * _data;
  int _len;
};

//--------------------------------------------------------------------------------


#endif
