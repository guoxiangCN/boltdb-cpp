#ifndef __BOLTDB_STATUS_H__
#define __BOLTDB_STATUS_H__

namespace boltdb {

class Status {
public:
 static Status OK();
 static Status Invalid();
 static Status VersionMismatch();
 static Status Checksum();
};
}

#endif