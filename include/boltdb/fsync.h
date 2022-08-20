#ifndef __BOLTDB_SYNC_H__
#define __BOLTDB_SYNC_H__

#include "boltdb/status.h"

namespace boltdb{
Status fsync(int fd);
}

#endif