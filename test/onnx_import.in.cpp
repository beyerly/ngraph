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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "gtest/gtest.h"
#include "ngraph/frontend/onnx_import/onnx.hpp"
#include "ngraph/ngraph.hpp"
#include "util/all_close.hpp"
#include "util/all_close_f.hpp"
#include "util/ndarray.hpp"
#include "util/test_control.hpp"
#include "util/test_tools.hpp"

using namespace ngraph;

static std::string s_manifest = "${MANIFEST}";

using Inputs = std::vector<std::vector<float>>;
using Outputs = std::vector<std::vector<float>>;

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_output_names_check)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/split_equal_parts_default.prototxt"));

    std::size_t size = function->get_output_size();
    for (std::size_t i{0}; i < size; ++i)
    {
        std::shared_ptr<Node> node = function->get_output_op(i);
        EXPECT_EQ(node->get_friendly_name(), "output_" + std::to_string(i + 1));
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_add_abc)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/add_abc.prototxt"));

    Inputs inputs{{1}, {2}, {3}};
    Outputs expected_outputs{{6}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_binary_add_abc)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/add_abc.onnx"));

    Inputs inputs{{1}, {2}, {3}};
    Outputs expected_outputs{{6}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_add_abc_initializers)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/add_abc_initializers.prototxt"));

    Inputs inputs{{1, 2, 3, 4}};
    Outputs expected_outputs{{3, 6, 9, 12}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_addmul_abc)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/addmul_abc.prototxt"));

    std::vector<std::vector<float>> inputs;

    Shape shape{1, 2, 2};
    inputs.emplace_back(test::NDArray<float, 3>({{{9, 10}}, {{11, 12}}}).get_vector());
    inputs.emplace_back(test::NDArray<float, 3>({{{5, 6}}, {{7, 8}}}).get_vector());
    inputs.emplace_back(test::NDArray<float, 3>({{{1, 2}}, {{3, 4}}}).get_vector());

    auto expected_output = test::NDArray<float, 3>({{{46, 62}}, {{80, 100}}}).get_vector();

    auto result_vectors = execute(function, inputs, "${BACKEND_NAME}");
    EXPECT_TRUE(test::all_close_f(expected_output, result_vectors.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_argmin_no_keepdims)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/argmin_no_keepdims.prototxt"));

    Inputs inputs{test::NDArray<float, 2>{{2, 1}, {3, 10}}.get_vector()};
    std::vector<std::vector<int64_t>> expected_output{{1, 0}};
    std::vector<std::vector<int64_t>> result{
        execute<float, int64_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_EQ(expected_output, result);
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_split_equal_parts_default)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/split_equal_parts_default.prototxt"));

    Inputs inputs{{1, 2, 3, 4, 5, 6}};
    Outputs expected_outputs{{1, 2}, {3, 4}, {5, 6}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_EQ(outputs.size(), expected_outputs.size());

    for (std::size_t i = 0; i < expected_outputs.size(); ++i)
    {
        EXPECT_EQ(outputs[i].size(), expected_outputs[i].size());
        EXPECT_TRUE(test::all_close_f(outputs[i], expected_outputs[i]));
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_split_equal_parts_2d)
{
    // Split into 2 equal parts along axis=1
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/split_equal_parts_2d.prototxt"));

    Inputs inputs{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    Outputs expected_outputs{{0, 1, 2, 6, 7, 8}, {3, 4, 5, 9, 10, 11}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_EQ(outputs.size(), expected_outputs.size());

    for (std::size_t i = 0; i < expected_outputs.size(); ++i)
    {
        EXPECT_EQ(outputs[i].size(), expected_outputs[i].size());
        EXPECT_TRUE(test::all_close_f(outputs[i], expected_outputs[i]));
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_split_variable_parts_2d)
{
    // Split into variable parts {2, 4} along axis=1
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/split_variable_parts_2d.prototxt"));

    Inputs inputs{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};
    Outputs expected_outputs{{0, 1, 6, 7}, {2, 3, 4, 5, 8, 9, 10, 11}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_EQ(outputs.size(), expected_outputs.size());

    for (std::size_t i = 0; i < expected_outputs.size(); ++i)
    {
        EXPECT_EQ(outputs[i].size(), expected_outputs[i].size());
        EXPECT_TRUE(test::all_close_f(outputs[i], expected_outputs[i]));
    }
}

namespace
{
    std::vector<std::vector<float>> conv2d_execute(const std::shared_ptr<Function>& function)
    {
        std::vector<std::vector<float>> args;

        // data (1, 1, 7, 5) input tensor
        args.emplace_back(test::NDArray<float, 4>{{{{{0.f, 1.f, 2.f, 3.f, 4.f},
                                                     {5.f, 6.f, 7.f, 8.f, 9.f},
                                                     {10.f, 11.f, 12.f, 13.f, 14.f},
                                                     {15.f, 16.f, 17.f, 18.f, 19.f},
                                                     {20.f, 21.f, 22.f, 23.f, 24.f},
                                                     {25.f, 26.f, 27.f, 28.f, 29.f},
                                                     {30.f, 31.f, 32.f, 33.f, 34.f}}}}}
                              .get_vector());

        // filters (1, 1, 3, 3) aka convolution weights
        args.emplace_back(
            test::NDArray<float, 4>{{{{{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}}}}}
                .get_vector());

        return execute(function, args, "${BACKEND_NAME}");
    }

} // namespace

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_conv2d_strides_padding)
{
    // Convolution with strides=2 and padding=1
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/conv_with_strides_padding.prototxt"));

    // (1, 1, 4, 3)
    auto expected_output = test::NDArray<float, 4>({{{{12.f, 27.f, 24.f},
                                                      {63.f, 108.f, 81.f},
                                                      {123.f, 198.f, 141.f},
                                                      {112.f, 177.f, 124.f}}}})
                               .get_vector();

    auto result = conv2d_execute(function);
    EXPECT_EQ(expected_output, result.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_conv2d_strides_no_padding)
{
    // Convolution with strides=2 and padding=1
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/conv_with_strides_no_padding.prototxt"));

    // (1, 1, 3, 2)
    auto expected_output =
        test::NDArray<float, 4>({{{{54.f, 72.f}, {144.f, 162.f}, {234.f, 252.f}}}}).get_vector();

    auto result = conv2d_execute(function);
    EXPECT_EQ(expected_output, result.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_conv2d_strides_assymetric_padding)
{
    // Convolution with strides=2 and padding=1
    auto function = onnx_import::import_onnx_model(file_util::path_join(
        SERIALIZED_ZOO, "onnx/conv_with_strides_and_asymmetric_padding.prototxt"));

    // (1, 1, 4, 2)
    auto expected_output =
        test::NDArray<float, 4>({{{{21.f, 33.f}, {99.f, 117.f}, {189.f, 207.f}, {171.f, 183.f}}}})
            .get_vector();

    auto result = conv2d_execute(function);
    EXPECT_EQ(expected_output, result.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_average_pool_2d)
{
    // Pooling with strides=2 and no padding
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/average_pool_2d.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs;
    inputs.push_back(test::NDArray<float, 4>({{{{0.f, 1.f, 2.f, 3.f},
                                                {4.f, 5.f, 6.f, 7.f},
                                                {8.f, 9.f, 10.f, 11.f},
                                                {12.f, 13.f, 14.f, 15.f}}}})
                         .get_vector());

    // (1, 1, 2, 2)
    auto expected_output = test::NDArray<float, 4>({{{{2.5f, 4.5f}, {10.5f, 12.5f}}}}).get_vector();

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_EQ(expected_output, outputs.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_average_pool_2d_pads)
{
    // Pooling with strides=2 and padding=1
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/average_pool_2d_pads.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs;
    inputs.push_back(test::NDArray<float, 4>({{{{0.f, 1.f, 2.f, 3.f},
                                                {4.f, 5.f, 6.f, 7.f},
                                                {8.f, 9.f, 10.f, 11.f},
                                                {12.f, 13.f, 14.f, 15.f}}}})
                         .get_vector());

    // (1, 1, 3, 3)
    auto expected_output =
        test::NDArray<float, 4>({{{{0.f, 1.5f, 3.f}, {6.f, 7.5f, 9.f}, {12.f, 13.5f, 15.f}}}})
            .get_vector();

    Outputs outputs = execute(function, inputs, "${BACKEND_NAME}");

    EXPECT_EQ(expected_output, outputs.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_max_pool_2d_pads)
{
    // Pooling with strides=2 and padding=1
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/max_pool_2d_pads.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs;
    inputs.push_back(test::NDArray<float, 4>({{{{0.f, 1.f, 2.f, 3.f},
                                                {4.f, 5.f, 6.f, 7.f},
                                                {8.f, 9.f, 10.f, 11.f},
                                                {12.f, 13.f, 14.f, 15.f}}}})
                         .get_vector());

    // (1, 1, 3, 3)
    auto expected_output =
        test::NDArray<float, 4>({{{{0.f, 2.f, 3.f}, {8.f, 10.f, 11.f}, {12.f, 14.f, 15.f}}}})
            .get_vector();

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_EQ(expected_output, outputs.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_batchnorm_default)
{
    // Batch Normalization with default parameters
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/batchnorm_default.prototxt"));

    Inputs inputs;

    // input data shape (1, 2, 1, 3)
    inputs.push_back(
        test::NDArray<float, 4>({{{{-1.f, 0.f, 1.f}}, {{2.f, 3.f, 4.f}}}}).get_vector());

    // scale (3)
    inputs.emplace_back(std::vector<float>{1.f, 1.5f});
    // bias (3)
    inputs.emplace_back(std::vector<float>{0.f, 1.f});
    // mean (3)
    inputs.emplace_back(std::vector<float>{0.f, 3.f});
    // var (3)
    inputs.emplace_back(std::vector<float>{1.f, 1.5f});

    // shape (1, 2, 1, 3)
    Outputs expected_outputs{test::NDArray<float, 4>{
        {{{{-0.999995f, 0.f, 0.999995f}}, {{-0.22474074f, 1.f, 2.2247407f}}}}}
                                 .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_relu)
{
    // Simple ReLU test
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/relu.prototxt"));

    Inputs inputs{{-1, -2, 0, 1, 2, 3}};
    Outputs expected_outputs{{0, 0, 0, 1, 2, 3}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sum)
{
    // Simple Sum test
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/sum.prototxt"));

    // input data shape (3, )
    Inputs inputs;
    inputs.emplace_back(std::vector<float>{3.f, 0.f, 2.f});
    inputs.emplace_back(std::vector<float>{1.f, 3.f, 4.f});
    inputs.emplace_back(std::vector<float>{2.f, 6.f, 6.f});

    Outputs expected_outputs{{6.f, 9.f, 12.f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sum_one_input)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/sum_one_input.prototxt"));

    // input data shape (3, )
    Inputs inputs{{3.f, 0.f, 2.f}};
    Outputs expected_outputs{{3.f, 0.f, 2.f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_min_two_inputs)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/min_two_inputs.prototxt"));

    // input data shape (3, )
    Inputs inputs;
    inputs.emplace_back(std::vector<float>{1.f, 2.f, 1.f});
    inputs.emplace_back(std::vector<float>{1.f, 4.f, 4.f});

    Outputs expected_outputs{{1.f, 2.f, 1.f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_max)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/max.prototxt"));

    // input data shape (3, )
    Inputs inputs;
    inputs.emplace_back(std::vector<float>{3.f, 2.f, 1.f});
    inputs.emplace_back(std::vector<float>{1.f, 4.f, 4.f});
    inputs.emplace_back(std::vector<float>{2.f, 5.f, 3.f});

    Outputs expected_outputs{{3.f, 5.f, 4.f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_mean)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/mean.prototxt"));

    // input data shape (3, )
    Inputs inputs;
    inputs.emplace_back(std::vector<float>{3.f, 0.f, 2.f});
    inputs.emplace_back(std::vector<float>{1.f, 3.f, 4.f});
    inputs.emplace_back(std::vector<float>{2.f, 6.f, 6.f});

    Outputs expected_outputs{{2.f, 3.f, 4.f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_gemm_abc)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/gemm_abc.prototxt"));

    Inputs inputs;
    inputs.emplace_back(test::NDArray<float, 2>(
                            {{1, 2, 3, 4, 5, 6}, {7, 8, 9, 10, 11, 12}, {13, 14, 15, 16, 17, 18}})
                            .get_vector());

    inputs.emplace_back(test::NDArray<float, 2>({{19, 20, 21, 22},
                                                 {23, 24, 25, 26},
                                                 {27, 28, 29, 30},
                                                 {31, 32, 33, 34},
                                                 {35, 36, 37, 38},
                                                 {39, 40, 41, 42}})
                            .get_vector());

    inputs.emplace_back(
        test::NDArray<float, 2>({{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}).get_vector());

    Outputs expected_outputs{
        test::NDArray<float, 2>(
            {{340, 350.5, 361, 371.5}, {862, 890.5, 919, 947.5}, {1384, 1430.5, 1477, 1523.5}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_matmul)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/matmul.prototxt"));

    std::vector<std::vector<float>> inputs;

    inputs.emplace_back(
        test::NDArray<float, 2>({{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}).get_vector());

    inputs.emplace_back(
        test::NDArray<float, 2>({{13, 14, 15}, {16, 17, 18}, {19, 20, 21}, {22, 23, 24}})
            .get_vector());

    Outputs expected_outputs{
        test::NDArray<float, 2>({{190, 200, 210}, {470, 496, 522}, {750, 792, 834}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_softmax)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/softmax.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {16, 17, 18, 19, 20}},

             {{21, 22, 23, 24, 25},
              {26, 27, 28, 29, 30},
              {31, 32, 33, 34, 35},
              {36, 37, 38, 39, 40}},

             {{41, 42, 43, 44, 45},
              {46, 47, 48, 49, 50},
              {51, 52, 53, 54, 55},
              {56, 57, 58, 59, 60}}})
            .get_vector());

    auto expected_output =
        test::NDArray<float, 3>(
            {{{1.50461533e-26f, 4.08996852e-26f, 1.11176871e-25f, 3.02210068e-25f, 8.21492137e-25f},
              {2.23304715e-24f, 6.07005148e-24f, 1.65001106e-23f, 4.48519509e-23f, 1.21920243e-22f},
              {3.31413582e-22f, 9.00875516e-22f, 2.44883355e-21f, 6.65661973e-21f, 1.80945684e-20f},
              {4.91861366e-20f,
               1.33701781e-19f,
               3.63439123e-19f,
               9.87929963e-19f,
               2.68547207e-18f}},

             {{7.29986992e-18f, 1.98431037e-17f, 5.39391483e-17f, 1.46621807e-16f, 3.98559393e-16f},
              {1.08339676e-15f, 2.94497771e-15f, 8.00527940e-15f, 2.17606055e-14f, 5.91514586e-14f},
              {1.60790335e-13f, 4.37073446e-13f, 1.18808881e-12f, 3.22956021e-12f, 8.77885484e-12f},
              {2.38634016e-11f,
               6.48674509e-11f,
               1.76328013e-10f,
               4.79309234e-10f,
               1.30289758e-09f}},

             {{3.54164282e-09f, 9.62718331e-09f, 2.61693974e-08f, 7.11357975e-08f, 1.93367146e-07f},
              {5.25626399e-07f, 1.42880069e-06f, 3.88388295e-06f, 1.05574884e-05f, 2.86982290e-05f},
              {7.80098743e-05f, 2.12052824e-04f, 5.76419338e-04f, 1.56687021e-03f, 4.25919482e-03f},
              {1.15776919e-02f,
               3.14714295e-02f,
               8.55482149e-02f,
               2.32544158e-01f,
               6.32120559e-01f}}})
            .get_vector();

    auto result_vectors = execute(function, inputs, "${BACKEND_NAME}");
    EXPECT_TRUE(test::all_close_f(expected_output, result_vectors.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_concat)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/concat.prototxt"));

    Inputs inputs;

    inputs.emplace_back(test::NDArray<float, 1>({1, 2}).get_vector());
    inputs.emplace_back(test::NDArray<float, 1>({3, 4}).get_vector());

    Outputs expected_outputs{test::NDArray<float, 1>({1, 2, 3, 4}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_flatten)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/flatten.prototxt"));

    Inputs inputs;

    inputs.emplace_back(
        test::NDArray<float, 4>({{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}}}).get_vector());

    Outputs expected_outputs{test::NDArray<float, 3>({{{1, 2, 3, 4}, {5, 6, 7, 8}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sub)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/sub.prototxt"));

    Inputs inputs;
    inputs.emplace_back(test::NDArray<float, 3>({{{1, 2, 3}}}).get_vector());

    inputs.emplace_back(test::NDArray<float, 3>({{{4, 5, 7}}}).get_vector());

    auto expected_output = test::NDArray<float, 3>({{{-3, -3, -4}}}).get_vector();

    auto result_vectors = execute(function, inputs, "${BACKEND_NAME}");
    EXPECT_TRUE(test::all_close_f(expected_output, result_vectors.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_unsqueeze)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/unsqueeze.prototxt"));

    Inputs inputs;
    inputs.emplace_back(test::NDArray<float, 3>(
                            {{{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
                             {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
                             {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}}})
                            .get_vector());

    Outputs expected_output{
        test::NDArray<float, 4>(
            {{{{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
              {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
              {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_squeeze)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/squeeze_duplicate_axes.prototxt"));

    // {1, 4, 1, 1, 2}
    Inputs inputs{test::NDArray<float, 5>(
                      {{{{{1.0f, 2.0f}}}, {{{3.0f, 4.0f}}}, {{{5.0f, 6.0f}}}, {{{7.0f, 8.0f}}}}})
                      .get_vector()};

    // {4, 2}
    Outputs expected_output{
        test::NDArray<float, 2>({{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}, {7.0f, 8.0f}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_div)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/div.prototxt"));

    Inputs inputs;
    inputs.emplace_back(test::NDArray<float, 3>({{{1, 2, 3}}}).get_vector());

    inputs.emplace_back(test::NDArray<float, 3>({{{1, 4, 12}}}).get_vector());

    auto expected_output = test::NDArray<float, 3>({{{1, 0.5, 0.25}}}).get_vector();

    auto result_vectors = execute(function, inputs, "${BACKEND_NAME}");
    EXPECT_TRUE(test::all_close_f(expected_output, result_vectors.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_add_bcast)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/add_bcast.prototxt"));

    Inputs inputs;
    inputs.emplace_back(test::NDArray<float, 3>(
                            {{{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
                             {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
                             {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}}})
                            .get_vector());

    inputs.emplace_back(test::NDArray<float, 1>({1, 2, 3, 4, 5}).get_vector());

    Outputs expected_output{
        test::NDArray<float, 4>(
            {{{{2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}},
              {{2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}},
              {{2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}, {2, 3, 4, 5, 6}}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_reduced_dims)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_reduced_dims.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (2, 12)
    Outputs expected_outputs{
        test::NDArray<float, 2>({{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
                                 {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_reordered_dims)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_reordered_dims.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (4, 2, 3)
    Outputs expected_outputs{test::NDArray<float, 3>({{{0, 1, 2}, {3, 4, 5}},
                                                      {{6, 7, 8}, {9, 10, 11}},
                                                      {{12, 13, 14}, {15, 16, 17}},
                                                      {{18, 19, 20}, {21, 22, 23}}})
                                 .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_extended_dims)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_extended_dims.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (3, 2, 2, 2)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{0, 1}, {2, 3}}, {{4, 5}, {6, 7}}},
                                                      {{{8, 9}, {10, 11}}, {{12, 13}, {14, 15}}},
                                                      {{{16, 17}, {18, 19}}, {{20, 21}, {22, 23}}}})
                                 .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_single_dim)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_single_dim.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (24, )
    Outputs expected_outputs{
        test::NDArray<float, 1>(
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_negative_dim)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_negative_dim.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (6, 2, 2)
    Outputs expected_outputs{test::NDArray<float, 3>({{{0, 1}, {2, 3}},
                                                      {{4, 5}, {6, 7}},
                                                      {{8, 9}, {10, 11}},
                                                      {{12, 13}, {14, 15}},
                                                      {{16, 17}, {18, 19}},
                                                      {{20, 21}, {22, 23}}})
                                 .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_negative_with_zero_dim)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_negative_with_zero_dims.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (2, 6, 2)
    Outputs expected_outputs{
        test::NDArray<float, 3>({{{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}},
                                 {{12, 13}, {14, 15}, {16, 17}, {18, 19}, {20, 21}, {22, 23}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reshape_output_shape_as_input)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reshape_output_shape_as_input.prototxt"));

    // input data shape (2, 3, 4)
    Inputs inputs{test::NDArray<float, 3>({{{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}},
                                           {{12, 13, 14, 15}, {16, 17, 18, 19}, {20, 21, 22, 23}}})
                      .get_vector()};

    // output data shape (2, 6, 2)
    Outputs expected_outputs{
        test::NDArray<float, 3>({{{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}},
                                 {{12, 13}, {14, 15}, {16, 17}, {18, 19}, {20, 21}, {22, 23}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_log_sum)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_log_sum.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{2.77258872f}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_log_sum_exp)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_log_sum_exp.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{3.77258872f}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_l1)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_l1.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{16}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_l2)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_l2.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{4}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_max)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_max.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{16}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_mean)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_mean.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{1}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_min)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_min.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{1}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_prod)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_prod.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{1}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_sum)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_sum.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{16}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_reduce_sum_square)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/reduce_sum_square.prototxt"));

    // input data shape (1, 1, 4, 4)
    Inputs inputs{
        test::NDArray<float, 4>({{{{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}}})
            .get_vector()};

    // output data shape (1,)
    Outputs expected_outputs{test::NDArray<float, 4>({{{{16}}}}).get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_shape)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/shape.prototxt"));

    Inputs inputs;
    inputs.emplace_back(test::NDArray<float, 3>(
                            {{{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
                             {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}},
                             {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}}})
                            .get_vector());

    std::vector<std::vector<int64_t>> expected_output{{3, 4, 5}};

    std::vector<std::vector<int64_t>> outputs =
        execute<float, int64_t>(function, inputs, "${BACKEND_NAME}");
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_elu)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/elu.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    Outputs expected_output{test::NDArray<float, 3>({{{-1.999753180391830f,
                                                       -1.999329074744190f,
                                                       -1.998176236068890f,
                                                       -1.995042495646670f,
                                                       -1.986524106001830f},
                                                      {-1.963368722222530f,
                                                       -1.900425863264270f,
                                                       -1.729329433526770f,
                                                       -1.264241117657120f,
                                                       0},
                                                      {1, 2, 3, 4, 5},
                                                      {6, 7, 8, 9, 10}},
                                                     {{-1.963368722222530f,
                                                       -1.900425863264270f,
                                                       -1.729329433526770f,
                                                       -1.264241117657120f,
                                                       0},
                                                      {1, 2, 3, 4, 5},
                                                      {6, 7, 8, 9, 10},
                                                      {11, 12, 13, 14, 15}},
                                                     {{1, 1, 1, 1, 1},
                                                      {-1.264241117657120f,
                                                       -1.264241117657120f,
                                                       -1.264241117657120f,
                                                       -1.264241117657120f,
                                                       -1.264241117657120f},
                                                      {0, 0, 0, 0, 0},
                                                      {2, 2, 2, 2, 2}}})
                                .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_leaky_relu)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/leaky_relu.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    Outputs expected_output{test::NDArray<float, 3>({{{-0.9f, -0.8f, -0.7f, -0.6f, -0.5f},
                                                      {-0.4f, -0.3f, -0.2f, -0.1f, 0},
                                                      {1, 2, 3, 4, 5},
                                                      {6, 7, 8, 9, 10}},
                                                     {{-0.4f, -0.3f, -0.2f, -0.1f, 0},
                                                      {1, 2, 3, 4, 5},
                                                      {6, 7, 8, 9, 10},
                                                      {11, 12, 13, 14, 15}},
                                                     {{1, 1, 1, 1, 1},
                                                      {-0.1f, -0.1f, -0.1f, -0.1f, -0.1f},
                                                      {0, 0, 0, 0, 0},
                                                      {2, 2, 2, 2, 2}}})
                                .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, prelu)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/prelu.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    inputs.emplace_back(test::NDArray<float, 3>(
                            {{{1, 0, 1, 0, 1}, {0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}, {0, 1, 0, 1, 0}},
                             {{0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}, {0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}},
                             {{1, 0, 1, 0, 1}, {0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}, {0, 1, 0, 1, 0}}})
                            .get_vector());

    Outputs expected_output{
        test::NDArray<float, 3>(
            {{{-9, 0, -7, 0, -5}, {0, -3, 0, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{0, -3, 0, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {0, -1, 0, -1, 0}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_selu)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/selu.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    Outputs expected_output{
        test::NDArray<float, 3>(
            {{{-5.99925954117548f,
               -5.99798722423258f,
               -5.99452870820667f,
               -5.98512748694000f,
               -5.95957231800549f},
              {-5.89010616666759f, -5.70127758979282f, -5.18798830058032f, -3.79272335297135f, 0},
              {3, 6, 9, 12, 15},
              {18, 21, 24, 27, 30}},
             {{-5.89010616666759f, -5.70127758979282f, -5.18798830058032f, -3.79272335297135f, 0},
              {3, 6, 9, 12, 15},
              {18, 21, 24, 27, 30},
              {33, 36, 39, 42, 45}},
             {{3, 3, 3, 3, 3},
              {-3.79272335297135f,
               -3.79272335297135f,
               -3.79272335297135f,
               -3.79272335297135f,
               -3.79272335297135f},
              {0, 0, 0, 0, 0},
              {6, 6, 6, 6, 6}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sigmoid)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/sigmoid.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    Outputs expected_output{test::NDArray<float, 3>({{{0.00012339457598623f,
                                                       0.00033535013046648f,
                                                       0.00091105119440065f,
                                                       0.00247262315663477f,
                                                       0.00669285092428486f},
                                                      {0.01798620996209160f,
                                                       0.04742587317756680f,
                                                       0.119202922022118f,
                                                       0.268941421369995f,
                                                       0.5f},
                                                      {0.731058578630005f,
                                                       0.880797077977882f,
                                                       0.952574126822433f,
                                                       0.982013790037908f,
                                                       0.993307149075715f},
                                                      {0.997527376843365f,
                                                       0.999088948805599f,
                                                       0.999664649869534f,
                                                       0.999876605424014f,
                                                       0.999954602131298f}},
                                                     {{0.01798620996209160f,
                                                       0.04742587317756680f,
                                                       0.119202922022118f,
                                                       0.268941421369995f,
                                                       0.5f},
                                                      {0.731058578630005f,
                                                       0.880797077977882f,
                                                       0.952574126822433f,
                                                       0.982013790037908f,
                                                       0.993307149075715f},
                                                      {0.997527376843365f,
                                                       0.999088948805599f,
                                                       0.999664649869534f,
                                                       0.999876605424014f,
                                                       0.999954602131298f},
                                                      {0.999983298578152f,
                                                       0.999993855825398f,
                                                       0.999997739675702f,
                                                       0.999999168471972f,
                                                       0.999999694097773f}},
                                                     {{0.731058578630005f,
                                                       0.731058578630005f,
                                                       0.731058578630005f,
                                                       0.731058578630005f,
                                                       0.731058578630005f},
                                                      {0.268941421369995f,
                                                       0.268941421369995f,
                                                       0.268941421369995f,
                                                       0.268941421369995f,
                                                       0.268941421369995f},
                                                      {0.5f, 0.5f, 0.5f, 0.5f, 0.5f},
                                                      {0.880797077977882f,
                                                       0.880797077977882f,
                                                       0.880797077977882f,
                                                       0.880797077977882f,
                                                       0.880797077977882f}}})
                                .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_tanh)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/tanh.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    Outputs expected_output{test::NDArray<float, 3>({{{-0.999999969540041f,
                                                       -0.999999774929676f,
                                                       -0.999998336943945f,
                                                       -0.999987711650796f,
                                                       -0.999909204262595f},
                                                      {-0.999329299739067f,
                                                       -0.995054753686731f,
                                                       -0.964027580075817f,
                                                       -0.761594155955765f,
                                                       0},
                                                      {0.761594155955765f,
                                                       0.964027580075817f,
                                                       0.995054753686731f,
                                                       0.999329299739067f,
                                                       0.999909204262595f},
                                                      {0.999987711650796f,
                                                       0.999998336943945f,
                                                       0.999999774929676f,
                                                       0.999999969540041f,
                                                       0.999999995877693f}},
                                                     {{-0.999329299739067f,
                                                       -0.995054753686731f,
                                                       -0.964027580075817f,
                                                       -0.761594155955765f,
                                                       0},
                                                      {0.761594155955765f,
                                                       0.964027580075817f,
                                                       0.995054753686731f,
                                                       0.999329299739067f,
                                                       0.999909204262595f},
                                                      {0.999987711650796f,
                                                       0.999998336943945f,
                                                       0.999999774929676f,
                                                       0.999999969540041f,
                                                       0.999999995877693f},
                                                      {0.999999999442106f,
                                                       0.999999999924497f,
                                                       0.999999999989782f,
                                                       0.999999999998617f,
                                                       0.999999999999813f}},
                                                     {{0.761594155955765f,
                                                       0.761594155955765f,
                                                       0.761594155955765f,
                                                       0.761594155955765f,
                                                       0.761594155955765f},
                                                      {-0.761594155955765f,
                                                       -0.761594155955765f,
                                                       -0.761594155955765f,
                                                       -0.761594155955765f,
                                                       -0.761594155955765f},
                                                      {0, 0, 0, 0, 0},
                                                      {0.964027580075817f,
                                                       0.964027580075817f,
                                                       0.964027580075817f,
                                                       0.964027580075817f,
                                                       0.964027580075817f}}})
                                .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_thresholded_relu)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/thresholded_relu.prototxt"));

    Inputs inputs;
    inputs.emplace_back(
        test::NDArray<float, 3>(
            {{{-9, -8, -7, -6, -5}, {-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{-4, -3, -2, -1, 0}, {1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{1, 1, 1, 1, 1}, {-1, -1, -1, -1, -1}, {0, 0, 0, 0, 0}, {2, 2, 2, 2, 2}}})
            .get_vector());

    Outputs expected_output{
        test::NDArray<float, 3>(
            {{{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 3, 4, 5}, {6, 7, 8, 9, 10}},
             {{0, 0, 0, 0, 0}, {0, 0, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}},
             {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_unsupported_op)
{
    try
    {
        onnx_import::import_onnx_model(
            file_util::path_join(SERIALIZED_ZOO, "onnx/unsupported_op.prototxt"));
        FAIL() << "Expected ngraph::ngraph_error";
    }
    catch (ngraph::ngraph_error const& err)
    {
        std::string what{err.what()};
        EXPECT_NE(what.find("nGraph does not support"), std::string::npos);
        EXPECT_NE(what.find("FakeOpName"), std::string::npos);
        EXPECT_NE(what.find("AnotherFakeOpName"), std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Expected ngraph::ngraph_error";
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_custom_op)
{
    onnx_import::register_operator(
        "AddQ", 1, "com.intel.ai", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            return {std::make_shared<ngraph::op::Add>(ng_inputs.at(0), ng_inputs.at(1))};
        });

    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/custom_operator.prototxt"));

    Inputs inputs{{1, 2, 3, 4}};
    Outputs expected_outputs{{3, 6, 9, 12}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_custom_op_default_domain)
{
    onnx_import::register_operator(
        "AddQ", 1, "com.intel.ai", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            return {std::make_shared<ngraph::op::Add>(ng_inputs.at(0), ng_inputs.at(1))};
        });

    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/custom_operator_default_domain.prototxt"));

    Inputs inputs{{1, 2, 3, 4}};
    Outputs expected_outputs{{3, 6, 9, 12}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_conv2d_dilation_assymetric_pads_strides)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/conv2d_dilation_assym_pads_strides.prototxt"));

    //   "",                           // auto_pad
    //   vector<int64_t>{1, 1},        // dilations
    //   1,                            // group
    //   vector<int64_t>{3, 3},        // kernel_shape
    //   vector<int64_t>{1, 1, 1, 2},  // pads
    //   vector<int64_t>{3, 1}         // strides

    Inputs inputs;
    // {2, 1, 1, 1}
    inputs.emplace_back(
        test::NDArray<float, 4>({{{{-0.09103918075561523f}}}, {{{-0.32513630390167236f}}}})
            .get_vector());
    // {2, 1, 3, 3}
    inputs.emplace_back(
        test::NDArray<float, 4>(
            {{{{0.4312484860420227f, -0.12559029459953308f, 0.44889551401138306f},
               {-0.3100617825984955f, 0.13522827625274658f, -0.06791308522224426f},
               {0.22671669721603394f, -0.17391827702522278f, -0.31299442052841187f}}},
             {{{-0.31545522809028625f, 0.06560015678405762f, 0.2656586766242981f},
               {0.41363757848739624f, 0.31231558322906494f, -0.376018226146698f},
               {-0.005708813667297363f, 0.34922850131988525f, 0.45095211267471313f}}}})
            .get_vector());

    // {2, 2, 1, 2}
    Outputs expected_output{
        test::NDArray<float, 4>({{{{-0.012311071157455444f, 0.02822777070105076f}},
                                  {{-0.028432954102754593f, -0.037657227367162704f}}},
                                 {{{-0.04396762326359749f, 0.10081233829259872f}},
                                  {{-0.10154513269662857f, -0.13448859751224518f}}}})
            .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_conv3d_bias)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/conv3d_bias.prototxt"));

    // "",                                 // auto_pad
    // vector<int64_t>{2, 2, 2},           // dilations
    // 1,                                  // group
    // vector<int64_t>{2, 2, 2},           // kernel_shape
    // vector<int64_t>{2, 2, 2, 2, 2, 2},  // pads
    // vector<int64_t>{2, 2, 2}            // strides

    Inputs inputs;
    // X: {2, 1, 4, 4, 4}
    inputs.emplace_back(
        std::vector<float>{0.46796226501464844f,   -0.4613912105560303f,  0.33512794971466064f,
                           -0.4010460674762726f,   0.41722816228866577f,  -0.048133403062820435f,
                           0.20415884256362915f,   0.03189706802368164f,  -0.04779183864593506f,
                           -0.0795503556728363f,   0.4987630844116211f,   0.3506373167037964f,
                           0.48065757751464844f,   0.269855260848999f,    -0.2463444471359253f,
                           0.19044137001037598f,   -0.11830493807792664f, -0.2576887905597687f,
                           -0.33940935134887695f,  -0.257951021194458f,   -0.08279827237129211f,
                           0.3513314127922058f,    -0.29122066497802734f, -0.43358397483825684f,
                           -0.13429927825927734f,  0.44032156467437744f,  0.05308258533477783f,
                           -0.3499870300292969f,   -0.28474611043930054f, -0.44209951162338257f,
                           -0.07418054342269897f,  -0.10919415950775146f, 0.2845439314842224f,
                           0.3498746156692505f,    -0.19313520193099976f, 0.32609254121780396f,
                           0.4880145788192749f,    0.05574071407318115f,  -0.46457427740097046f,
                           -0.02524462342262268f,  -0.18780940771102905f, -0.14720159769058228f,
                           0.207585871219635f,     0.47157740592956543f,  -0.05567386746406555f,
                           -0.49871665239334106f,  0.2274145483970642f,   0.4589425325393677f,
                           -0.4725189805030823f,   -0.4358765780925751f,  0.2841453552246094f,
                           -0.27037882804870605f,  0.34227508306503296f,  0.33575427532196045f,
                           -0.19485199451446533f,  -0.27679920196533203f, -0.4238079786300659f,
                           -0.4385119676589966f,   0.43724071979522705f,  0.3065117597579956f,
                           0.45696544647216797f,   0.05291992425918579f,  -0.023618370294570923f,
                           -0.1860884726047516f,   0.08669537305831909f,  0.32541000843048096f,
                           0.1846179962158203f,    -0.1984834372997284f,  -0.2754465937614441f,
                           0.32004624605178833f,   -0.34846532344818115f, 0.0999596118927002f,
                           -0.11374691128730774f,  0.21225297451019287f,  -0.02315312623977661f,
                           0.1671370267868042f,    0.22319108247756958f,  0.03609824180603027f,
                           -0.1587022840976715f,   0.059984564781188965f, -0.03951650857925415f,
                           -0.4841443598270416f,   0.32919085025787354f,  -0.23115816712379456f,
                           0.39441078901290894f,   -0.3554944396018982f,  -0.17022761702537537f,
                           -0.055081307888031006f, 0.15856128931045532f,  -0.4183449149131775f,
                           -0.2474445104598999f,   0.03603637218475342f,  -0.2836887538433075f,
                           0.4602506160736084f,    0.29092925786972046f,  -0.199321448802948f,
                           0.380856454372406f,     -0.13847029209136963f, -0.238397479057312f,
                           -0.1907123327255249f,   -0.11061936616897583f, -0.08717870712280273f,
                           0.24449139833450317f,   -0.14727482199668884f, 0.1437196135520935f,
                           0.3955056071281433f,    -0.12538021802902222f, 0.11590522527694702f,
                           0.4598066806793213f,    -0.30005723237991333f, -0.46578651666641235f,
                           -0.33955082297325134f,  -0.2671887278556824f,  0.3611910939216614f,
                           -0.11423084139823914f,  -0.08382436633110046f, -0.31819307804107666f,
                           0.14515334367752075f,   0.3157258629798889f,   0.33179205656051636f,
                           -0.2558857202529907f,   0.11888682842254639f,  0.12824326753616333f,
                           -0.33106181025505066f,  0.2549159526824951f,   -0.46760573983192444f,
                           -0.11983257532119751f,  0.1834418773651123f});

    // W: {2, 1, 2, 2, 2}
    inputs.emplace_back(std::vector<float>{0.388077974319458f,
                                           -0.16366064548492432f,
                                           -0.42871910333633423f,
                                           0.4276432394981384f,
                                           0.21517693996429443f,
                                           0.007908165454864502f,
                                           0.33897721767425537f,
                                           0.21843165159225464f,
                                           0.34095364809036255f,
                                           -0.17043980956077576f,
                                           -0.013571739196777344f,
                                           -0.26793742179870605f,
                                           -0.34863436222076416f,
                                           -0.2672275900840759f,
                                           -0.36691007018089294f,
                                           0.37296557426452637f});

    // B: {2}
    inputs.emplace_back(std::vector<float>{0.4310183525085449f, -0.4564093053340912f});

    // {2, 2, 3, 3, 3}
    Outputs expected_output{std::vector<float>{
        0.5332361459732056f,   0.6628494262695312f,   0.544619083404541f,    0.4242798388004303f,
        0.6271085739135742f,   0.6721994876861572f,   0.43064039945602417f,  0.4246789515018463f,
        0.53834068775177f,     0.6932926177978516f,   0.42797625064849854f,  0.2218741625547409f,
        0.29522019624710083f,  0.8329390287399292f,   0.37605351209640503f,  0.43735477328300476f,
        0.2920728623867035f,   0.6692450046539307f,   0.5527016520500183f,   0.22643595933914185f,
        0.5138190984725952f,   0.3041342794895172f,   0.7423423528671265f,   0.26707080006599426f,
        0.4617553651332855f,   0.32416003942489624f,  0.511577844619751f,    -0.28187549114227295f,
        -0.5031181573867798f,  -0.5793710947036743f,  -0.5992864370346069f,  -0.5055556893348694f,
        -0.7562476396560669f,  -0.44363799691200256f, -0.5730307102203369f,  -0.6302952766418457f,
        -0.4756688177585602f,  -0.728988528251648f,   -0.3900943398475647f,  -0.6694478988647461f,
        -0.38822290301322937f, -0.35774707794189453f, -0.39807581901550293f, -0.547709047794342f,
        -0.35872578620910645f, -0.5326492786407471f,  -0.40852290391921997f, -0.4537881314754486f,
        -0.4545857608318329f,  -0.379546195268631f,   -0.5250767469406128f,  -0.42439910769462585f,
        -0.5558245182037354f,  -0.38563215732574463f, 0.44995537400245667f,  0.5007325410842896f,
        0.49359965324401855f,  0.40685802698135376f,  0.407518208026886f,    0.4628955125808716f,
        0.4301188290119171f,   0.40635955333709717f,  0.4260363280773163f,   0.55128413438797f,
        0.5498291254043579f,   0.27105778455734253f,  0.40259143710136414f,  0.5747092962265015f,
        0.4187920391559601f,   0.4507707953453064f,   0.420598566532135f,    0.3950541913509369f,
        0.593889057636261f,    0.16578882932662964f,  0.5332239270210266f,   0.43014785647392273f,
        0.50260329246521f,     0.39225444197654724f,  0.4074971079826355f,   0.5073125958442688f,
        0.3823610544204712f,   -0.4240749180316925f,  -0.41936254501342773f, -0.5241475105285645f,
        -0.5220003724098206f,  -0.502869725227356f,   -0.5122783780097961f,  -0.4260129928588867f,
        -0.4105660617351532f,  -0.4483373165130615f,  -0.33759188652038574f, -0.735706090927124f,
        -0.3714444637298584f,  -0.4888814687728882f,  -0.6191370487213135f,  -0.2640320658683777f,
        -0.47542816400527954f, -0.5078460574150085f,  -0.4205915927886963f,  -0.5584549903869629f,
        -0.39770257472991943f, -0.45317384600639343f, -0.5598302483558655f,  -0.2542789578437805f,
        -0.5359901785850525f,  -0.48090484738349915f, -0.38603779673576355f, -0.4991581439971924f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_matmul_vec_ten3d)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/matmul_vec_ten3d.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0.f, 1.f});
    inputs.emplace_back(
        test::NDArray<float, 3>{{{0.f}, {1.f}}, {{2.f}, {3.f}}, {{4.f}, {5.f}}}.get_vector());

    Outputs expected_output{test::NDArray<float, 2>{{1.f}, {3.f}, {5.f}}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_softplus)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/softplus.prototxt"));

    // -1.0f, 0, 1.0f, 10.f,                    normal input values for activation
    // 100.0f, -100.0f, 1000.0f, -1000.0f,      input values that leads to exp() overflow
    // FLT_MIN, FLT_MIN / 16, -FLT_MIN / 16,    min, denorm, -denorm
    // FLT_MAX, -FLT_MAX,                       max, -max;
    Inputs inputs{std::vector<float>{-1.0f,
                                     0,
                                     1.0f,
                                     10.f,
                                     100.0f,
                                     -100.0f,
                                     1000.0f,
                                     -1000.0f,
                                     FLT_MIN,
                                     FLT_MIN / 16,
                                     -FLT_MIN / 16,
                                     FLT_MAX,
                                     -FLT_MAX}};

    std::vector<float>& input = inputs.back();
    std::vector<float> output;
    auto softplus_impl = [](float x) -> float {
        if (x > 0)
        {
            return x + std::log(std::exp(-x) + 1);
        }
        else
        {
            return std::log(std::exp(x) + 1);
        }
    };

    std::transform(std::begin(input), std::end(input), std::back_inserter(output), softplus_impl);

    Outputs expected_output{output};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_softplus_infinity)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/softplus.prototxt"));

    Inputs inputs{std::vector<float>{std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity()}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    for (float v : outputs.front())
    {
        EXPECT_TRUE(std::isinf(v));
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sum_opset8)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/sum_opset8.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{1.0f, 2.0f, 3.0f});
    inputs.emplace_back(test::NDArray<float, 2>{{10.0f}, {20.0f}, {30.0f}}.get_vector());
    inputs.emplace_back(test::NDArray<float, 3>{{{100.0f}}, {{200.0f}}, {{300.0f}}}.get_vector());

    Outputs expected_output{test::NDArray<float, 3>{
        {{111.0f, 112.0f, 113.0f}, {121.0f, 122.0f, 123.0f}, {131.0f, 132.0f, 133.0f}},

        {{211.0f, 212.0f, 213.0f}, {221.0f, 222.0f, 223.0f}, {231.0f, 232.0f, 233.0f}},

        {{311.0f, 312.0f, 313.0f}, {321.0f, 322.0f, 323.0f}, {331.0f, 332.0f, 333.0f}}}
                                .get_vector()};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_conv_transpose_w_groups)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/conv_transpose_w_groups.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{
        0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f});
    inputs.emplace_back(std::vector<float>{0.f,  1.f,  2.f,  3.f,  4.f,  5.f,  6.f,  7.f,
                                           8.f,  9.f,  10.f, 11.f, 12.f, 13.f, 14.f, 15.f,
                                           16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
                                           24.f, 25.f, 26.f, 27.f, 28.f, 29.f, 30.f, 31.0f});

    Outputs expected_output{
        std::vector<float>{28.f, 34.f, 252.f, 274.f, 732.f, 770.f, 1468.f, 1522.f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_argmax_int32)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/argmax_int32.prototxt"));

    std::vector<std::vector<std::int32_t>> inputs{
        std::vector<std::int32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};

    std::vector<std::vector<std::int64_t>> expected_output{
        std::vector<std::int64_t>{1, 1, 1, 1, 1, 1}};

    std::vector<std::vector<std::int64_t>> outputs{
        execute<std::int32_t, std::int64_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_argmin_int32)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/argmin_int32.prototxt"));

    std::vector<std::vector<std::int32_t>> inputs{
        std::vector<std::int32_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};

    std::vector<std::vector<std::int64_t>> expected_output{std::vector<std::int64_t>{0, 0, 0, 0}};

    std::vector<std::vector<std::int64_t>> outputs{
        execute<std::int32_t, std::int64_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, is_op_supported)
{
    // Simple case
    EXPECT_TRUE(onnx_import::is_operator_supported("Sum", 1, "ai.onnx"));
    // With fallback
    EXPECT_TRUE(onnx_import::is_operator_supported("Sum", 100, "ai.onnx"));

    // Different opset versions
    EXPECT_TRUE(onnx_import::is_operator_supported("Add", 1, "ai.onnx"));
    EXPECT_TRUE(onnx_import::is_operator_supported("Add", 7, "ai.onnx"));

    // Default domain name
    EXPECT_TRUE(onnx_import::is_operator_supported("Sum", 1));

    // Unregistered operator
    EXPECT_FALSE(onnx_import::is_operator_supported("DummyOp", 1));
    EXPECT_FALSE(onnx_import::is_operator_supported("DummyOp", 1, "ai.onnx"));
    EXPECT_FALSE(onnx_import::is_operator_supported("DummyOp", 10, "ai.onnx"));

    // Operator with bad domain name
    EXPECT_FALSE(onnx_import::is_operator_supported("Sum", 1, "bad.domain"));

    // Registered custom operator
    onnx_import::register_operator(
        "AddQ", 1, "com.intel.ai", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            return {std::make_shared<ngraph::op::Add>(ng_inputs.at(0), ng_inputs.at(1))};
        });
    EXPECT_TRUE(onnx_import::is_operator_supported("AddQ", 1, "com.intel.ai"));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_depth_to_space)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/depth_to_space.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{
        0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f});

    Outputs expected_output{std::vector<float>{
        0.f, 4.f, 1.f, 5.f, 8.f, 12.f, 9.f, 13.f, 2.f, 6.f, 3.f, 7.f, 10.f, 14.f, 11.f, 15.f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_depth_to_space_chw)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/depth_to_space_chw.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{
        0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f});

    Outputs expected_output{std::vector<float>{
        0.f, 4.f, 1.f, 5.f, 8.f, 12.f, 9.f, 13.f, 2.f, 6.f, 3.f, 7.f, 10.f, 14.f, 11.f, 15.f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_depth_to_space_bad_blocksize)
{
    // This model fails to import since the depth channel length must be a multiple of the
    // `blocksize` attribute value.
    EXPECT_THROW(onnx_import::import_onnx_model(file_util::path_join(
                     SERIALIZED_ZOO, "onnx/depth_to_space_bad_blocksize.prototxt")),
                 std::runtime_error);
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_depth_to_space_no_blocksize)
{
    // This model fails to import since it lacks of required parameter `blocksize`.
    EXPECT_THROW(onnx_import::import_onnx_model(file_util::path_join(
                     SERIALIZED_ZOO, "onnx/depth_to_space_no_blocksize.prototxt")),
                 std::runtime_error);
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_space_to_depth)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/space_to_depth.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0.f,  1.f,  2.f,  3.f,  4.f,  5.f,  6.f,  7.f,
                                           8.f,  9.f,  10.f, 11.f, 12.f, 13.f, 14.f, 15.f,
                                           16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
                                           24.f, 25.f, 26.f, 27.f, 28.f, 29.f, 30.f, 31.f});

    Outputs expected_output{std::vector<float>{
        0.f, 2.f, 8.f,  10.f, 16.f, 18.f, 24.f, 26.f, 1.f, 3.f, 9.f,  11.f, 17.f, 19.f, 25.f, 27.f,
        4.f, 6.f, 12.f, 14.f, 20.f, 22.f, 28.f, 30.f, 5.f, 7.f, 13.f, 15.f, 21.f, 23.f, 29.f, 31.f,
    }};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_space_to_depth_chw)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/space_to_depth_chw.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0.f,  1.f,  2.f,  3.f,  4.f,  5.f,  6.f,  7.f,
                                           8.f,  9.f,  10.f, 11.f, 12.f, 13.f, 14.f, 15.f,
                                           16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
                                           24.f, 25.f, 26.f, 27.f, 28.f, 29.f, 30.f, 31.f});

    Outputs expected_output{std::vector<float>{
        0.f, 2.f, 8.f,  10.f, 16.f, 18.f, 24.f, 26.f, 1.f, 3.f, 9.f,  11.f, 17.f, 19.f, 25.f, 27.f,
        4.f, 6.f, 12.f, 14.f, 20.f, 22.f, 28.f, 30.f, 5.f, 7.f, 13.f, 15.f, 21.f, 23.f, 29.f, 31.f,
    }};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_space_to_depth_bad_blocksize)
{
    // This model fails to import since the depth channel length must be a multiple of the
    // `blocksize` attribute value.
    EXPECT_THROW(onnx_import::import_onnx_model(file_util::path_join(
                     SERIALIZED_ZOO, "onnx/space_to_depth_bad_blocksize.prototxt")),
                 std::runtime_error);
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_space_to_depth_no_blocksize)
{
    // This model fails to import since it lacks of required `blocksize` attribute.
    EXPECT_THROW(onnx_import::import_onnx_model(file_util::path_join(
                     SERIALIZED_ZOO, "onnx/space_to_depth_no_blocksize.prototxt")),
                 std::runtime_error);
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_missing_op_domain)
{
    onnx_import::register_operator(
        "CustomAdd", 1, "custom.op", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            return {std::make_shared<ngraph::op::Add>(ng_inputs.at(0), ng_inputs.at(1))};
        });

    EXPECT_TRUE(onnx_import::is_operator_supported("CustomAdd", 1, "custom.op"));

    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/missing_op_domain.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0.f, 1.f, 2.f, 3.f});
    inputs.emplace_back(std::vector<float>{0.f, 1.f, 2.f, 3.f});

    Outputs expected_output{std::vector<float>{0.f, 2.f, 4.f, 6.f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_top_k)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/top_k.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

    std::vector<float> expected_values_output{3, 2, 1, 7, 6, 5, 11, 10, 9};
    std::vector<std::int64_t> expected_indices_output{3, 2, 1, 3, 2, 1, 3, 2, 1};

    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> result_tensors =
        prepare_and_run(function, inputs, "${BACKEND_NAME}");

    std::vector<float> values_output = read_vector<float>(result_tensors.at(0));
    std::vector<std::int64_t> indices_output = read_vector<std::int64_t>(result_tensors.at(1));

    EXPECT_TRUE(test::all_close_f(expected_values_output, values_output));
    EXPECT_TRUE(test::all_close(expected_indices_output, indices_output));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_lstm_fwd_with_clip)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/lstm_fwd_with_clip.prototxt"));

    Inputs inputs{};
    // X
    inputs.emplace_back(std::vector<float>{-0.455351, -0.276391, -0.185934, -0.269585});

    // W
    inputs.emplace_back(std::vector<float>{-0.494659f,
                                           0.0453352f,
                                           -0.487793f,
                                           0.417264f,
                                           -0.0175329f,
                                           0.489074f,
                                           -0.446013f,
                                           0.414029f,
                                           -0.0091708f,
                                           -0.255364f,
                                           -0.106952f,
                                           -0.266717f,
                                           -0.0888852f,
                                           -0.428709f,
                                           -0.283349f,
                                           0.208792f});

    // R
    inputs.emplace_back(std::vector<float>{0.146626f,
                                           -0.0620289f,
                                           -0.0815302f,
                                           0.100482f,
                                           -0.219535f,
                                           -0.306635f,
                                           -0.28515f,
                                           -0.314112f,
                                           -0.228172f,
                                           0.405972f,
                                           0.31576f,
                                           0.281487f,
                                           -0.394864f,
                                           0.42111f,
                                           -0.386624f,
                                           -0.390225f});

    // B
    inputs.emplace_back(std::vector<float>{0.381619f,
                                           0.0323954f,
                                           -0.14449f,
                                           0.420804f,
                                           -0.258721f,
                                           0.45056f,
                                           -0.250755f,
                                           0.0967895f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f});
    // P
    inputs.emplace_back(std::vector<float>{0.2345f, 0.5235f, 0.4378f, 0.3475f, 0.8927f, 0.3456f});

    Outputs expected_output{};
    // Y_data
    expected_output.emplace_back(
        std::vector<float>{-0.02280854f, 0.02744377f, -0.03516197f, 0.03875681f});
    // Y_h_data
    expected_output.emplace_back(std::vector<float>{-0.03516197f, 0.03875681f});
    // Y_c_data
    expected_output.emplace_back(std::vector<float>{-0.07415761f, 0.07395997f});

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(outputs.size() == expected_output.size());
    for (std::size_t i{0}; i < expected_output.size(); ++i)
    {
        // We have to enlarge tolerance bits to 3 - it's only one bit more than default value.
        // The discrepancies may occur at most on 7th decimal position.
        EXPECT_TRUE(test::all_close_f(expected_output.at(i), outputs.at(i), 3));
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_missing_input)
{
    onnx_import::register_operator(
        "TestMissingInOut", 1, "com.intel.ai", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            std::shared_ptr<ngraph::Node> A = ng_inputs.at(0);
            std::shared_ptr<ngraph::Node> B = ng_inputs.at(1);
            std::shared_ptr<ngraph::Node> C = ng_inputs.at(2);

            A = A * C;
            if (!B->is_null())
            {
                B = B / C;
            }

            C = C + C;
            return {A, B, C};
        });

    onnx_import::register_operator(
        "TestMissingIn", 1, "com.intel.ai", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            std::shared_ptr<ngraph::Node> result = std::make_shared<ngraph::op::Constant>(
                element::f32, ngraph::Shape{2, 2}, std::vector<float>{1, 1, 1, 1});

            for (const auto& ng_input : ng_inputs)
            {
                if (!ng_input->is_null())
                {
                    result = ng_input * result;
                }
            }

            return {result};
        });

    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/missing_input.prototxt"));

    Inputs inputs{{1, 2, 3, 4}, {5, 6, 7, 8}};
    Outputs expected_outputs{{50, 144, 294, 512}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sinh)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/sinh.prototxt"));

    Inputs inputs{std::vector<float>{-1.0f, 0.0f, 1.0f}};
    Outputs expected_outputs{std::vector<float>{-1.1752012f, 0.f, 1.1752012f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_cosh)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/cosh.prototxt"));

    Inputs inputs{std::vector<float>{-1.0f, 0.0f, 1.0f}};
    Outputs expected_outputs{std::vector<float>{1.54308069f, 1.f, 1.54308069f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_initializer_wo_input)
{
    // This test checks a model which has an initializer, but no input with the same name
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/initializer_wo_input.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0, 1, 2, 3, 4, 5});

    std::vector<float> expected_output{0, 2, 6, 12, 20, 30};

    Outputs output{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output, output.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_sign)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/sign.prototxt"));

    Inputs inputs{std::vector<float>{-std::numeric_limits<float>::infinity(),
                                     -3.141592f,
                                     0.0f,
                                     2.71828f,
                                     std::numeric_limits<float>::infinity()}};

    Outputs expected_outputs{std::vector<float>{-1.0f, -1.0f, 0.0f, 1.0f, 1.0f}};

    Outputs outputs{execute<float>(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_global_lp_pool_p0)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/global_lp_pool_p0.prototxt"));

    std::vector<std::vector<std::int64_t>> inputs{std::vector<std::int64_t>{
        1, 0, -4, 0, 2, 1, -6, 1, 0, 0, 0, 0, -7, 1, -1, 0, -1, 8, 0, 10, 9, 0, 0, 5}};

    std::vector<std::vector<std::int64_t>> expected_outputs{std::vector<std::int64_t>{6, 8}};

    std::vector<std::vector<std::int64_t>> outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_global_lp_pool_p1)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/global_lp_pool_p1.prototxt"));

    Inputs inputs{std::vector<float>(2 * 3 * 4)};
    std::iota(std::begin(inputs.front()), std::end(inputs.front()), 0.f);

    Outputs expected_outputs{std::vector<float>{66.f, 210.f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_global_lp_pool_p2)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/global_lp_pool_p2.prototxt"));

    Inputs inputs{std::vector<float>(2 * 3 * 4)};
    std::iota(std::begin(inputs.front()), std::end(inputs.front()), 0.f);

    Outputs expected_outputs{std::vector<float>{22.494444f, 61.789967f}};
    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_global_lp_pool_p3)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/global_lp_pool_p3.prototxt"));

    Inputs inputs{std::vector<float>(2 * 3 * 4)};
    std::iota(std::begin(inputs.front()), std::end(inputs.front()), 0.f);

    Outputs expected_outputs{std::vector<float>{16.331620904278438f, 41.56697946707537f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_one_hot_with_axis)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/one_hot_axis.prototxt"));

    Inputs inputs{{1.0, 9.0, 2.0, 4.0}, {1.0, 3.0}};
    Outputs expected_outputs{{1.0, 1.0, 3.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                              1.0, 1.0, 1.0, 1.0, 1.0, 3.0, 1.0, 1.0, 1.0, 1.0, 3.0, 1.0, 1.0, 1.0,
                              1.0, 3.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close_f(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_one_hot_without_axis)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/one_hot_no_axis.prototxt"));

    std::vector<std::vector<std::int64_t>> inputs{{0, 7, 8}, {2, 5}};
    std::vector<std::vector<std::int64_t>> expected_outputs{{5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                             2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2,
                                                             2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2}};

    std::vector<std::vector<std::int64_t>> outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_TRUE(test::all_close(expected_outputs.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_where)
{
    auto function =
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/where.prototxt"));

    // conditions tensor - 3x3x3
    auto condition = std::vector<int>{
        {0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0}};

    // 1x3 tensor of "1"
    auto x1 = std::vector<int>{1, 1, 1};
    // 3x1 tensor of "2"
    auto x2 = std::vector<int>{2, 2, 2};

    std::vector<std::vector<int>> inputs;
    inputs.push_back(std::move(condition));
    inputs.push_back(std::move(x1));
    inputs.push_back(std::move(x2));

    // y = 3x3x3
    std::vector<std::vector<int>> expected_outputs{
        {2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2, 1, 2, 1, 2}};

    std::vector<std::vector<int>> outputs{execute(function, inputs, "${BACKEND_NAME}")};

    EXPECT_EQ(expected_outputs.front(), outputs.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_override_op)
{
    onnx_import::register_operator(
        "FalseAdd", 1, "", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            return {std::make_shared<ngraph::op::Add>(ng_inputs.at(0), ng_inputs.at(1))};
        });

    onnx_import::register_operator(
        "FalseAdd", 1, "", [](const onnx_import::Node& node) -> NodeVector {
            NodeVector ng_inputs{node.get_ng_inputs()};
            return {std::make_shared<ngraph::op::Subtract>(ng_inputs.at(0), ng_inputs.at(1))};
        });

    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/override_op.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{0.f, 1.f, 2.f, 3.f});
    inputs.emplace_back(std::vector<float>{3.f, 2.f, 1.f, 0.f});

    Outputs expected_output{std::vector<float>{-3.f, -1.f, 1.f, 3.f}};

    Outputs outputs{execute(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, import_non_existing_file)
{
    try
    {
        onnx_import::import_onnx_model(file_util::path_join(SERIALIZED_ZOO, "onnx/i.dont.exist"));
    }
    catch (const std::runtime_error& exc)
    {
        // asserts that an exception was thrown and that the error message contains the file name
        std::string msg{exc.what()};
        EXPECT_TRUE(msg.find("i.dont.exist") != std::string::npos);
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_lstm_fwd_mixed_seq)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/lstm_fwd_mixed_seq.prototxt"));

    int hidden_size{3};
    int parameters_cout{5};

    // X
    std::vector<float> in_x{1.f, 2.f, 10.f, 11.f};
    // W
    std::vector<float> in_w{0.1f, 0.2f, 0.3f, 0.4f, 1.f, 2.f, 3.f, 4.f, 10.f, 11.f, 12.f, 13.f};
    // R
    std::vector<float> in_r(4 * hidden_size * hidden_size, 0.1f);
    // B
    std::vector<float> in_b(8 * hidden_size, 0.0f);

    std::vector<int> in_seq_lengths{1, 2};

    std::vector<float> out_y_data{0.28828835f,
                                  0.36581863f,
                                  0.45679406f,
                                  0.34526032f,
                                  0.47220859f,
                                  0.55850911f,
                                  0.f,
                                  0.f,
                                  0.f,
                                  0.85882828f,
                                  0.90703777f,
                                  0.92382453f};

    std::vector<float> out_y_h_data{
        0.28828835f, 0.36581863f, 0.45679406f, 0.85882828f, 0.90703777f, 0.92382453f};

    std::vector<float> out_y_c_data{
        0.52497941f, 0.54983425f, 0.5744428f, 1.3249796f, 1.51063104f, 1.61451544f};

    Outputs expected_output;

    expected_output.emplace_back(out_y_data);
    expected_output.emplace_back(out_y_h_data);
    expected_output.emplace_back(out_y_c_data);

    auto backend = ngraph::runtime::Backend::create("${BACKEND_NAME}");
    auto parameters = function->get_parameters();

    EXPECT_TRUE(parameters.size() == parameters_cout);

    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> arg_tensors;

    auto add_tensor = [&arg_tensors, &backend](const std::vector<float>& v,
                                               const std::shared_ptr<ngraph::op::Parameter>& p) {
        auto t = backend->create_tensor(p->get_element_type(), p->get_shape());
        copy_data(t, v);
        arg_tensors.push_back(t);
    };

    add_tensor(in_x, parameters.at(0));
    add_tensor(in_w, parameters.at(1));
    add_tensor(in_r, parameters.at(2));
    add_tensor(in_b, parameters.at(3));

    auto t_in_seq_lengths =
        backend->create_tensor(parameters.at(4)->get_element_type(), parameters.at(4)->get_shape());
    copy_data(t_in_seq_lengths, in_seq_lengths);
    arg_tensors.push_back(t_in_seq_lengths);

    auto results = function->get_results();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> result_tensors(results.size());

    for (std::size_t i{0}; i < results.size(); ++i)
    {
        result_tensors.at(i) =
            backend->create_tensor(results.at(i)->get_element_type(), results.at(i)->get_shape());
    }

    auto handle = backend->compile(function);
    handle->call_with_validate(result_tensors, arg_tensors);

    Outputs outputs;
    for (auto rt : result_tensors)
    {
        outputs.push_back(read_vector<float>(rt));
    }

    EXPECT_TRUE(outputs.size() == expected_output.size());
    for (std::size_t i{0}; i < expected_output.size(); ++i)
    {
        // We have to enlarge tolerance bits to 3 - it's only one bit more than default value.
        // The discrepancies may occur at most on 7th decimal position.
        EXPECT_TRUE(test::all_close_f(expected_output.at(i), outputs.at(i), 3));
    }
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quantize_linear)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quant_lin.prototxt"));

    Inputs inputs;
    inputs.emplace_back(std::vector<float>{32.25f, 48.34f, 50.f, 83.f});
    inputs.emplace_back(std::vector<float>{0.5f});

    std::vector<std::vector<std::uint8_t>> expected_output{
        std::vector<std::uint8_t>{64, 97, 100, 166}};

    std::vector<std::vector<std::uint8_t>> outputs{
        execute<float, std::uint8_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_dequantize_linear)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/dequant_lin.prototxt"));

    std::vector<std::vector<std::uint8_t>> inputs;
    inputs.emplace_back(std::vector<std::uint8_t>{19, 210, 21, 10});

    Outputs expected_output{std::vector<float>{76.f, 840.f, 84.f, 40.f}};

    Outputs outputs{execute<std::uint8_t, float>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close_f(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quant_conv_linear)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/quant_conv_lin.prototxt"));

    std::vector<std::vector<std::uint8_t>> inputs;
    inputs.emplace_back(std::vector<std::uint8_t>{
        1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
        22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
        43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81});

    std::vector<std::vector<std::int8_t>> expected_output{std::vector<std::int8_t>{
        2,  3,  3,  3,  4,  4,  4,  5,  2,  4,  6,  7,  8,  8,  9,  9,  10, 3,  8,  11, 12,
        13, 13, 14, 14, 15, 5,  11, 16, 17, 18, 18, 19, 19, 20, 7,  14, 22, 22, 23, 23, 24,
        24, 25, 8,  18, 27, 27, 28, 28, 29, 29, 30, 10, 21, 32, 32, 33, 33, 34, 34, 35, 12,
        24, 37, 37, 38, 38, 39, 40, 40, 13, 17, 26, 27, 27, 27, 28, 28, 28, 9}};

    std::vector<std::vector<std::int8_t>> outputs{
        execute<std::uint8_t, std::int8_t>(function, inputs, "${BACKEND_NAME}")};
    EXPECT_TRUE(test::all_close(expected_output.front(), outputs.front()));
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quant_conv_linear_2d)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/qlinear_conv_2d.prototxt"));

    auto x = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/x.bin"));
    auto x_scale = read_binary_file<float>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/x_scale.bin"));
    auto x_zero_point = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/x_zero_point.bin"));

    auto w = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/w.bin"));
    auto w_scale = read_binary_file<float>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/w_scale.bin"));
    auto w_zero_point = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/w_zero_point.bin"));

    auto y_scale = read_binary_file<float>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/y_scale.bin"));
    auto y_zero_point = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/y_zero_point.bin"));

    auto backend = ngraph::runtime::Backend::create("${BACKEND_NAME}");

    auto params = function->get_parameters();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> input_tensors;
    input_tensors.push_back(backend->create_tensor(params.at(0)->get_element_type(), params.at(0)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(1)->get_element_type(), params.at(1)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(2)->get_element_type(), params.at(2)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(3)->get_element_type(), params.at(3)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(4)->get_element_type(), params.at(4)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(5)->get_element_type(), params.at(5)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(6)->get_element_type(), params.at(6)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(7)->get_element_type(), params.at(7)->get_shape()));

    copy_data(input_tensors[0], x);
    copy_data(input_tensors[1], x_scale);
    copy_data(input_tensors[2], x_zero_point);
    copy_data(input_tensors[3], w);
    copy_data(input_tensors[4], w_scale);
    copy_data(input_tensors[5], w_zero_point);
    copy_data(input_tensors[6], y_scale);
    copy_data(input_tensors[7], y_zero_point);

    auto results = function->get_results();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> result_tensors;
    result_tensors.push_back(
        backend->create_tensor(results.at(0)->get_element_type(), results.at(0)->get_shape()));

    auto handle = backend->compile(function);
    handle->call_with_validate(result_tensors, input_tensors);

    std::vector<std::vector<uint8_t>> outputs;
    outputs.push_back(read_vector<uint8_t>(result_tensors[0]));

    std::vector<std::vector<uint8_t>> expected_output;
    expected_output.push_back(read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv2d/y.bin")));

    EXPECT_EQ(expected_output.front(), outputs.front());
}

NGRAPH_TEST(onnx_${BACKEND_NAME}, model_quant_conv_linear_3d)
{
    auto function = onnx_import::import_onnx_model(
        file_util::path_join(SERIALIZED_ZOO, "onnx/qlinear_conv_3d.prototxt"));

    auto x = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/x.bin"));
    auto x_scale = read_binary_file<float>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/x_scale.bin"));
    auto x_zero_point = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/x_zero_point.bin"));

    auto w = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/w.bin"));
    auto w_scale = read_binary_file<float>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/w_scale.bin"));
    auto w_zero_point = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/w_zero_point.bin"));

    auto y_scale = read_binary_file<float>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/y_scale.bin"));
    auto y_zero_point = read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/y_zero_point.bin"));

    auto backend = ngraph::runtime::Backend::create("${BACKEND_NAME}");

    auto params = function->get_parameters();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> input_tensors;
    input_tensors.push_back(backend->create_tensor(params.at(0)->get_element_type(), params.at(0)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(1)->get_element_type(), params.at(1)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(2)->get_element_type(), params.at(2)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(3)->get_element_type(), params.at(3)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(4)->get_element_type(), params.at(4)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(5)->get_element_type(), params.at(5)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(6)->get_element_type(), params.at(6)->get_shape()));
    input_tensors.push_back(backend->create_tensor(params.at(7)->get_element_type(), params.at(7)->get_shape()));

    copy_data(input_tensors[0], x);
    copy_data(input_tensors[1], x_scale);
    copy_data(input_tensors[2], x_zero_point);
    copy_data(input_tensors[3], w);
    copy_data(input_tensors[4], w_scale);
    copy_data(input_tensors[5], w_zero_point);
    copy_data(input_tensors[6], y_scale);
    copy_data(input_tensors[7], y_zero_point);

    auto results = function->get_results();
    std::vector<std::shared_ptr<ngraph::runtime::Tensor>> result_tensors;
    result_tensors.push_back(
        backend->create_tensor(results.at(0)->get_element_type(), results.at(0)->get_shape()));

    auto handle = backend->compile(function);
    handle->call_with_validate(result_tensors, input_tensors);

    std::vector<std::vector<uint8_t>> outputs;
    outputs.push_back(read_vector<uint8_t>(result_tensors[0]));

    std::vector<std::vector<uint8_t>> expected_output;
    expected_output.push_back(read_binary_file<uint8_t>(file_util::path_join(SERIALIZED_ZOO, "onnx/qlinearconv3d/y.bin")));

    EXPECT_EQ(expected_output.front(), outputs.front());
}