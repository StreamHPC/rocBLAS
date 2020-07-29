/* ************************************************************************
 * Copyright 2018-2020 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#include "rocblas_iamin_strided_batched.hpp"
#include "rocblas_reduction_impl.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_iamin_strided_batched_name[] = "unknown";
    template <>
    constexpr char rocblas_iamin_strided_batched_name<float>[] = "rocblas_isamin_strided_batched";
    template <>
    constexpr char rocblas_iamin_strided_batched_name<double>[] = "rocblas_idamin_strided_batched";
    template <>
    constexpr char rocblas_iamin_strided_batched_name<rocblas_float_complex>[]
        = "rocblas_icamin_strided_batched";
    template <>
    constexpr char rocblas_iamin_strided_batched_name<rocblas_double_complex>[]
        = "rocblas_izamin_strided_batched";

    // allocate workspace inside this API
    template <typename S, typename T>
    rocblas_status rocblas_iamin_strided_batched_impl(rocblas_handle handle,
                                                      rocblas_int    n,
                                                      const T*       x,
                                                      rocblas_int    incx,
                                                      rocblas_stride stridex,
                                                      rocblas_int    batch_count,
                                                      rocblas_int*   result)
    {
        static constexpr bool        isbatched = true;
        static constexpr rocblas_int shiftx_0  = 0;
        static constexpr int         NB        = 1024;

        rocblas_index_value_t<S>* mem = nullptr;
        rocblas_status            checks_status
            = rocblas_reduction_setup<NB, isbatched>(handle,
                                                     n,
                                                     x,
                                                     incx,
                                                     stridex,
                                                     batch_count,
                                                     result,
                                                     rocblas_iamin_strided_batched_name<T>,
                                                     "iamin_strided_batched",
                                                     mem);
        if(checks_status != rocblas_status_continue)
        {
            return checks_status;
        }

        return rocblas_iamin_template<NB, isbatched>(
            handle, n, x, shiftx_0, incx, stridex, batch_count, result, mem);
    }

}

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

#ifdef IMPL
#error IMPL IS ALREADY DEFINED
#endif

#define IMPL(name_routine_, T_, S_)                             \
    rocblas_status name_routine_(rocblas_handle handle,         \
                                 rocblas_int    n,              \
                                 const T_*      x,              \
                                 rocblas_int    incx,           \
                                 rocblas_stride stridex,        \
                                 rocblas_int    batch_count,    \
                                 rocblas_int*   results)        \
    try                                                         \
    {                                                           \
        return rocblas_iamin_strided_batched_impl<S_>(          \
            handle, n, x, incx, stridex, batch_count, results); \
    }                                                           \
    catch(...)                                                  \
    {                                                           \
        return exception_to_rocblas_status();                   \
    }

IMPL(rocblas_isamin_strided_batched, float, float);
IMPL(rocblas_idamin_strided_batched, double, double);
IMPL(rocblas_icamin_strided_batched, rocblas_float_complex, float);
IMPL(rocblas_izamin_strided_batched, rocblas_double_complex, double);

#undef IMPL

} // extern "C"
