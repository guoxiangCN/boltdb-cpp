#include "boltdb/boltdb.h"

namespace boltdb {

Status Meta::Validate() const {
  if (this->magic != kMagic) {
    return Status::Invalid();
  }
  if (this->version != kVersion) {
    return Status::VersionMismatch();
  }
  if (this->checksum != 0 && this->checksum != Sum64()) {
    return Status::Checksum();
  }
  return Status::OK();
}

void Meta::Copy(Meta* dest) { *dest = *this; }

void Meta::Write(Page* page) {
  // TODO

  // Page id is either going to be 0 or 1 which we can determine by the
  // transaction ID.
  page->id = (this->txid % 2);
  page->flags |= kPageFlagMeta;

  // Calculate the checksum.
  this->checksum = this->Sum64();

  this->Copy(page->AsMeta());
}

#define FNV_PRIME 1099511628211UL
#define FNV_OFFSET 14695981039346656037UL

static uint64_t fnv1a(char* key, int stringlen) {
  uint64_t hash = FNV_OFFSET;
  for (int i = 0; i < stringlen; i++) {
    hash = hash ^ key[i];
    hash = hash * FNV_PRIME;
  }
  return hash;
}

uint64_t Meta::Sum64() const {
  const char* base = reinterpret_cast<const char*>(this);
  return fnv1a(const_cast<char*>(base),
               static_cast<int>(offsetof(Meta, checksum)));
}

}  // namespace boltdb