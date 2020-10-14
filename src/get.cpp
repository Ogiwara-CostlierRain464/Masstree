#include "get.h"

namespace masstree{

#ifndef NDEBUG
SequentialHandler get_handler0{};
SequentialHandler get_handler1{};
Marker has_locked_marker{};
Marker was_unstable_marker{};
#endif

}
