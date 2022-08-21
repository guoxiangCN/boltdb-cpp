#ifndef __BOLTDB_NONCOPYABLE_H__
#define __BOLTDB_NONCOPYABLE_H__

namespace boltdb {

class noncopyable {
 public:
  noncopyable() {}
  virtual ~noncopyable() {}
#if __cplusplus >= 201103L
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;
#else
 private:
  noncopyable(const noncopyable&) {}
  noncopyable& operator=(const noncopyable&) { return *this; }
#endif
};
}  // namespace boltdb

#endif