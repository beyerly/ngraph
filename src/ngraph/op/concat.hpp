//*****************************************************************************
// Copyright 2017-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#pragma once

#include <memory>

#include "ngraph/op/op.hpp"

namespace ngraph
{
    namespace op
    {
        /// \brief Concatenation operation.
        class Concat : public Op
        {
        public:
            /// \brief Constructs a concatenation operation.
            ///
            /// \param args               The nodes producing the input tensors.
            /// \param concatenation_axis The axis along which to concatenate the input tensors.
            Concat(const NodeVector& args, size_t concatenation_axis);

            void validate_and_infer_types() override;

            virtual std::shared_ptr<Node>
                copy_with_new_args(const NodeVector& new_args) const override;

            virtual std::vector<std::shared_ptr<op::Constant>> as_constants() const override;

            /// \return The concatenation axis.
            size_t get_concatenation_axis() const { return m_concatenation_axis; }
        protected:
            virtual void generate_adjoints(autodiff::Adjoints& adjoints,
                                           const NodeVector& deltas) override;
            const size_t m_concatenation_axis;
        };
    }
}
