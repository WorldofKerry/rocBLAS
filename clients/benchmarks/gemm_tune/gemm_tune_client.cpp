/* ************************************************************************
 * Copyright (C) 2016-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell cop-
 * ies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM-
 * PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNE-
 * CTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ************************************************************************ */
#include "gemm_tuners.hpp"

#include "type_dispatch.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>

static const auto DELIM = ",";

template <typename Ti, typename To = Ti, typename Tc = To, typename = void>
struct GEMMTunerDispatch
{
    int operator()(const Arguments& arg)
    {
        static std::unordered_set<std::string> displayed{};

        std::stringstream ss;
        ss << arg.a_type << arg.c_type << arg.compute_type;
        std::string key = ss.str();

        if(!displayed.count(key))
        {
            displayed.insert(key);
            rocblas_cout << "rocblas-gemm-tune WARN: unsupported datatype: " << arg.a_type << ", "
                         << arg.c_type << ", " << arg.compute_type << std::endl;
        }
        return -1;
    }
};

template <typename Ti, typename To, typename Tc>
struct GEMMTunerDispatch<Ti, To, Tc, std::enable_if_t<!std::is_same_v<Tc, void>>>
{
    int operator()(const Arguments& arg)
    {
        if(!strcmp(arg.function, "gemm") || !strcmp(arg.function, "gemm_ex"))
        {
            return get_best_solution<GEMMTunerEx>(arg);
        }
        else if(!strcmp(arg.function, "gemm_batched") || !strcmp(arg.function, "gemm_batched_ex"))
        {
            return get_best_solution<GEMMTunerBatchedEx>(arg);
        }
        else if(!strcmp(arg.function, "gemm_strided_batched")
                || !strcmp(arg.function, "gemm_strided_batched_ex"))
        {
            return get_best_solution<GEMMTunerStridedBatchedEx>(arg);
        }
        else
        {
            static std::unordered_set<std::string> displayed{};

            std::string key(arg.function);

            if(!displayed.count(key))
            {
                displayed.insert(key);
                rocblas_cout << "rocblas-gemm-tune WARN: unsupported function: " << arg.function
                             << std::endl;
            }
            return -1;
        }
    }

    template <template <typename...> class GEMMTUNER>
    int get_best_solution(const Arguments& arg)
    {
        static_assert(std::is_base_of_v<GEMMTunerBase<Tc>, GEMMTUNER<Ti, To, Tc>>,
                      "GEMMtuner must be derived from GEMMTunerBase");
        GEMMTUNER<Ti, To, Tc> gemm_tuner(arg);
        return gemm_tuner.get_best_solution();
    }
};

