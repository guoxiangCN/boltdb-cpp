#ifndef __BOLTDB_MLOCK_H__
#define __BOLTDB_MLOCK_H__

#include <stddef.h>

namespace boltdb {
void MLock(void* addr, size_t locksz);
void MUnLock(void* addr, size_t locksz);
}  // namespace boltdb

#endif