/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef XLA_SERVICE_GPU_REDUCTION_SPLITTER_H_
#define XLA_SERVICE_GPU_REDUCTION_SPLITTER_H_

#include "xla/hlo/ir/hlo_module.h"
#include "xla/service/hlo_pass_interface.h"

namespace xla {
namespace gpu {

// Splits a reduce op into two consecutive reduce ops if the reduce dimensions
// are not contiguous. Ignores small reduce dimensions if `ignore_small_dims` is
// set.
//
// Reductions with non-contiguous dimensions are emitted as simple element-wise
// loops. This is inefficient when reducing large input shape dimensions.
// Splitting such reductions allows using more efficient reduction emitters.
//
// This pass splits reduce ops into two consecutive reduce ops. Run it to a
// fixpoint to split reduce ops along multiple dimensions.
//
// Precondition: ReductionDimensionGrouper has been run and adjacent reduce
// dimentsions have been grouped. Reduction layouts have been normalized.

class ReductionSplitter : public HloModulePass {
 public:
  explicit ReductionSplitter(bool ignore_small_dims)
      : ignore_small_dims_(ignore_small_dims) {}
  absl::string_view name() const override { return "reduction-splitter"; }

  using HloPassInterface::Run;
  absl::StatusOr<bool> Run(
      HloModule* module,
      const absl::flat_hash_set<absl::string_view>& execution_threads) override;

 private:
  bool ignore_small_dims_;
};

}  // namespace gpu
}  // namespace xla

#endif  // XLA_SERVICE_GPU_REDUCTION_SPLITTER_H_
