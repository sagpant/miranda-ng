//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/utils/ByteFlow.h"
#include "td/utils/Gzip.h"

#include <limits>

namespace td {

#if TD_HAVE_ZLIB
class GzipByteFlow final : public ByteFlowBase {
 public:
  GzipByteFlow() = default;

  explicit GzipByteFlow(Gzip::Mode mode) {
    gzip_.init(mode).ensure();
  }

  void init_decode() {
    gzip_.init_decode().ensure();
  }

  void init_encode() {
    gzip_.init_encode().ensure();
  }

  void set_max_output_size(size_t max_output_size) {
    max_output_size_ = max_output_size;
  }

  void loop() override;

 private:
  Gzip gzip_;
  size_t uncommited_size_ = 0;
  size_t total_output_size_ = 0;
  size_t max_output_size_ = std::numeric_limits<size_t>::max();
  static constexpr size_t MIN_UPDATE_SIZE = 1 << 14;
};
#endif

}  // namespace td