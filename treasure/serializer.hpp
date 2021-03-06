#ifndef __SERIALIZER_HPP__
#define __SERIALIZER_HPP__

// template<typename T, char SplitChar>
// struct Vector : std::vector<T>
// {
//   static const char split_char = SplitChar;
// };
// template<typename Key, typename Value, char SplitChar1, char SplitChar2>
// struct Map : std::map<Key, Value>
// {};

template<typename T, char SplitChar>
struct Vector
{
  static const char split_char = SplitChar;
  std::vector<T> d;
};

template<typename Key, typename Value, char SplitChar1, char SplitChar2>
struct Map
{
  std::map<Key, Value> d;
};

template<typename T, int N, char SplitChar>
struct Array
{
  static const char split_char = SplitChar;
  T & operator[](int i) { return data[i]; }
  T const & operator[](int i) const { return data[i]; }
  T data[N];
};

template<char SplitChar, typename Derived> 
struct Serializable;

struct Serializer{};
struct TextSerializer : Serializer
{
  static int extract(std::string text, std::string & t){ t = text; return 0; }
  static int insert (std::string & text, std::string const & t){ text = t; return 0; }
  
  template<typename T>
  static typename std::enable_if<std::is_arithmetic<T>::value, int>::type
  extract(std::string text, T & t){ 
    try { string_to_arithmetic(text, t); } catch(...) { return 1; } return 0;
  }
  template<typename T>
  static typename std::enable_if<std::is_arithmetic<T>::value, int>::type
  insert (std::string & text, T const & t){ 
    try { text = std::to_string(t); } catch(...) { return 2; } return 0;
  }
  
  template<typename T, char SplitChar>
  static int extract(std::string text, Vector<T, SplitChar> & container){
    int ret;
    std::vector<std::string> v;
    boost::split(v, text, boost::is_any_of(std::string(1, SplitChar)));
    for(auto a : v){
      T t; 
      if(0 != (ret = extract(a, t))) 
	return ret;
      container.d.push_back(t);
    }
    return 0;
  }
  template<typename T, char SplitChar>
  static int insert(std::string & text, Vector<T, SplitChar> const & container){
    int ret;
    std::vector<std::string> v;
    for(auto a : container.d){
      std::string str; 
      if(0 != (ret = insert(str, a)))
	return ret;
      v.push_back(str);
    }
    text = algorithm::join(v, std::string(1, SplitChar));
    return 0;
  }
  
  template<typename T, int N, char SplitChar>
  static int extract(std::string text, Array<T, N, SplitChar> & container){
    int ret;
    std::vector<std::string> v;
    boost::split(v, text, boost::is_any_of(std::string(1, SplitChar)));
    if(N != v.size()) return 3;
    int n = std::min((int)v.size(), N);
    for(int i=0; i<n; ++i){
      T t; 
      if(0 != (ret = extract(v[i], t))) return ret;
      container[i] = t;
    }
    return 0;
  }
  template<typename T, int N, char SplitChar>
  static int insert (std::string & text, Array<T, N, SplitChar> const & container){
    int ret;
    std::vector<std::string> v;
    for(int i=0; i<N; ++i){
      std::string str; 
      if(0 != (ret = insert(str, container[i])))
	return ret;
      v.push_back(str);
    }
    text = algorithm::join(v, std::string(1, SplitChar));
    return 0;
  }

  template<typename... Args>
  static int extract(std::string text, char SplitChar, Args &... args){
    int i = 0;
    std::vector<std::string> v;
    boost::split(v, text, boost::is_any_of(std::string(1, SplitChar)));
    if(v.size() != sizeof...(args)) return 3;
    int n = std::min(v.size(), sizeof...(args));
    int ret = 0;
    int dump[sizeof...(args)] =  { ( i < n ? (ret += extract(v[i++], args), 0) : 1 )... };
    return ret;
  }
  template<typename... Args>
  static int insert (std::string & text, char SplitChar, Args const &... args){
    std::vector<std::string> v;
    std::string str;
    int ret = 0;
    int dump[sizeof...(args)] = { ( ret += insert(str, args), v.push_back(str), 0)... };
    text = algorithm::join(v, std::string(1, SplitChar));
    return ret;
  }
  
