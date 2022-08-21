#ifndef __BOLTDB_BOLTDB_H__
#define __BOLTDB_BOLTDB_H__

#include <bitset>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "boltdb/noncopyable.h"
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
using txids_t = std::vector<txid_t>;

// All forward declares.
struct Meta;
struct BranchPageElement;
struct LeafPageElement;
struct BucketHdr;
struct Bucket;
class Tx;
class TxPending;
class FreeList;
class DB;
class Cursor;

// PageFlags defination.
static constexpr uint64_t kPageFlagBranch = 1;
static constexpr uint64_t kPageFlagLeaf = 1 << 1;
static constexpr uint64_t kPageFlagMeta = 1 << 2;
static constexpr uint64_t kPageFlagFreeList = 1 << 4;

enum class FreeListType {
  FreeListArray,
  FreeListHashMap,
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
struct dBucket {
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
  dBucket root;
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

static constexpr uint64_t kBucketHeaderSize = sizeof(dBucket);
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

// Tx represents a read-only or read/write transaction on the database.
// Read-only transactions can be used for retrieving values for keys and
// creating cursors. Read/write transactions can create and remove buckets and
// create and remove keys.
//
// IMPORTANT: You must commit or rollback transactions when you are done with
// them. Pages can not be reclaimed by the writer until no more transactions
// are using them. A long running read transaction can cause the database to
// quickly grow.
class Tx : public noncopyable {
 public:
  explicit Tx(DB* db);
  ~Tx();

  // ID returns the transaction id.
  int ID() const { return int(this->meta_->txid); }

  // DB returns a reference to the database that created the transaction.
  DB* GetDB() const { return this->db_; }

  // Size returns current database size in bytes as seen by this transaction.
  int64_t Size() const;

  // Writable returns whether the transaction can perform write operations.
  bool Writable() const { return writable_; }

  // Cursor creates a cursor associated with the root bucket.
  // All items in the cursor will return a nil value because all root bucket
  // keys point to buckets. The cursor is only valid as long as the transaction
  // is open. Do not use a cursor after the transaction is closed.
  Cursor* Cursor();

  // Stats retrieves a copy of the current transaction statistics.
  TxStats Stats() const { return stats_; }

  // Bucket retrieves a bucket by name.
  // Returns nil if the bucket does not exist.
  // The bucket instance is only valid for the lifetime of the transaction.
  Bucket* GetBucket(const std::string& name);

  // CreateBucket creates a new bucket.
  // Returns an error if the bucket already exists, if the bucket name is blank,
  // or if the bucket name is too long. The bucket instance is only valid for
  // the lifetime of the transaction.
  Status CreateBucket(const std::string& name, Bucket** b);

  // CreateBucketIfNotExists creates a new bucket if it doesn't already exist.
  // Returns an error if the bucket name is blank, or if the bucket name is too
  // long. The bucket instance is only valid for the lifetime of the
  // transaction.
  Status CreateBucketIfNotExists(const std::string& name, Bucket** b);

  // DeleteBucket deletes a bucket.
  // Returns an error if the bucket cannot be found or if the key represents a
  // non-bucket value.
  Status DeleteBucket(const std::string& name);

  // ForEach executes a function for each bucket in the root.
  // If the provided function returns an error then the iteration is stopped and
  // the error is returned to the caller.
  Status ForEach(std::function<Status(const std::string&, Bucket*)>);

  // OnCommit adds a handler function to be executed after the transaction
  // successfully commits.
  void OnCommit(std::function<void()>&& fn) {
    commit_handlers_.push_back(std::move(fn));
  }

  Status Commit();
  Status Rollback();

 private:
  Status commitFreeList();
  void rollback();
  void Close();

 private:
  bool writable_;
  bool managed_;
  DB* db_;
  Meta* meta_;
  // Bucket root;
  std::map<pgid_t, Page*> pages_;
  TxStats stats_;
  std::list<std::function<void()>> commit_handlers_;
  int write_flag_;
};

// txPending holds a list of pgids and corresponding allocation txns
// that are pending to be freed.
class TxPending {
 public:
  pgids_t ids_;
  txids_t alloc_txs_;
  txid_t last_release_begin_;
};

// freelist represents a list of all pages that are available for allocation.
// It also tracks pages that have been freed but are still in use by open
// transactions.
class FreeList : public noncopyable {
 public:
  using allocate_fn_t = pgid_t (*)(txid_t, int);
  using free_count_fn_t = int (*)();
  using merge_spans_fn_t = void (*)(pgidset_t);
  using get_freepageid_fn_t = pgids_t (*)();
  using readid_fn_t = void (*)(pgids_t);

 public:
  // Construct an empty but initialized freelist.
  explicit FreeList(FreeListType typ);
  // size returns the size of the page after serialization.
  int Size();
  // count returns count of pages on the freelist
  int Count();
  // arrayFreeCount returns count of free pages(array version)
  int ArrayFreeCount();
  // pending_count returns count of pending pages
  int PendingCount();

 private:
  FreeListType freelist_type_;
  pgids_t ids_;
  std::unordered_map<pgid_t, txid_t> allocs_;
  std::unordered_map<txid_t, TxPending*> pending_;
  std::unordered_map<pgid_t, bool> cache_;
  std::unordered_map<uint64_t, pgidset_t> freemaps_;
  std::unordered_map<pgid_t, uint64_t> forward_map_;
  std::unordered_map<pgid_t, uint64_t> backward_map_;

 public:
  allocate_fn_t allocate_fn_;
  free_count_fn_t free_count_fn_;
  merge_spans_fn_t merge_spans_fn_;
  get_freepageid_fn_t get_freepageid_fn_;
  readid_fn_t read_ids_fn_;
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

  static Options Default();
};

class Cursor {};

class DB : public noncopyable {
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
  friend class Tx;
  int page_size_;
};

}  // namespace boltdb

#endif