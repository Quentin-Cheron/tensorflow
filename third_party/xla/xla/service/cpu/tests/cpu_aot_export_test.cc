/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
#include "xla/hlo/ir/hlo_module.h"
#include "xla/hlo/ir/hlo_module_group.h"
#include "xla/service/compiler.h"
#include "xla/service/executable.h"
#include "xla/service/platform_util.h"
#include "xla/stream_executor/multi_platform_manager.h"
#include "xla/stream_executor/platform.h"
#include "xla/stream_executor/stream_executor.h"
#include "xla/tests/hlo_test_base.h"
#include "tsl/platform/statusor.h"

namespace xla {
namespace cpu {

using CpuAotCompilationTest = HloTestBase;

TEST_F(CpuAotCompilationTest, ExportAndLoadExecutable) {
  const absl::string_view hlo_string = R"(
    HloModule Test

    ENTRY main {
      a = f32[2, 2]{1,0} parameter(0)
      ROOT b = f32[2, 2]{1,0} add(a, a)
    })";

  TF_ASSERT_OK_AND_ASSIGN(std::unique_ptr<HloModule> module,
                          ParseAndReturnVerifiedModule(hlo_string));

  auto compiler = backend().compiler();
  auto name = absl::AsciiStrToUpper(
      PlatformUtil::CanonicalPlatformName("host").value());
  TF_ASSERT_OK_AND_ASSIGN(se::Platform * platform,
                          se::MultiPlatformManager::PlatformWithName(name));
  TF_ASSERT_OK_AND_ASSIGN(se::StreamExecutor * stream_exec,
                          platform->ExecutorForDevice(0));

  // JIT compile executable
  auto module_group = std::make_unique<HloModuleGroup>(std::move(module));
  TF_ASSERT_OK_AND_ASSIGN(
      std::vector<std::unique_ptr<Executable>> executables,
      compiler->Compile(std::move(module_group), {{stream_exec}}, nullptr));

  TF_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<AotCompilationResult> exported_aot_result,
      compiler->Export(executables[0].get()));

  // Serialize-deserialize AOT compilation result.
  TF_ASSERT_OK_AND_ASSIGN(std::string serialized_aot_result,
                          exported_aot_result->SerializeAsString());
  TF_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<AotCompilationResult> loaded_aot_result,
      compiler->LoadAotCompilationResult(serialized_aot_result));

  // Load Executable from AOT compilation result.
  TF_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<Executable> executable,
      loaded_aot_result->LoadExecutable(compiler, stream_exec));
}

}  // namespace cpu
}  // namespace xla
