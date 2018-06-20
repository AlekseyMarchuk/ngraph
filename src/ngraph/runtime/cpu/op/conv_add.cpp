/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
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

#include <numeric>

#include "ngraph/runtime/cpu/op/conv_add.hpp"

#include "ngraph/op/convolution.hpp"
#include "ngraph/util.hpp"

using namespace std;
using namespace ngraph;

op::ConvolutionAdd::ConvolutionAdd(const std::shared_ptr<op::Convolution>& conv,
                                   const std::shared_ptr<Node>& sum_input)
    : RequiresTensorViewArgs("ConvolutionAdd",
                             {conv->get_argument(0), conv->get_argument(1), sum_input})
    , m_window_movement_strides(conv->get_window_movement_strides())
    , m_window_dilation_strides(conv->get_window_dilation_strides())
    , m_padding_below(conv->get_padding_below())
    , m_padding_above(conv->get_padding_above())
    , m_data_dilation_strides(conv->get_data_dilation_strides())
{
    set_value_type_checked(conv->get_element_type(), conv->get_shape());
}

op::ConvolutionAdd::ConvolutionAdd(const std::shared_ptr<Node>& data_batch,
                                   const std::shared_ptr<Node>& filters,
                                   const std::shared_ptr<Node>& sum_input,
                                   const Strides& window_movement_strides,
                                   const Strides& window_dilation_strides,
                                   const CoordinateDiff& padding_below,
                                   const CoordinateDiff& padding_above,
                                   const Strides& data_dilation_strides)
    : RequiresTensorViewArgs("ConvolutionAdd", {data_batch, filters, sum_input})
    , m_window_movement_strides(window_movement_strides)
    , m_window_dilation_strides(window_dilation_strides)
    , m_padding_below(padding_below)
    , m_padding_above(padding_above)
    , m_data_dilation_strides(data_dilation_strides)
{
    auto& data_batch_shape = data_batch->get_shape();
    auto& data_batch_et = data_batch->get_element_type();
    auto& filters_shape = filters->get_shape();
    auto& filters_et = filters->get_element_type();

    //
    // Make sure data batch and filter element types match.
    //
    if (data_batch_et != filters_et)
    {
        throw ngraph_error("Convolution data batch and filter element types do not match");
    }

    set_value_type_checked(
        data_batch_et,
        util::infer_convolution_output_shape(data_batch_shape,
                                             filters_shape,
                                             window_movement_strides,
                                             window_dilation_strides,
                                             padding_below,
                                             padding_above,
                                             data_dilation_strides,
                                             0, /* batch_axis_data,              */
                                             1, /* input_channel_axis_data,      */
                                             1, /* input_channel_axis_filters,   */
                                             0, /* output_channel_axis_filters,  */
                                             0, /* batch_axis_result,            */
                                             1, /* output_channel_axis_result,   */
                                             ""));
}

std::shared_ptr<Node> op::ConvolutionAdd::copy_with_new_args(const NodeVector& new_args) const
{
    if (new_args.size() != 3)
    {
        throw ngraph_error("Incorrect number of new arguments");
    }

    return std::shared_ptr<Node>(new ConvolutionAdd(new_args.at(0),
                                                    new_args.at(1),
                                                    new_args.at(2),
                                                    get_window_movement_strides(),
                                                    get_window_dilation_strides(),
                                                    get_padding_below(),
                                                    get_padding_above(),
                                                    get_data_dilation_strides()));
}

op::ConvolutionBiasAdd::ConvolutionBiasAdd(const std::shared_ptr<op::ConvolutionBias>& conv,
                                           const std::shared_ptr<Node>& sum_input)
    : RequiresTensorViewArgs(
          "ConvolutionBiasAdd",
          {conv->get_argument(0), conv->get_argument(1), conv->get_argument(2), sum_input})
    , m_window_movement_strides(conv->get_window_movement_strides())
    , m_window_dilation_strides(conv->get_window_dilation_strides())
    , m_padding_below(conv->get_padding_below())
    , m_padding_above(conv->get_padding_above())
    , m_data_dilation_strides(conv->get_data_dilation_strides())
{
    set_value_type_checked(conv->get_element_type(), conv->get_shape());
}

op::ConvolutionBiasAdd::ConvolutionBiasAdd(const std::shared_ptr<Node>& data_batch,
                                           const std::shared_ptr<Node>& filters,
                                           const std::shared_ptr<Node>& bias,
                                           const std::shared_ptr<Node>& sum_input,
                                           const Strides& window_movement_strides,
                                           const Strides& window_dilation_strides,
                                           const CoordinateDiff& padding_below,
                                           const CoordinateDiff& padding_above,
                                           const Strides& data_dilation_strides)
    : RequiresTensorViewArgs("ConvolutionBiasAdd", {data_batch, filters, bias, sum_input})
    , m_window_movement_strides(window_movement_strides)
    , m_window_dilation_strides(window_dilation_strides)
    , m_padding_below(padding_below)
    , m_padding_above(padding_above)
    , m_data_dilation_strides(data_dilation_strides)
{
    auto& data_batch_shape = data_batch->get_shape();
    auto& data_batch_et = data_batch->get_element_type();
    auto& filters_shape = filters->get_shape();
    auto& filters_et = filters->get_element_type();

    //
    // Make sure data batch and filter element types match.
    //
    if (data_batch_et != filters_et)
    {
        throw ngraph_error("Convolution data batch and filter element types do not match");
    }

    set_value_type_checked(
        data_batch_et,
        util::infer_convolution_output_shape(data_batch_shape,
                                             filters_shape,
                                             window_movement_strides,
                                             window_dilation_strides,
                                             padding_below,
                                             padding_above,
                                             data_dilation_strides,
                                             0, /* batch_axis_data,              */
                                             1, /* input_channel_axis_data,      */
                                             1, /* input_channel_axis_filters,   */
                                             0, /* output_channel_axis_filters,  */
                                             0, /* batch_axis_result,            */
                                             1, /* output_channel_axis_result,   */
                                             ""));
}

std::shared_ptr<Node> op::ConvolutionBiasAdd::copy_with_new_args(const NodeVector& new_args) const
{
    if (new_args.size() != 4)
    {
        throw ngraph_error("Incorrect number of new arguments");
    }

    return std::shared_ptr<Node>(new ConvolutionBiasAdd(new_args.at(0),
                                                        new_args.at(1),
                                                        new_args.at(2),
                                                        new_args.at(3),
                                                        get_window_movement_strides(),
                                                        get_window_dilation_strides(),
                                                        get_padding_below(),
                                                        get_padding_above(),
                                                        get_data_dilation_strides()));
}