  template<char SplitChar, typename Derived>
  static int extract(std::string text, Serializable<SplitChar, Derived> & object) {
    return ((Derived*)&object)->load(text);
  }
  template<char SplitChar, typename Derived>
  static int insert (std::string & text, Serializable<SplitChar, Derived> const & object) {
    return ((Derived*)&object)->save(text);
  }

  template<typename Key, typename Value, char SplitChar1, char SplitChar2>
  static int extract(std::string text, Map<Key, Value, SplitChar1, SplitChar2> & map) {
    int ret;
    std::vector<std::string> v;
    boost::split(v, text, boost::is_any_of(std::string(1, SplitChar1)));
    for(auto a : v) {
      {
	std::vector<std::string> v;
	boost::split(v, a, boost::is_any_of(std::string(1, SplitChar2)));
	if(2 == v.size()) {
	  Key key; 
	  if(0 != (ret = extract(v[0], key))) return ret;
	  Value value; 
	  if(0 != (ret = extract(v[1], value))) return ret;
	  map.d[key] = value;
	}
      }
    }
    return 0;
  }
  template<typename Key, typename Value, char SplitChar1, char SplitChar2>
  static int insert (std::string & text, Map<Key, Value, SplitChar1, SplitChar2> const & map) {
    int ret;
    std::vector<std::string> v;
    for(auto a : map.d){
      std::string str1; 
      if(0 != (ret = insert(str1, a.first))) return ret;
      std::string str2; 
      if(0 != (ret = insert(str2, a.second))) return ret;
      v.push_back(str1 + std::string(1, SplitChar2) + str2);
    }
    text = algorithm::join(v, std::string(1, SplitChar1));
    return 0;
  }
  
};

struct nonsense{};

template<char SplitChar = '&', typename Derived = nonsense>
struct Serializable
{
  static const char splitchar = SplitChar;
};

#define SERIALIZE_MEMEBER(...)						\
  int load(std::string s){ return TextSerializer::extract(s, splitchar, __VA_ARGS__); } \
  int save(std::string& s) const { return TextSerializer::insert(s, splitchar, __VA_ARGS__); } \
  /**/


#endif

// struct Nums : Serializable<',', Nums>
// {
//   int a;
//   int b;
//   int c;
//   SERIALIZE_MEMEBER(a, b, c)
// };
// struct Ball : Serializable<'+', Ball>
// {
//   int a;
//   int b;
//   int c;
//   Vector<Nums, '^'> d;
//   SERIALIZE_MEMEBER(a, b, c, d)
// };
// struct Sell : Serializable<'*', Sell>
// {
//   int a;
//   std::string b;
//   Ball ball;
//   int c;
//   Vector<int, '|'> d;
//   Array<std::string, 5, '$'> e;
//   Map<std::string, int, '~', ':'> f;
//   SERIALIZE_MEMEBER(a, b, ball, c, d, e, f)
// };

// Sell a;
// a.a = 1; a.b = "aaa"; a.c = 2; a.d.push_back(7); a.d.push_back(8); a.d.push_back(9);
// a.ball.a = 33; a.ball.b = 44; a.ball.c = 55; 
// Nums n1; n1.a = 001; n1.b = 002; n1.c = 003; 
// Nums n2; n2.a = 101; n2.b = 102; n2.c = 103; 
// Nums n3; n3.a = 201; n3.b = 202; n3.c = 203; 
// a.ball.d.push_back(n1);
// a.ball.d.push_back(n2);
// a.ball.d.push_back(n3);
// a.e[0] = "hhh"; a.e[1] = "iii"; a.e[2] = "jjj"; a.e[3] = "kkk"; a.e[4] = "lll";
// a.f["aaa"] = 111;
// a.f["bbb"] = 222;
// a.f["ccc"] = 333;
// a.f["ddd"] = 444;
// std::string str;
// a.save(str);
// std::cout << str << '\n';

// {
//   Sell a;
//   a.load(str);
//   std::string str2;
//   a.save(str2);
//   std::cout << str2 << '\n';
//   return 0;
// }
