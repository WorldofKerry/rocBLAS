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
#include "Tensile/gemm.hpp"
#include "logging.hpp"
#include "rocblas_block_sizes.h"
#include "rocblas_syr2k_her2k.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_her2k_name[] = "unknown";
    template <>
    constexpr char rocblas_her2k_name<rocblas_float_complex>[] = "rocblas_cher2k_batched";
    template <>
    constexpr char rocblas_her2k_name<rocblas_double_complex>[] = "rocblas_zher2k_batched";

    template <typename T>
    rocblas_status rocblas_her2k_batched_impl(rocblas_handle    handle,
                                              rocblas_fill      uplo,
                                              rocblas_operation trans,
                                              rocblas_int       n,
                                              rocblas_int       k,
                                              const T*          alpha,
                                              const T* const    A[],
                                              rocblas_int       lda,
                                              const T* const    B[],
                                              rocblas_int       ldb,
                                              const real_t<T>*  beta,
                                              T* const          C[],
                                              rocblas_int       ldc,
                                              rocblas_int       batch_count)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        // Copy alpha and beta to host if on device. This is because gemm is called and it
        // requires alpha and beta to be on host
        T         alpha_h;
        real_t<T> beta_h;
        RETURN_IF_ROCBLAS_ERROR(
            rocblas_copy_alpha_beta_to_host_if_on_device(handle, alpha, beta, alpha_h, beta_h, k));
        auto saved_pointer_mode = handle->push_pointer_mode(rocblas_pointer_mode_host);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;
        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto uplo_letter   = rocblas_fill_letter(uplo);
            auto transA_letter = rocblas_transpose_letter(trans);

            if(layer_mode & rocblas_layer_mode_log_trace)
                log_trace(handle,
                          rocblas_her2k_name<T>,
                          uplo,
                          trans,
                          n,
                          k,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          A,
                          lda,
                          B,
                          ldb,
                          LOG_TRACE_SCALAR_VALUE(handle, beta),
                          C,
                          ldc,
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f her2k_batched -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "--transposeA",
                          transA_letter,
                          "-n",
                          n,
                          "-k",
                          k,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--lda",
                          lda,
                          "--ldb",
                          ldb,
                          LOG_BENCH_SCALAR_VALUE(handle, beta),
                          "--ldc",
                          ldc,
                          "--batch_count",
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_her2k_name<T>,
                            "uplo",
                            uplo_letter,
                            "trans",
                            transA_letter,
                            "N",
                            n,
                            "K",
                            k,
                            "lda",
                            lda,
                            "ldb",
                            ldb,
                            "ldc",
                            ldc,
                            "batch_count",
                            batch_count);
        }

        static constexpr rocblas_stride offset_C = 0, offset_A = 0, offset_B = 0;
        static constexpr rocblas_stride stride_C = 0, stride_A = 0, stride_B = 0;

        rocblas_status arg_status = rocblas_her2k_arg_check(handle,
                                                            uplo,
                                                            trans,
                                                            n,
                                                            k,
                                                            alpha,
                                                            A,
                                                            offset_A,
                                                            lda,
                                                            stride_A,
                                                            B,
                                                            offset_B,
                                                            ldb,
                                                            stride_B,
                                                            beta,
                                                            C,
                                                            offset_C,
                                                            ldc,
                                                            stride_C,
                                                            batch_count);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        static constexpr bool Hermetian = true;
        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status her2k_check_numerics_status
                = rocblas_her2k_syr2k_check_numerics<Hermetian>(rocblas_her2k_name<T>,
                                                                handle,
                                                                uplo,
                                                                trans,
                                                                n,
                                                                k,
                                                                A,
                                                                lda,
                                                                stride_A,
                                                                B,
                                                                ldb,
                                                                stride_B,
                                                                C,
                                                                ldc,
                                                                stride_C,
                                                                batch_count,
                                                                check_numerics,
                                                                is_input);

            if(her2k_check_numerics_status != rocblas_status_success)
                return her2k_check_numerics_status;
        }

        rocblas_status status = rocblas_internal_her2k_batched_template<T>(handle,
                                                                           uplo,
                                                                           trans,
                                                                           n,
                                                                           k,
                                                                           alpha,
                                                                           A,
                                                                           offset_A,
                                                                           lda,
                                                                           stride_A,
                                                                           B,
                                                                           offset_B,
                                                                           ldb,
                                                                           stride_B,
                                                                           beta,
                                                                           C,
                                                                           offset_C,
                                                                           ldc,
                                                                           stride_C,
                                                                           batch_count);

        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status her2k_check_numerics_status
                = rocblas_her2k_syr2k_check_numerics<Hermetian>(rocblas_her2k_name<T>,
                                                                handle,
                                                                uplo,
                                                                trans,
                                                                n,
                                                                k,
                                                                A,
                                                                lda,
                                                                stride_A,
                                                                B,
                                                                ldb,
                                                                stride_B,
                                                                C,
                                                                ldc,
                                                                stride_C,
                                                                batch_count,
                                                                check_numerics,
                                                                is_input);

            if(her2k_check_numerics_status != rocblas_status_success)
                return her2k_check_numerics_status;
        }
        return status;
    }

}
/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, T_)                                                           \
    rocblas_status routine_name_(rocblas_handle    handle,                                \
                                 rocblas_fill      uplo,                                  \
                                 rocblas_operation trans,                                 \
                                 rocblas_int       n,                                     \
                                 rocblas_int       k,                                     \
                                 const T_*         alpha,                                 \
                                 const T_* const   A[],                                   \
                                 rocblas_int       lda,                                   \
                                 const T_* const   B[],                                   \
                                 rocblas_int       ldb,                                   \
                                 const real_t<T_>* beta,                                  \
                                 T_* const         C[],                                   \
                                 rocblas_int       ldc,                                   \
                                 rocblas_int       batch_count)                           \
    try                                                                                   \
    {                                                                                     \
        return rocblas_her2k_batched_impl(                                                \
            handle, uplo, trans, n, k, alpha, A, lda, B, ldb, beta, C, ldc, batch_count); \
    }                                                                                     \
    catch(...)                                                                            \
    {                                                                                     \
        return exception_to_rocblas_status();                                             \
    }

IMPL(rocblas_cher2k_batched, rocblas_float_complex);
IMPL(rocblas_zher2k_batched, rocblas_double_complex);

#undef IMPL

} // extern "C"
