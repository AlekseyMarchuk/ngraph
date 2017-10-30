// ----------------------------------------------------------------------------
// Copyright 2017 Nervana Systems Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// ----------------------------------------------------------------------------

#include <memory>

#include "ngraph/ops/convert.hpp"
#include "ngraph/ops/greater.hpp"
#include "ngraph/ops/maximum.hpp"
#include "ngraph/ops/multiply.hpp"
#include "ngraph/types/element_type.hpp"

using namespace std;
using namespace ngraph;

void ngraph::op::Maximum::generate_adjoints(autodiff::Adjoints& adjoints,
                                            const std::shared_ptr<Node>& delta)
{
    auto x = m_arguments[0];
    auto y = m_arguments[1];

    adjoints.add_delta(x,
                       delta * make_shared<op::Convert>(make_shared<op::Greater>(x, y),
                                                        element::Float32::element_type()));
    adjoints.add_delta(y,
                       delta * make_shared<op::Convert>(make_shared<op::Greater>(y, x),
                                                        element::Float32::element_type()));
}