#ifndef __TREASURE_HEADERS_HPP__
#define __TREASURE_HEADERS_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
//#include <sys/socket.h>
//#include <sys/epoll.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <sys/param.h> 
//#include <sys/ioctl.h> 
//#include <net/if.h> 
//#include <net/if_arp.h> 
#include <time.h>
#include <sys/time.h>
#include <iconv.h>
#include <ctype.h>

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <functional>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <memory>
#include <thread>
#include <future>
#include <condition_variable>
		
#include <glib.h>
#include <gtk/gtk.h>

#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/type_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/pool/object_pool.hpp>

#include <boost/thread/thread.hpp>

#include <boost/tuple/tuple.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/hex.hpp>

#include <boost/range/algorithm_ext/erase.hpp>

#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/mod.hpp>
#include <boost/preprocessor/arithmetic/div.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/debug/assert.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>
#include <boost/preprocessor/variadic/to_array.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/while.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/while.hpp>
#include <boost/preprocessor/array.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/quote.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/protect.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/base.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>

#include <boost/functional/forward_adapter.hpp>
#include <boost/functional/lightweight_forward_adapter.hpp>

#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/bind.hpp>
#include <boost/bind/arg.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/bind/apply.hpp>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scope_exit.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>

namespace chrono = std::chrono;
namespace lambda = boost::lambda;
namespace algorithm = boost::algorithm;
namespace mpl = boost::mpl;
namespace fusion = boost::fusion;
namespace interprocess = boost::interprocess;

template<typename T> using shared_ptr = std::shared_ptr<T>;
template<typename T> using function = std::function<T>;
using std::make_shared;

#include "utilities.hpp"
#include "memory_leak.hpp"
#include "rawbuf.hpp"
#include "thread.hpp"
#include "socket_helper.hpp"
#include "queue.hpp"
#include "object_pool.hpp"
#include "socket_server_client.hpp"
#include "serializer.hpp"
#include "loop.hpp"
#include "task.hpp"
#include "threadpool.hpp"

// #include "socket.hpp"

// #include <boost/type_erasure/any.hpp>
// #include <boost/type_erasure/builtin.hpp>
// #include <boost/type_erasure/callable.hpp>
// #include <boost/type_erasure/any_cast.hpp>
// #include <boost/type_erasure/iterator.hpp>
// #include <boost/type_erasure/operators.hpp>
// #include <boost/type_erasure/tuple.hpp>
// #include <boost/type_erasure/same_type.hpp>
// #include <boost/type_erasure/member.hpp>

// #include <boost/log/common.hpp>
// #include <boost/log/expressions.hpp>
// #include <boost/log/utility/setup/file.hpp>
// #include <boost/log/utility/setup/console.hpp>
// #include <boost/log/utility/setup/common_attributes.hpp>
// #include <boost/log/attributes/timer.hpp>
// #include <boost/log/attributes/named_scope.hpp>
// #include <boost/log/sources/logger.hpp>
// #include <boost/log/support/date_time.hpp>
// #include <boost/log/trivial.hpp>
// #include <boost/log/core.hpp>

// namespace logging = boost::log;
// namespace sinks = boost::log::sinks;
// namespace attrs = boost::log::attributes;
// namespace src = boost::log::sources;
// namespace expr = boost::log::expressions;
// namespace keywords = boost::log::keywords;

// namespace type_erasure = boost::type_erasure;
// namespace lambda = boost::lambda;
// 

// using boost::asio::ip::tcp;

#endif
