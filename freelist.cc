#include "boltdb/boltdb.h"

namespace boltdb {

FreeList::FreeList(FreeListType typ) : freelist_type_(typ) {
  if (typ == FreeListType::FreeListHashMap) {
    this->allocate_fn_ = nullptr;
    this->free_count_fn_ = nullptr;
    this->merge_spans_fn_ = nullptr;
    this->get_freepageid_fn_ = nullptr;
    this->read_ids_fn_ = nullptr;
  } else {
    this->allocate_fn_ = nullptr;
    this->free_count_fn_ = nullptr;
    this->merge_spans_fn_ = nullptr;
    this->get_freepageid_fn_ = nullptr;
    this->read_ids_fn_ = nullptr;
  }
}

int FreeList::Size() {
  // TODO
  return 0;
}

int FreeList::Count() {
  int free_count = this->free_count_fn_();
  return free_count + this->PendingCount();
}

int FreeList::ArrayFreeCount() {
  // count of free pages (array version)
  return this->ids_.size();
}

int FreeList::PendingCount() {
  int count = 0;
  for (const auto& p : pending_) {
    count += p.second->ids_.size();
  }
  return count;
}

}  // namespace boltdb