#ifndef __BQUEUES_HPP__
#define __BQUEUES_HPP__

#if 0

enum {
  TASK_NCPC,
  TASK_TERM_MGR,
  TASK_AC,
  TASK_GL,
  TASK_ADDER,
  TASK_CNT,
};

struct List {
  char* data;
  List* next;
};

const int[] vSize = {  16,   32,   64,  128,  256,  512, 1024 };
const int[] vCnt  = {1024, 1024, 2048, 2048, 1024, 2048, 1024 };
const int type_cnt =  sizeof(vSize/sizeof(vSize[0]));
constexpr int node_cnt() {
  int ret = 0;
  for(int i=0; i<type_cnt; ++i)
    ret += vCnt[i];
  return ret;
}

constexpr int block_size(){
  int ret = 0;
  for(int i=0; i<type_cnt; ++i)
    ret += vSize[i]*vCnt[i];
  return ret;
}

struct ShmData
{
  List* heads[type_cnt];
  List* taskHeads[TASK_CNT]; // for queue
  List* taskTails[TASK_CNT];
  List  nodes[node_cnt()*2]; // *2 for split
  int   node_end;            // init = node_cnt;
  List* freeNodes;
  char  buf  [block_size()];
};

ShmData* shmdata = NULL;

int create_shared_memory()
{
  shmdata= shm_create(SHM_ID, sizeof(ShmData));
  char * p = shmdata->buf;
  List* node = shmdata->nodes;
  
  for(int i=0; i<type_cnt; ++i) {
    shmdata->heads[i] = node;
    int size = vSize[i]; int cnt = vCnt[i];
    for(int j=0; j<cnt; ++j) {
      node->init(p, ++node); p += size;
    }
    node->next = NULL;
  }  
}

int get_shared_memory() {
  shmdata= shm_get(SHM_ID, sizeof(ShmData));
}

// 16   : 64*1024
// 32   : 32*1024
// 64   : 16*2048
// 128  : 8*2048
// 256  : 4*2048
// 512  : 2*1024
// 1024 : 1*4


int sort_list(Node* node) {
  auto node_cnt_func = [=](Node* node)
    {
      int cnt = 0;
      while(node) { cnt++; node = node->next; }
      return cnt;
    };

  auto swap_node = [&] (Node* left, Node* right)
    {
      char* data = left->data;
      left->data = right->data;
      right->data = data;
    };

  int cnt = node_cnt_func(node);
  for(int e=cnt-1; e>0; --e) {
    Node* curr = node;
    for(int s=0; s<e; ++s) {
      if(curr->data > curr->next->data)
	swap_node(curr, curr->next);
      curr = curr->next;
    }
  }
}

Node* find_continuous_block(Node* & node, int blocksize, int blockcnt)
{
  int cnt = node_cnt_func(node);
  char* data = node->data;
  Node* curr = node->next;
  int accumblockcnt = 1;
  Node* start = node; Node* end; Node* prev = NULL; Node* before_start = NULL;
  while(curr) {
    if(curr->data - data == blocksize){
      ++accumblockcnt;
      if(accumblockcnt >= blockcnt){
	end = curr->next; curr->next = NULL;
	(before_start == NULL) ? ( node = end ) : ( before_start->next = end );
	return start;
      }
    } else {
      accumblockcnt = 1;
      start = curr;
      before_start = prev;
    }
    data = curr->data;
    prev = curr;
    curr = curr->next;
  }
  return NULL;
}

std::vector<int> borrowable_block_types()
{
  int min_space = 8192;
  std::vector< pair<int, int> > v;
  for(int i=0; i<type_cnt; ++i) {
    int cnt = node_cnt_func(shmdata->heads[i]);
    if(cnt*vSize[i] <= min_space) continue;
    v.push_back(make_pair(cnt*vSize[i], i));
  }
  
  std::sort(v.begin(), v.end(), [](auto a, auto b){ return a.first < b.first; });
  std::vector<int> r;
  for(auto a : v) r.push_back(a.second);
  return r;
}

int bqueues_balance(int type)
{
  auto v = borrowable_block_types();
  for(auto i : v) {
    if(i < type) continue;
  //for(int i=type+1; i < type_cnt; ++i){
    List* node;
    {
      shm_lockguard l(i);
      if(node = shmdata->heads[i]){
	shmdata->heads[i] = shmdata->heads[i]->next;
      } else continue;
    }

    int cnt = vSize[i]/vSize[type];
    char* p = node->data;
    for(int j=0; j<cnt; ++j) {
      Node* subnode;
      if(shmdata->freeNodes) {
	subnode = shmdata->freeNodes;
	shmdata->freeNodes = shmdata->freeNodes->next;
      } else subnode = &shmdata->nodes[shmdata->node_end++];
      
      subnode->init(p+j*vSize[type], NULL);
      shm_lockguard l(type);
      subnode->next = shmdata->heads[type];
      shmdata->heads[type] = subnode;
    }

    node->next = shmdata->freeNodes;
    shmdata->freeNodes = node;
    
    return 0;
  }

  // bigger block all has been used!!!
  // we have to join the smaller blocks
  auto find_last = [=](Node* node) {
    while(node){
      if(node->next = NULL) return node;
      node = node->next;
    }
  };

  v.reverse_sort(); // bigger first
  for(auto i : v) {
    if(i > type) continue;
  //for(int i=type-1; i>0; --i) {
    sort_list(shmdata->heads[i]);
    Node* node = find_continuous_block(shmdata->heads[i], vSize[i], vSize[type]/vSize[i]);
    if(node == NULL) continue;
    
    find_last(node->next)->next = shmdata->freeNodes;
    shmdata->freeNodes = node->next;
    
    node->next = shmdata->heads[type] ;
    shmdata->heads[type] = node->next;
    return 0;
  }

  // failed
  return 1;
}

int bqueues_send(int taskid, char* buf, int len)
{
  int type = pick_block_type(len);
  List* node;

 START:
  {
    shm_lockguard l(type);
    node = shmdata->heads[type];
    shmdata->heads[type] = shmdata->heads[type]->next;
  }

  if(node == NULL) {
    bqueues_balance(); goto START;
  }
  
  memcpy(node->data, buf, len);

  {
    task_lockguard l(taskid);
    node->next = shmdata->taskHeads[taskid];
    shmdata->taskHeads[taskid] = node;
  }
}

int bqueues_recv(int taskid, char* buf, int len)
{
  bool first = true;
  List* node = NULL;
 START:
  first ? (first = false) : ( sleep(1) );
  {
    task_lockguard l(taskid);
    List* & taskHeads = shmdata->taskHeads[taskid];
    if(NULL == taskHeads) goto START;
    node = taskHeads;
    taskHeads = taskHeads->next;
  }

  memcpy(buf, node->data, *(uint16*)node->data);
  int type = pick_block_type(*(uint16*)node->data);
    
  {
    shm_lockguard l(type);
    node->next = shmdata->heads[type];
    shmdata->heads[type] = node;
  }
}

void thread_func_ncpc() {
  bqueues_register_task(TASK_NCPC);
  while(true) {
    char buf[1024]; int len;
    receive_msg_from_network(buf, len);
    if(ncpc_handle_msg(buf, len)) {
      bqueues_send(TASK_TERM_MGR, buf, len);
    }
  }
}

void thread_func_term_mgr() {
  bqueues_register_task(TASK_TERM_MGR);
  while(true) {
    char buf[1024], int len;
    bqueues_recv(TASK_TERM_MGR, buf, len);
    if(term_mgr_handle_msg(buf, len)) {
      bqueues_send(TASK_AC, buf, len);
    }
  }
}




#endif

#endif
