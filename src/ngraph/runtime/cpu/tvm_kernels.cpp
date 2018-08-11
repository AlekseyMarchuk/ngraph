/*******************************************************************************
* Copyright 2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <memory>
#include <string>

#include <dmlc/logging.h>
#include <gtest/gtest.h>
#include <topi/broadcast.h>
#include <topi/x86/default.h>
#include <tvm/build_module.h>
#include <tvm/operation.h>
#include <tvm/tvm.h>
#include <ngraph/util.hpp>

#include "tvm_kernels.hpp"

using namespace ngraph::runtime::cpu;

TVMInstance::TVMInstance()
{
    std::cout << "Creating TVMInstance" << std::endl;
    m_config = tvm::build_config();
    m_target = tvm::target::llvm();
    m_dl_ctx.device_type = static_cast<DLDeviceType>(kDLCPU);
    m_dl_ctx.device_id = 0;
}
TVMInstance::~TVMInstance()
{
}
DLTensor TVMInstance::create_dltensor(const DLDataType& type,
                                      const size_t ndim,
                                      tvm_index_t* shape,
                                      void* data)
{
    DLTensor t;
    t.ctx = m_dl_ctx;
    t.ndim = ndim;
    t.dtype = type;
    t.shape = static_cast<int64_t*>(shape);
    t.strides = nullptr;
    t.byte_offset = 0;
    t.data = data;
    return t;
}

static const DLDataType DLType_Float32{kDLFloat, 32, 1};

template <>
tvm::PackedFunc
    tvm_kernel::unary_elemwise_builder<float>(const std::unique_ptr<TVMInstance>& tvm_instance,
                                              const UnaryElemwiseFunc& topi_func)
{
    tvm::Var n("n");
    auto A = tvm::placeholder({n}, tvm::Float(32), "a");

    auto R = topi_func(A, "tensor", topi::kElementWise);

    std::unordered_map<tvm::Tensor, tvm::Buffer> binds;

    auto schedule = topi::x86::default_schedule(tvm_instance->target(), {R});
    auto lowered = tvm::lower(schedule, {A, R}, "func", binds, tvm_instance->config());
    auto module =
        tvm::build(lowered, tvm_instance->target(), tvm::Target(), tvm_instance->config());
    // store module to keep its lifetime
    tvm_instance->add_module(module);
    return module->GetFunction("func", false);
}

template <>
void tvm_kernel::unary_elemwise_kernel<float>(const std::unique_ptr<TVMInstance>& tvm_instance,
                                              const tvm::PackedFunc& func,
                                              void* input,
                                              void* output,
                                              size_t count)
{
    std::cout << "running tvm_kernel::unary_elemwise_kernel" << std::endl;
    int64_t dlshape[] = {static_cast<int64_t>(count)};
    DLTensor a = tvm_instance->create_dltensor(DLType_Float32, 1, dlshape, input);
    DLTensor r = tvm_instance->create_dltensor(DLType_Float32, 1, dlshape, output);

    func(&a, &r);
}

template <>
tvm::PackedFunc
    tvm_kernel::binary_elemwise_builder<float>(const std::unique_ptr<TVMInstance>& tvm_instance,
                                               const BinaryElemwiseFunc& topi_func)
{
    tvm::Var n("n");
    auto A = tvm::placeholder({n}, tvm::Float(32), "a");
    auto B = tvm::placeholder({n}, tvm::Float(32), "b");

    auto R = topi_func(A, B, "tensor", topi::kBroadcast);

    std::unordered_map<tvm::Tensor, tvm::Buffer> binds;

    auto schedule = topi::x86::default_schedule(tvm_instance->target(), {R});
    auto lowered = tvm::lower(schedule, {A, B, R}, "func", binds, tvm_instance->config());
    auto module =
        tvm::build(lowered, tvm_instance->target(), tvm::Target(), tvm_instance->config());
    // store module to keep its lifetime
    tvm_instance->add_module(module);
    return module->GetFunction("func", false);
}

template <>
void tvm_kernel::binary_elemwise_kernel<float>(const std::unique_ptr<TVMInstance>& tvm_instance,
                                               const tvm::PackedFunc& func,
                                               void* input0,
                                               void* input1,
                                               void* output,
                                               size_t count)
{
    std::cout << "running tvm_kernel::binary_elemwise_kernel" << std::endl;
    int64_t dlshape[] = {static_cast<int64_t>(count)};
    DLTensor a = tvm_instance->create_dltensor(DLType_Float32, 1, dlshape, input0);
    DLTensor b = tvm_instance->create_dltensor(DLType_Float32, 1, dlshape, input1);
    DLTensor r = tvm_instance->create_dltensor(DLType_Float32, 1, dlshape, output);

    func(&a, &b, &r);
}

template <>
tvm::PackedFunc tvm_kernel::relu_builder<float>(const std::unique_ptr<TVMInstance>& tvm_instance)
{
    tvm::Var n("n");
    auto A = tvm::placeholder({n}, tvm::Float(32), "a");

    auto R = topi::relu<float>(A);

    std::unordered_map<tvm::Tensor, tvm::Buffer> binds;

    auto schedule = topi::x86::default_schedule(tvm_instance->target(), {R});
    auto lowered = tvm::lower(schedule, {A, R}, "func", binds, tvm_instance->config());
    auto module =
        tvm::build(lowered, tvm_instance->target(), tvm::Target(), tvm_instance->config());
    // store module to keep its lifetime
    tvm_instance->add_module(module);
    return module->GetFunction("func", false);
}

template <>
tvm::PackedFunc tvm_kernel::transpose_builder<float>(const std::unique_ptr<TVMInstance>& tvm_instance,
                                  const size_t in_rank,
                                  const std::vector<size_t>& in_shape,
                                  const size_t out_rank,
                                  const std::vector<size_t>& axes)
{
    std::cout << in_rank << " " << out_rank << " axes: " << axes.size() << std::endl;

    tvm::Array<tvm::Expr> in_dlshape;
    for (size_t i = 0; i < in_rank; ++i) {
        tvm::Var n("n_"+std::to_string(i));
        in_dlshape.push_back(n);
    }
    std::cout << ngraph::vector_to_string(axes) << std::endl;
    tvm::Array<tvm::Expr> out_axes;
    for (size_t i = 0; i < out_rank; ++i) {
        std::cout << "axes[i]: " << axes[i] << std::endl;
        out_axes.push_back(axes[i]);
    }
    auto A = tvm::placeholder(in_dlshape, tvm::Float(32), "a");

    auto R = topi::transpose(A, out_axes);

    std::unordered_map<tvm::Tensor, tvm::Buffer> binds;

    auto schedule = topi::x86::default_schedule(tvm_instance->target(), {R});
    auto lowered = tvm::lower(schedule, {A, R}, "func", binds, tvm_instance->config());
    auto module =
        tvm::build(lowered, tvm_instance->target(), tvm::Target(), tvm_instance->config());
    // store module to keep its lifetime
    tvm_instance->add_module(module);
    return module->GetFunction("func", false);
}

template <>
void tvm_kernel::transpose_kernel<float>(const std::unique_ptr<TVMInstance>& tvm_instance,
                                         const tvm::PackedFunc& func,
                                         void* input,
                                         void* output,
                                         Shape input_shape,
                                         Shape output_shape)
{
    std::cout << "running tvm_kernel::transpose_kernel" << std::endl;

    std::cout << ngraph::vector_to_string(input_shape) << std::endl;
    std::cout << ngraph::vector_to_string(output_shape) << std::endl;
//    int64_t* in_dlshape = reinterpret_cast<int64_t*>(&input_shape[0]);
//    int64_t* out_dlshape = reinterpret_cast<int64_t*>(&output_shape[0]);

    std::vector<int64_t> in_dlshape(input_shape.size());
    for (int i = 0; i < input_shape.size(); ++i) {
      in_dlshape[i] = (int64_t)input_shape[i];
    }
    std::vector<int64_t> out_dlshape(output_shape.size());
    for (int i = 0; i < output_shape.size(); ++i) {
      out_dlshape[i] = (int64_t)output_shape[i];
    }
    std::cout << ngraph::vector_to_string(in_dlshape) << std::endl;
    std::cout << ngraph::vector_to_string(out_dlshape) << std::endl;

    DLTensor a = tvm_instance->create_dltensor(DLType_Float32, in_dlshape.size(), &in_dlshape[0], input);
    DLTensor r = tvm_instance->create_dltensor(DLType_Float32, out_dlshape.size(), &out_dlshape[0], output);

    func(&a, &r);
    std::cout << "tvm reshape output: " << std::endl;
    for (int i = 0; i < ngraph::shape_size(out_dlshape); ++i) {
      std::cout << static_cast<float*>(r.data)[i] << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < ngraph::shape_size(out_dlshape); ++i) {
      std::cout << static_cast<float*>(output)[i] << " ";
    }
    std::cout << std::endl;
}
