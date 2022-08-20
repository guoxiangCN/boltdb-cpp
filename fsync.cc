#include "boltdb/fsync.h"

#if defined(__linux__)
#include <unistd.h>
namespace boltdb {
Status fsync(int fd) {
#ifdef USE_FDATASYNC
  ::fdatasync(fd);
#else
  ::fsync(fd);
#endif
  return Status::OK();
}
}  // namespace boltdb
#else

namespace boltdb {
Status fsync(int fd) { return Status::OK(); }
}  // namespace boltdb
#endif