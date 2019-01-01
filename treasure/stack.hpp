#ifndef __STATCK_HPP__
#define __STATCK_HPP__

template<class T>
struct Stack
{
  void push(const T & value)
  {
    _nodes.push_back(value);
  }
  
  T pop() {
    if(_nodes.empty())
      return T();
    auto value = _nodes.back();
    _nodes.pop_back();
    return value;
  }

  void reverse()
  {
    int i = 0;
    int j = _nodes.size()-1;
    
    while(i<j) {
      std::swap(_nodes[i], _nodes[j]);
      i++;
      j--;
    }
  }
  
  void dump()
  {
    std::cout << "stack dump:\n";
    for(auto a : _nodes)
      std::cout << a << " ";
    std::cout << "\n";
  }
  
private:
  
  std::vector<T> _nodes;
};


#endif
