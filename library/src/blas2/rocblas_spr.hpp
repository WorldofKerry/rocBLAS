/* ************************************************************************
 * Copyright (C) 2016-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#pragma once

#include "check_numerics_vector.hpp"
#include "handle.hpp"

template <typename TScal, typename TConstPtr, typename TPtr>
inline rocblas_status rocblas_spr_arg_check(rocblas_handle handle,
                                            rocblas_fill   uplo,
                                            rocblas_int    n,
                                            TScal          alpha,
                                            TConstPtr      x,
                                            rocblas_stride offset_x,
                                            rocblas_int    incx,
                                            rocblas_stride stride_x,
                                            TPtr           AP,
                                            rocblas_stride offset_A,
                                            rocblas_stride stride_A,
                                            rocblas_int    batch_count)
{
    if(uplo != rocblas_fill_lower && uplo != rocblas_fill_upper)
        return rocblas_status_invalid_value;
    if(n < 0 || !incx || batch_count < 0)
        return rocblas_status_invalid_size;
    if(!n || !batch_count)
        return rocblas_status_success;

    if(!alpha)
        return rocblas_status_invalid_pointer;

    if(handle->pointer_mode == rocblas_pointer_mode_host)
    {
        if(*alpha == 0)
            return rocblas_status_success;

        // pointers are validated if they need to be dereferenced
        if(!AP || !x)
            return rocblas_status_invalid_pointer;
    }

    return rocblas_status_continue;
}

/**
 * TScal     is always: const T* (either host or device)
 * TConstPtr is either: const T* OR const T* const*
 * TPtr      is either:       T* OR       T* const*
 * Where T is the base type (float, double, rocblas_float_complex, etc.)
 */
template <typename TScal, typename TConstPtr, typename TPtr>
rocblas_status rocblas_spr_template(rocblas_handle handle,
                                    rocblas_fill   uplo,
                                    rocblas_int    n,
                                    TScal const*   alpha,
                                    TConstPtr      x,
                                    rocblas_stride offset_x,
                                    rocblas_int    incx,
                                    rocblas_stride stride_x,
                                    TPtr           AP,
                                    rocblas_stride offset_A,
                                    rocblas_stride stride_A,
                                    rocblas_int    batch_count);

//TODO :-Add rocblas_check_numerics_sp_matrix_template for checking Matrix `A` which is a Symmetric Packed Matrix
template <typename T, typename U>
rocblas_status rocblas_spr_check_numerics(const char*    function_name,
                                          rocblas_handle handle,
                                          rocblas_int    n,
                                          T              A,
                                          rocblas_stride offset_a,
                                          rocblas_stride stride_a,
                                          U              x,
                                          rocblas_stride offset_x,
                                          rocblas_int    inc_x,
                                          rocblas_stride stride_x,
                                          rocblas_int    batch_count,
                                          const int      check_numerics,
                                          bool           is_input);
