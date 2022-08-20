#include "mlock.h"

#if defined(__unix__) || defined(__linux__)
#include <sys/mman.h>
#endif

namespace boltdb {

void MLock(void* addr, size_t locksz) {}
void MUnLock(void* addr, size_t locksz) {}

}  // namespace boltdb
