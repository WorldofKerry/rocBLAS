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
#include "rocblas_ger.hpp"
#include "handle.hpp"
#include "logging.hpp"
#include "rocblas.h"
#include "utility.hpp"

namespace
{
    template <bool, typename>
    constexpr char rocblas_ger_name[] = "unknown";
    template <>
    constexpr char rocblas_ger_name<false, float>[] = "rocblas_sger";
    template <>
    constexpr char rocblas_ger_name<false, double>[] = "rocblas_dger";
    template <>
    constexpr char rocblas_ger_name<false, rocblas_float_complex>[] = "rocblas_cgeru";
    template <>
    constexpr char rocblas_ger_name<false, rocblas_double_complex>[] = "rocblas_zgeru";
    template <>
    constexpr char rocblas_ger_name<true, rocblas_float_complex>[] = "rocblas_cgerc";
    template <>
    constexpr char rocblas_ger_name<true, rocblas_double_complex>[] = "rocblas_zgerc";

    template <bool, typename>
    constexpr char rocblas_ger_fn_name[] = "unknown";
    template <>
    constexpr char rocblas_ger_fn_name<false, float>[] = "ger";
    template <>
    constexpr char rocblas_ger_fn_name<false, double>[] = "ger";
    template <>
    constexpr char rocblas_ger_fn_name<false, rocblas_float_complex>[] = "geru";
    template <>
    constexpr char rocblas_ger_fn_name<false, rocblas_double_complex>[] = "geru";
    template <>
    constexpr char rocblas_ger_fn_name<true, rocblas_float_complex>[] = "gerc";
    template <>
    constexpr char rocblas_ger_fn_name<true, rocblas_double_complex>[] = "gerc";

    template <bool CONJ, typename T>
    rocblas_status rocblas_ger_impl(rocblas_handle handle,
                                    rocblas_int    m,
                                    rocblas_int    n,
                                    const T*       alpha,
                                    const T*       x,
                                    rocblas_int    incx,
                                    const T*       y,
                                    rocblas_int    incy,
                                    T*             A,
                                    rocblas_int    lda)
    {
        if(!handle)
            return rocblas_status_invalid_handle;
        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;
        if(layer_mode & rocblas_layer_mode_log_trace)
            log_trace(handle,
                      rocblas_ger_name<CONJ, T>,
                      m,
                      n,
                      LOG_TRACE_SCALAR_VALUE(handle, alpha),
                      x,
                      incx,
                      y,
                      incy,
                      A,
                      lda);

        if(layer_mode & rocblas_layer_mode_log_bench)
            log_bench(handle,
                      "./rocblas-bench -f",
                      rocblas_ger_fn_name<CONJ, T>,
                      "-r",
                      rocblas_precision_string<T>,
                      "-m",
                      m,
                      "-n",
                      n,
                      LOG_BENCH_SCALAR_VALUE(handle, alpha),
                      "--incx",
                      incx,
                      "--incy",
                      incy,
                      "--lda",
                      lda);

        if(layer_mode & rocblas_layer_mode_log_profile)
            log_profile(handle,
                        rocblas_ger_name<CONJ, T>,
                        "M",
                        m,
                        "N",
                        n,
                        "incx",
                        incx,
                        "incy",
                        incy,
                        "lda",
                        lda);

        rocblas_status arg_status = rocblas_ger_arg_check<CONJ, T>(
            handle, m, n, alpha, 0, x, 0, incx, 0, y, 0, incy, 0, A, 0, lda, 0, 1);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status ger_check_numerics_status
                = rocblas_ger_check_numerics(rocblas_ger_name<CONJ, T>,
                                             handle,
                                             m,
                                             n,
                                             A,
                                             0,
                                             lda,
                                             0,
                                             x,
                                             0,
                                             incx,
                                             0,
                                             y,
                                             0,
                                             incy,
                                             0,
                                             1,
                                             check_numerics,
                                             is_input);
            if(ger_check_numerics_status != rocblas_status_success)
                return ger_check_numerics_status;
        }

        rocblas_status status;
        if constexpr(rocblas_is_complex<T> && CONJ)
            status = rocblas_internal_gerc_template(
                handle, m, n, alpha, 0, x, 0, incx, 0, y, 0, incy, 0, A, 0, lda, 0, 1);
        else
            status = rocblas_internal_ger_template(
                handle, m, n, alpha, 0, x, 0, incx, 0, y, 0, incy, 0, A, 0, lda, 0, 1);

        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status ger_check_numerics_status
                = rocblas_ger_check_numerics(rocblas_ger_name<CONJ, T>,
                                             handle,
                                             m,
                                             n,
                                             A,
                                             0,
                                             lda,
                                             0,
                                             x,
                                             0,
                                             incx,
                                             0,
                                             y,
                                             0,
                                             incy,
                                             0,
                                             1,
                                             check_numerics,
                                             is_input);
            if(ger_check_numerics_status != rocblas_status_success)
                return ger_check_numerics_status;
        }
        return status;
    }

} // namespace

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, CONJ_, T_)                                                     \
    rocblas_status routine_name_(rocblas_handle handle,                                    \
                                 rocblas_int    m,                                         \
                                 rocblas_int    n,                                         \
                                 const T_*      alpha,                                     \
                                 const T_*      x,                                         \
                                 rocblas_int    incx,                                      \
                                 const T_*      y,                                         \
                                 rocblas_int    incy,                                      \
                                 T_*            A,                                         \
                                 rocblas_int    lda)                                       \
    try                                                                                    \
    {                                                                                      \
        return rocblas_ger_impl<CONJ_, T_>(handle, m, n, alpha, x, incx, y, incy, A, lda); \
    }                                                                                      \
    catch(...)                                                                             \
    {                                                                                      \
        return exception_to_rocblas_status();                                              \
    }

IMPL(rocblas_sger, false, float);
IMPL(rocblas_dger, false, double);
IMPL(rocblas_cgeru, false, rocblas_float_complex);
IMPL(rocblas_zgeru, false, rocblas_double_complex);
IMPL(rocblas_cgerc, true, rocblas_float_complex);
IMPL(rocblas_zgerc, true, rocblas_double_complex);

#undef IMPL

} // extern "C"
