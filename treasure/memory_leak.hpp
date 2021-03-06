#ifndef __TREASURE_MEMORY_LEAK_HPP__
#define __TREASURE_MEMORY_LEAK_HPP__


#define MAX_NEW_CNT 512*1024

#ifdef DEBUG

extern boost::atomic<int> new_cnt;

#define NEW ++new_cnt
#define DEL --new_cnt
#define NEW_CNT new_cnt
#define PRINT_NEW_CNT do{ \
    std::cout << "new_cnt: " << NEW_CNT << std::endl;	\
  } while(0)
#define CHECK_MAX_NEW_CNT do{ if(new_cnt > MAX_NEW_CNT) { dot('_'); usleep(100*1024); return; } } while(0)

#else

#define NEW (void)0
#define DEL (void)0
#define NEW_CNT (void)0
#define PRINT_NEW_CNT (void)0
#define CHECK_MAX_NEW_CNT (void)0
#endif 

#endif
