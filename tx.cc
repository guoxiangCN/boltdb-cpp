#include "boltdb/boltdb.h"

namespace boltdb {

void TxStats::Add(const TxStats& other) {
  this->page_count += other.page_count;
  this->page_alloc += other.page_alloc;
  this->cursor_count += other.cursor_count;
  this->node_count += other.node_count;
  this->node_deref += other.node_deref;
  this->rebalance += other.rebalance;
  this->rebalance_time += other.rebalance_time;
  this->split += other.split;
  this->spill += other.spill;
  this->spill_time += other.spill_time;
  this->write += other.write;
  this->write_time += other.write_time;
}

TxStats TxStats::Sub(const TxStats& other) {
  TxStats diff;
  diff.page_count = this->page_count - other.page_count;
  diff.page_alloc = this->page_alloc - other.page_alloc;
  diff.cursor_count = this->cursor_count - other.cursor_count;
  diff.node_count = this->node_count - other.node_count;
  diff.node_deref = this->node_deref - other.node_deref;
  diff.rebalance = this->rebalance - other.rebalance;
  diff.rebalance_time = this->rebalance_time - other.rebalance_time;
  diff.split = this->split - other.split;
  diff.spill = this->spill - other.spill;
  diff.spill_time = this->spill_time - other.spill_time;
  diff.write = this->write - other.write;
  diff.write_time = this->write_time - other.write_time;
  return diff;
}

int64_t Tx::Size() const {
    // TODO
    return 0;
}

}  // namespace boltdb