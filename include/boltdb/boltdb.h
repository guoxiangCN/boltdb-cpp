#ifndef __BOLTDB_BOLTDB_H__
#define __BOLTDB_BOLTDB_H__

#include <chrono>
#include <cstdint>
#include <vector>

#include "boltdb/slice.h"
#include "boltdb/status.h"

namespace boltdb {

using pgid_t = uint64_t;
using pgids_t = std::vector<pgid_t>;
using txid_t = uint64_t;

// All forward declares.
struct Meta;
struct BranchPageElement;
struct LeafPageElement;

// PageFlags defination.
static constexpr uint64_t kPageFlagBranch = 1;
static constexpr uint64_t kPageFlagLeaf = 1 << 1;
static constexpr uint64_t kPageFlagMeta = 1 << 2;
static constexpr uint64_t kPageFlagFreeList = 1 << 4;

enum class FreeListType {
  FreeListArray,
  FreeListHash,
};

/**
 * @brief Page is an page represention in disk.
 * ------------------------------------------------------------------
 * | pageid | flags | count | overflow |       ...pagedata...        |
 * ------------------------------------------------------------------
 * the pagedata's format was determinated by the page type.
 */
struct __attribute__((packed)) Page {
 public:
  pgid_t id;
  uint16_t flags;
  uint16_t count;
  uint32_t overflow;
  char data[0];

 public:
  std::string Type() const;
  Meta* AsMeta();
  void HexDump(int bytes) const;

  BranchPageElement* GetBranchPageElementAt(uint16_t index);
  std::vector<BranchPageElement*> GetBranchPageElements();

  LeafPageElement* GetLeafPageElementAt(uint16_t index);
  std::vector<LeafPageElement*> GetLeafPageElements();
};

/**
 * @brief An page element representation of bplus-tree's branch page.
 * 
 * BranchPage:
 * ----------------------------------------------------------------------------------
 * | PageHeader | BranchPageElement1 | BranchPageElement2 | BranchPageElement3 | ...
 * ----------------------------------------------------------------------------------
 * ------------------------------------------------------------
 * | pos(uint32) | keysz(uint32) | pgid(uint64) |  ...key...   |
 * ------------------------------------------------------------
 */
struct __attribute__((packed)) BranchPageElement {
  uint32_t pos;
  uint32_t ksize;
  pgid_t pgid;
  char data[0];

  Slice key();
};

/**
 * @brief An page element representation of bplus-tree's leaf page.
 * 
 * LeafPage:
 * ----------------------------------------------------------------------------------
 * | PageHeader | LeafPageElement1 | LeafPageElement2 | LeafPageElement3 |    ...   |
 * ----------------------------------------------------------------------------------
 * ----------------------------------------------------------------------------------
 * | flags(uint32) | pos(uint32) | ksize(uint32) | vsize(uint64) |  key.. | val..   |
 * ----------------------------------------------------------------------------------
 */
struct __attribute__((packed)) LeafPageElement {
  uint32_t flags;
  uint32_t pos;
  uint32_t ksize;
  uint32_t vsize;
  char data[0];

  Slice key();
  Slice value();
};

/**
 * @brief PageInfo represents human readable information about a page.
 */
struct PageInfo {
  int id;
  std::string type;
  int count;
  int overflowCount;
};

// bucket represents the on-file representation of a bucket.
// This is stored as the "value" of a bucket key. If the bucket is small enough,
// then its root page can be stored inline in the "value", after the bucket
// header. In the case of inline buckets, the "root" will be 0.
struct Bucket {
  pgid_t root;          // page id of the bucket's root-level page
  uint64_t sequence;  // monotonically incrementing, used by NextSequence()
};

struct __attribute__((packed)) Meta {
  uint32_t magic;
  uint32_t version;
  uint32_t pageSize;
  uint32_t flags;
  Bucket root;
  pgid_t freelist;
  pgid_t pgid;
  txid_t txid;
  uint64_t checksum;

  Status validate() const;
  void Write(Page* page);
  uint64_t Sum64() const;
};

constexpr uint64_t kMaxMmapStep = 1 << 30;
constexpr uint64_t kVersion = 2;
constexpr uint32_t kMagic = 0xED0CDAED;

constexpr uint64_t kPageHeaderSize = sizeof(Page);
constexpr uint64_t kMinKeysPerPage = 2;

struct Options {
  std::chrono::milliseconds timeout;
  bool noGrowSync;
  bool noFreeListSync;
  FreeListType freeListType;
  bool readOnly;
  int mmapFlags;
  int initialMmapSize;
  int pageSize;
  bool noSync;
  bool mlock;

  static Options Default() {
    return Options{
        .timeout = std::chrono::milliseconds(300),
        .noGrowSync = false,
        .noFreeListSync = false,
        .freeListType = FreeListType::FreeListHash,
        .readOnly = false,
        .pageSize = 4096,
        .noSync = false,
        .mlock = true,
    };
  }
};

}  // namespace boltdb

#endif