// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#include "server/table.h"

#include "base/logging.h"

namespace dfly {

#define ADD(x) (x) += o.x

// It should be const, but we override this variable in our tests so that they run faster.
unsigned kInitSegmentLog = 3;

DbTableStats& DbTableStats::operator+=(const DbTableStats& o) {
  constexpr size_t kDbSz = sizeof(DbTableStats);
  static_assert(kDbSz == 64);

  ADD(inline_keys);
  ADD(obj_memory_usage);
  ADD(strval_memory_usage);
  ADD(update_value_amount);
  ADD(listpack_blob_cnt);
  ADD(listpack_bytes);
  ADD(external_entries);
  ADD(external_size);

  return *this;
}

DbTable::DbTable(std::pmr::memory_resource* mr)
    : prime(kInitSegmentLog, detail::PrimeTablePolicy{}, mr),
      expire(0, detail::ExpireTablePolicy{}, mr),
      mcflag(0, detail::ExpireTablePolicy{}, mr) {
}

DbTable::~DbTable() {
}

void DbTable::Clear() {
  prime.size();
  prime.Clear();
  expire.Clear();
  mcflag.Clear();
  stats = DbTableStats{};
}

void DbTable::Release(IntentLock::Mode mode, std::string_view key, unsigned count) {
  DVLOG(1) << "Release " << IntentLock::ModeName(mode) << " " << count << " for " << key;

  auto it = trans_locks.find(key);
  CHECK(it != trans_locks.end()) << key;
  it->second.Release(mode, count);
  if (it->second.IsFree()) {
    trans_locks.erase(it);
  }
}

}  // namespace dfly
