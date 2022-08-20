#ifndef __BOLTDB_BOLTDB_H__
#define __BOLTDB_BOLTDB_H__

#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "boltdb/slice.h"
#include "boltdb/status.h"

#ifndef BOLTDB_MAX_MMAP_SIZE
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || \
    defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64) ||     \
    defined(_M_ARM64) || defined(__aarch64__) || defined(__RISCV64__)
#define BOLTDB_MAX_MMAP_SIZE (0xFFFFFFFFFFFF)  // 256TB
#define BOLTDB_MAX_ALLOC_SIZE (0x7FFFFFFF)
#else
#define BOLTDB_MAX_MMAP_SIZE (0x7FFFFFFF)  // 2GB
#define BOLTDB_MAX_ALLOC_SIZE (0xFFFFFFF)
#endif
#endif

namespace boltdb {

using pgid_t = uint64_t;
using pgidset_t = std::set<pgid_t>;
using pgids_t = std::vector<pgid_t>;
using txid_t = uint64_t;

// All forward declares.
struct Meta;
struct BranchPageElement;
struct LeafPageElement;
struct BucketHdr;
struct Bucket;
struct Tx;

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
 * -----------------------------------------------------------------------------
 * | PageHeader | BranchPageElement1 | ... | BranchPageElementN | k1 |...| kN |
 * -----------------------------------------------------------------------------
 * ----------------------------------------------
 * | pos(uint32) | keysz(uint32) | pgid(uint64) |
 * ----------------------------------------------
 */
struct __attribute__((packed)) BranchPageElement {
  uint32_t pos;
  uint32_t ksize;
  pgid_t pgid;

  Slice key();
};

/**
 * @brief An page element representation of bplus-tree's leaf page.
 *
 * LeafPage:
 * --------------------------------------------------------------------------
 * | PageHeader | LeafPageElement1 | ... | LeafPageElementN |k1|v1|k2|v2|...|
 * --------------------------------------------------------------------------
 * ---------------------------------------------------------------
 * | flags(uint32) | pos(uint32) | ksize(uint32) | vsize(uint64) |
 * ---------------------------------------------------------------
 */
struct __attribute__((packed)) LeafPageElement {
  uint32_t flags;
  uint32_t pos;
  uint32_t ksize;
  uint32_t vsize;

  Slice key();
  Slice value();
};

// bucket represents the on-file representation of a bucket.
// This is stored as the "value" of a bucket key. If the bucket is small enough,
// then its root page can be stored inline in the "value", after the bucket
// header. In the case of inline buckets, the "root" will be 0.
struct BucketHdr {
  pgid_t root;        // page id of the bucket's root-level page
  uint64_t sequence;  // monotonically incrementing, used by NextSequence()
};

/**
 * @brief An page element representation of meta page.
 *
 * LeafPage:
 * ------------------------------------------------------------------
 * | PageHeader | MetaInfo |
 * ------------------------------------------------------------------
 * ------------------------------------------------------------------
 * |magic|version|pagesz|flags|buckethdr|freelist|pgid|txid|checksum|
 * ------------------------------------------------------------------
 */
struct __attribute__((packed)) Meta {
  uint32_t magic;
  uint32_t version;
  uint32_t page_size;
  uint32_t flags;
  BucketHdr root;
  pgid_t freelist;
  pgid_t pgid;
  txid_t txid;
  uint64_t checksum;

  Status Validate() const;
  void Copy(Meta* dest);
  void Write(Page* page);
  uint64_t Sum64() const;
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

static constexpr uint64_t kMaxMmapStep = 1 << 30;
static constexpr uint64_t kVersion = 2;
static constexpr uint32_t kMagic = 0xED0CDAED;

static constexpr uint64_t kPageHeaderSize = sizeof(Page);
static constexpr uint64_t kMinKeysPerPage = 2;

static constexpr uint64_t kMaxKeySize = 32768;
static constexpr uint64_t kMaxValueSize = ((uint64_t(1) << 31) - 2);

static constexpr uint64_t kBucketHeaderSize = sizeof(BucketHdr);
static constexpr double kMinBucketFillPercent = 0.1;
static constexpr double kMaxBucketFillPercent = 1.0;
static constexpr double kDefaultBucketFillPercent = 1.0;

struct TxStats {
  // Page statistics.
  int page_count;
  int page_alloc;

  // Cursor statistics.
  int cursor_count;

  // Node statistics.
  int node_count;
  int node_deref;

  // Rebalance statistics.
  int rebalance;
  std::chrono::nanoseconds rebalance_time;

  // Split/Spill statistics.
  int split;
  int spill;
  std::chrono::nanoseconds spill_time;

  // Write statistics.
  int write;
  std::chrono::nanoseconds write_time;

  void Add(const TxStats& other);
  TxStats Sub(const TxStats& other);
};

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

class DB {
 public:
  DB(const DB&) = delete;
  DB& operator=(const DB&) = delete;

  std::string Path() const;
  std::string String() const;
  std::string GoString() const;
  Status Sync();
  bool IsReadOnly() const;
  static Status Open(const std::string& path, Options& opts, DB** dbptr);

 private:
};

}  // namespace boltdb

#endif