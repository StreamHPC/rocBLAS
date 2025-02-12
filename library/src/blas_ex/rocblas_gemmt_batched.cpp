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
#include "logging.hpp"
#include "rocblas_gemmt.hpp"
#include "utility.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_gemmt_name[] = "unknown";
    template <>
    constexpr char rocblas_gemmt_name<float>[] = "rocblas_sgemmt_batched";
    template <>
    constexpr char rocblas_gemmt_name<double>[] = "rocblas_dgemmt_batched";
    template <>
    constexpr char rocblas_gemmt_name<rocblas_float_complex>[] = "rocblas_cgemmt_batched";
    template <>
    constexpr char rocblas_gemmt_name<rocblas_double_complex>[] = "rocblas_zgemmt_batched";

    template <typename T>
    rocblas_status rocblas_gemmt_batched_impl(rocblas_handle    handle,
                                              rocblas_fill      uplo,
                                              rocblas_operation transA,
                                              rocblas_operation transB,
                                              rocblas_int       n,
                                              rocblas_int       k,
                                              const T*          alpha,
                                              const T* const    A[],
                                              rocblas_int       lda,
                                              const T* const    B[],
                                              rocblas_int       ldb,
                                              const T*          beta,
                                              T* const          C[],
                                              rocblas_int       ldc,
                                              rocblas_int       batch_count)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;
        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto uplo_letter   = rocblas_fill_letter(uplo);
            auto transA_letter = rocblas_transpose_letter(transA);
            auto transB_letter = rocblas_transpose_letter(transB);

            if(layer_mode & rocblas_layer_mode_log_trace)
                log_trace(handle,
                          rocblas_gemmt_name<T>,
                          uplo,
                          transA,
                          transB,
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
                          "./rocblas-bench -f gemmt_batched -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "--transposeA",
                          transA_letter,
                          "--transposeB",
                          transB_letter,
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
                            rocblas_gemmt_name<T>,
                            "uplo",
                            uplo_letter,
                            "transA",
                            transA_letter,
                            "--transposeB",
                            transB_letter,
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

        static constexpr rocblas_stride stride_C = 0, stride_A = 0, stride_B = 0;

        rocblas_status arg_status = rocblas_gemmt_arg_check(
            handle, uplo, transA, transB, n, k, alpha, A, lda, B, ldb, beta, C, ldc, batch_count);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status gemmt_check_numerics_status
                = rocblas_gemmt_check_numerics<T>(rocblas_gemmt_name<T>,
                                                  handle,
                                                  uplo,
                                                  transA,
                                                  transB,
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

            if(gemmt_check_numerics_status != rocblas_status_success)
                return gemmt_check_numerics_status;
        }

        rocblas_status status = rocblas_internal_gemmt_template(handle,
                                                                uplo,
                                                                transA,
                                                                transB,
                                                                n,
                                                                k,
                                                                alpha,
                                                                A,
                                                                lda,
                                                                stride_A,
                                                                B,
                                                                ldb,
                                                                stride_B,
                                                                beta,
                                                                C,
                                                                ldc,
                                                                stride_C,
                                                                batch_count);

        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status gemmt_check_numerics_status
                = rocblas_gemmt_check_numerics<T>(rocblas_gemmt_name<T>,
                                                  handle,
                                                  uplo,
                                                  transA,
                                                  transB,
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

            if(gemmt_check_numerics_status != rocblas_status_success)
                return gemmt_check_numerics_status;
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

#define IMPL(routine_name_, T_)                                                                    \
    rocblas_status routine_name_(rocblas_handle    handle,                                         \
                                 rocblas_fill      uplo,                                           \
                                 rocblas_operation transA,                                         \
                                 rocblas_operation transB,                                         \
                                 rocblas_int       n,                                              \
                                 rocblas_int       k,                                              \
                                 const T_*         alpha,                                          \
                                 const T_* const   A[],                                            \
                                 rocblas_int       lda,                                            \
                                 const T_* const   B[],                                            \
                                 rocblas_int       ldb,                                            \
                                 const T_*         beta,                                           \
                                 T_* const         C[],                                            \
                                 rocblas_int       ldc,                                            \
                                 rocblas_int       batch_count)                                    \
    try                                                                                            \
    {                                                                                              \
        return rocblas_gemmt_batched_impl(                                                         \
            handle, uplo, transA, transB, n, k, alpha, A, lda, B, ldb, beta, C, ldc, batch_count); \
    }                                                                                              \
    catch(...)                                                                                     \
    {                                                                                              \
        return exception_to_rocblas_status();                                                      \
    }

IMPL(rocblas_sgemmt_batched, float);
IMPL(rocblas_dgemmt_batched, double);
IMPL(rocblas_cgemmt_batched, rocblas_float_complex);
IMPL(rocblas_zgemmt_batched, rocblas_double_complex);

#undef IMPL

} // extern "C"
