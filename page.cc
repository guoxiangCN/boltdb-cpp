#include "boltdb/boltdb.h"

namespace boltdb {

std::string Page::Type() const {
  if (flags & kPageFlagBranch) {
    return "branch";
  } else if (flags & kPageFlagLeaf) {
    return "leaf";
  } else if (flags & kPageFlagMeta) {
    return "meta";
  } else if (flags & kPageFlagFreeList) {
    return "freelist";
  } else {
    char buf[1024] = {'\0'};
    snprintf(buf, sizeof(buf), "unknown<%02x>", flags);
    return std::string(buf);
  }
}

Meta* Page::AsMeta() { return reinterpret_cast<Meta*>(this->data); }

void Page::HexDump(int bytes) const {
  std::string buf(reinterpret_cast<const char*>(this), bytes);
  fprintf(stderr, "%s\n", buf.c_str());
}

BranchPageElement* Page::GetBranchPageElementAt(uint16_t index) {
  auto* base = reinterpret_cast<BranchPageElement*>(data);
  return base + index;
}

std::vector<BranchPageElement*> Page::GetBranchPageElements() {
  std::vector<BranchPageElement*> vec;
  auto* base = reinterpret_cast<BranchPageElement*>(data);
  for (int i = 0; i < count; i++) {
    vec.push_back(base++);
  }
  return vec;
}

LeafPageElement* Page::GetLeafPageElementAt(uint16_t index) {
  auto* base = reinterpret_cast<LeafPageElement*>(data);
  return base + index;
}

std::vector<LeafPageElement*> Page::GetLeafPageElements() {
  std::vector<LeafPageElement*> vec;
  auto* base = reinterpret_cast<LeafPageElement*>(data);
  for (int i = 0; i < count; i++) {
    vec.push_back(base++);
  }
  return vec;
}

Slice BranchPageElement::key() {
  const char* ptr = reinterpret_cast<const char*>(this);
  ptr += pos;
  return Slice(ptr, ksize);
}

Slice LeafPageElement::key() {
  const char* ptr = reinterpret_cast<const char*>(this);
  ptr += pos;
  return Slice(ptr, ksize);
}

Slice LeafPageElement::value() {
  const char* ptr = reinterpret_cast<const char*>(this);
  ptr += pos;
  ptr += ksize;
  return Slice(ptr, vsize);
}

}  // namespace boltdb