int main(int argc, char* argv[])
{
#if BUILD_WITH_TENSILE
    // Get arguments from file
    if(!rocblas_parse_data(argc, argv))
    {
        rocblas_cout << "Usage:"
                     << "\n"
                     << "  <path> points to file generated by profile logging."
                     << "\n\n"
                     << "  To activate profile logging use environment variable ROCBLAS_LAYER:"
                     << "\n"
                     << "    (ROCBLAS_LAYER & 4) != 0"
                     << "\n\n"
                     << "  Example of expected entry:"
                     << "\n"
                     << "  - {'rocblas_function': 'rocblas_sgemm', 'transA': 'T', 'transB': 'N', "
                        "'M': 512, 'N': 8320, 'K': 512, 'alpha': 1, 'lda': 512, 'ldb': 512, "
                        "'beta': 0, 'ldc': 512, 'device': 0, 'cold_iters': 5, 'iters': 20}"
                     << std::endl;
        return EXIT_FAILURE;
    }

    rocblas_parallel_initialize(1);
    rocblas_cout << "\n";

    // Keep separate streams for strided/non-strided since param numbers are different
    rocblas_internal_ostream gemm_ex_os;
    bool                     gemm_ex_has_entries = false;
    gemm_ex_os << "transA" << DELIM << "transB" << DELIM << "M" << DELIM << "N" << DELIM
               << "batch_count" << DELIM << "K" << DELIM << "alpha" << DELIM << "beta" << DELIM
               << "lda" << DELIM << "ldb" << DELIM << "ldc" << DELIM << "input_type" << DELIM
               << "output_type" << DELIM << "compute_type" << DELIM << "solution_index"
               << "\n";

    rocblas_internal_ostream gemm_strided_ex_os;
    bool                     gemm_strided_ex_has_entries = false;
    gemm_strided_ex_os << "transA" << DELIM << "transB" << DELIM << "M" << DELIM << "N" << DELIM
                       << "batch_count" << DELIM << "K" << DELIM << "alpha" << DELIM << "beta"
                       << DELIM << "lda" << DELIM << "ldb" << DELIM << "ldc" << DELIM << "stride_a"
                       << DELIM << "stride_b" << DELIM << "stride_c" << DELIM << "input_type"
                       << DELIM << "output_type" << DELIM << "compute_type" << DELIM
                       << "solution_index"
                       << "\n";

    rocblas_internal_ostream* current_os;
    bool*                     current_entry;

    // Track unique args to avoid duplicates
    std::unordered_set<std::string> processed{};

    // Benchmark each case
    for(const Arguments& arg : RocBLAS_TestData())
    {
        std::stringstream ss;

        // Build log entry, which doubles as set key for duplicate check
        if(!strcmp(arg.function, "gemm") || !strcmp(arg.function, "gemm_ex")
           || !strcmp(arg.function, "gemm_batched") || !strcmp(arg.function, "gemm_batched_ex"))
        {
            ss << arg.transA << DELIM << arg.transB << DELIM << arg.M << DELIM << arg.N << DELIM
               << arg.batch_count << DELIM << arg.K << DELIM << arg.alpha << DELIM << arg.beta
               << DELIM << arg.lda << DELIM << arg.ldb << DELIM << arg.ldc << DELIM
               << rocblas_datatype2string(arg.a_type) << DELIM
               << rocblas_datatype2string(arg.c_type) << DELIM
               << rocblas_datatype2string(arg.compute_type);

            current_os    = &gemm_ex_os;
            current_entry = &gemm_ex_has_entries;
        }
        else
        {
            // NOTE: if function is not supported, AutoTunerDispatch will reject it
            ss << arg.transA << DELIM << arg.transB << DELIM << arg.M << DELIM << arg.N << DELIM
               << arg.batch_count << DELIM << arg.K << DELIM << arg.alpha << DELIM << arg.beta
               << DELIM << arg.lda << DELIM << arg.ldb << DELIM << arg.ldc << DELIM << arg.stride_a
               << DELIM << arg.stride_b << DELIM << arg.stride_c << DELIM
               << rocblas_datatype2string(arg.a_type) << DELIM
               << rocblas_datatype2string(arg.c_type) << DELIM
               << rocblas_datatype2string(arg.compute_type);

            current_os    = &gemm_strided_ex_os;
            current_entry = &gemm_strided_ex_has_entries;
        }

        std::string arg_key = ss.str();
        if(!processed.count(arg_key))
        {
            processed.insert(arg_key);

            // run benchmark
            int best_solution_index = rocblas_gemm_dispatch<GEMMTunerDispatch>(arg);

            // log result, if solution is found
            if(best_solution_index > 0)
            {
                *current_entry = true;
                *current_os << arg_key << DELIM << best_solution_index << "\n";
            }
        }
    }

    // final log
    if(gemm_ex_has_entries)
    {
        rocblas_cout << gemm_ex_os;

        if(gemm_strided_ex_has_entries)
            rocblas_cout << "\n";
    }

    if(gemm_strided_ex_has_entries)
        rocblas_cout << gemm_strided_ex_os;

    rocblas_cout << std::endl;

    test_cleanup::cleanup();
    return EXIT_SUCCESS;

#else
    rocblas_cout << "rocblas-gemm-tune ERROR: Tensile required" << std::endl;
    return EXIT_FAILURE;
#endif
}
