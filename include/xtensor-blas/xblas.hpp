/***************************************************************************
* Copyright (c) 2016, Wolf Vollprecht, Johan Mabille and Sylvain Corlay    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XBLAS_HPP
#define XBLAS_HPP

#include "xtensor/xarray.hpp"
#include "xtensor/xcomplex.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xutils.hpp"

#include "flens/cxxblas/cxxblas.cxx"

#include "xblas_config.hpp"
#include "xblas_utils.hpp"

namespace xt
{

namespace blas
{
    /**
     * Calculate the 1-norm of a vector
     *
     * @param a vector of n elements
     * @returns scalar result
     */
    template <class E, class R>
    void asum(const xexpression<E>& a, R& result)
    {
        auto&& ad = view_eval<E::static_layout>(a.derived_cast());
        XTENSOR_ASSERT(ad.dimension() == 1);

        cxxblas::asum<BLAS_IDX>(
            (BLAS_IDX) ad.shape()[0],
            ad.raw_data() + ad.raw_data_offset(),
            (BLAS_IDX) ad.strides().front(),
            result
        );
    }

    /**
     * Calculate the 2-norm of a vector
     *
     * @param a vector of n elements
     * @returns scalar result
     */
    template <class E, class R>
    void nrm2(const xexpression<E>& a, R& result)
    {
        auto&& ad = view_eval<E::static_layout>(a.derived_cast());
        XTENSOR_ASSERT(ad.dimension() == 1);

        cxxblas::nrm2<BLAS_IDX>(
            (BLAS_IDX) ad.shape()[0],
            ad.raw_data() + ad.raw_data_offset(),
            (BLAS_IDX) ad.strides().front(),
            result
        );
    }

    /**
     * Calculate the dot product between two vectors, conjugating
     * the first argument \em a in the case of complex vectors.
     *
     * @param a vector of n elements
     * @param b vector of n elements
     * @returns scalar result
     */
    template <class E1, class E2, class R>
    void dot(const xexpression<E1>& a, const xexpression<E2>& b,
             R& result)
    {
        auto&& ad = view_eval<E1::static_layout>(a.derived_cast());
        auto&& bd = view_eval<E2::static_layout>(b.derived_cast());
        XTENSOR_ASSERT(ad.dimension() == 1);

        cxxblas::dot<BLAS_IDX>(
            (BLAS_IDX) ad.shape()[0],
            ad.raw_data() + ad.raw_data_offset(),
            (BLAS_IDX) ad.strides().front(),
            bd.raw_data() + bd.raw_data_offset(),
            (BLAS_IDX) bd.strides().front(),
            result
        );
    }

    /**
     * Calculate the dot product between two complex vectors, not conjugating the
     * first argument \em a.
     *
     * @param a vector of n elements
     * @param b vector of n elements
     * @returns scalar result
     */
    template <class E1, class E2, class R>
    void dotu(const xexpression<E1>& a, const xexpression<E2>& b, R& result)
    {
        auto&& ad = view_eval<E1::static_layout>(a.derived_cast());
        auto&& bd = view_eval<E2::static_layout>(b.derived_cast());
        XTENSOR_ASSERT(ad.dimension() == 1);

        cxxblas::dotu<BLAS_IDX>(
            (BLAS_IDX) ad.shape()[0],
            ad.raw_data() + ad.raw_data_offset(),
            (BLAS_IDX) ad.strides().front(),
            bd.raw_data() + bd.raw_data_offset(),
            (BLAS_IDX) bd.strides().front(),
            result
        );
    }

    /**
     * Calculate the general matrix times vector product according to
     * ``y := alpha * A * x + beta * y``.
     *
     * @param A matrix of n x m elements
     * @param x vector of n elements
     * @param transpose select if A should be transposed
     * @param alpha scalar scale factor
     * @returns the resulting vector
     */
    template <class E1, class E2, class R, class value_type = typename E1::value_type>
    void gemv(const xexpression<E1>& A, const xexpression<E2>& x,
              R& result,
              bool transpose_A = false,
              const value_type& alpha = value_type(1.0),
              const value_type& beta = value_type(0.0))
    {
        auto&& dA = view_eval<E1::static_layout>(A.derived_cast());
        auto&& dx = view_eval<E2::static_layout>(x.derived_cast());

        cxxblas::gemv<BLAS_IDX>(
            get_blas_storage_order(result),
            transpose_A ? cxxblas::Transpose::Trans : cxxblas::Transpose::NoTrans,
            (BLAS_IDX) dA.shape()[0],
            (BLAS_IDX) dA.shape()[1],
            alpha,
            dA.raw_data() + dA.raw_data_offset(),
            get_leading_stride(dA),
            dx.raw_data() + dx.raw_data_offset(),
            get_leading_stride(dx),
            beta,
            result.raw_data() + result.raw_data_offset(),
            get_leading_stride(result)
        );
    }

    /**
     * Calculate the matrix-matrix product of matrix @A and matrix @B
     *
     * C := alpha * A * B + beta * C
     *
     * @param A matrix of m-by-n elements
     * @param B matrix of n-by-k elements
     * @param transpose_A transpose A on the fly
     * @param transpose_B transpose B on the fly
     * @param alpha scale factor for A * B (defaults to 1)
     * @param beta scale factor for C (defaults to 0)
     */
    template <class E, class F, class R, class value_type = typename E::value_type>
    void gemm(const xexpression<E>& A, const xexpression<F>& B, R& result,
              char transpose_A = false,
              char transpose_B = false,
              const value_type& alpha = value_type(1.0),
              const value_type& beta = value_type(0.0))
    {
        static_assert(R::static_layout != layout_type::dynamic, "GEMM result layout cannot be dynamic.");
        auto&& dA = view_eval<R::static_layout>(A.derived_cast());
        auto&& dB = view_eval<R::static_layout>(B.derived_cast());

        XTENSOR_ASSERT(dA.layout() == dB.layout());
        XTENSOR_ASSERT(result.layout() == dA.layout());
        XTENSOR_ASSERT(dA.dimension() == 2);
        XTENSOR_ASSERT(dB.dimension() == 2);

        cxxblas::gemm<BLAS_IDX>(
            get_blas_storage_order(result),
            transpose_A ? cxxblas::Transpose::Trans : cxxblas::Transpose::NoTrans,
            transpose_B ? cxxblas::Transpose::Trans : cxxblas::Transpose::NoTrans,
            (BLAS_IDX) (transpose_A ? dA.shape()[1] : dA.shape()[0]),
            (BLAS_IDX) (transpose_B ? dB.shape()[0] : dB.shape()[1]),
            (BLAS_IDX) (transpose_B ? dB.shape()[1] : dB.shape()[0]),
            alpha,
            dA.raw_data() + dA.raw_data_offset(),
            get_leading_stride(dA),
            dB.raw_data() + dB.raw_data_offset(),
            get_leading_stride(dB),
            beta,
            result.raw_data() + result.raw_data_offset(),
            get_leading_stride(result)
        );
    }

    /**
     * Calculate the outer product of vector x and y.
     * According to A:= alpha * x * y' + A
     *
     * @param x vector of n elements
     * @param y vector of m elements
     * @param alpha scalar scale factor
     * @returns matrix of n-by-m elements
     */
    template <class E1, class E2, class R, class value_type = typename E1::value_type>
    void ger(const xexpression<E1>& x, const xexpression<E2>& y,
             R& result,
             const value_type& alpha = value_type(1.0))
    {
        auto&& dx = view_eval(x.derived_cast());
        auto&& dy = view_eval(y.derived_cast());

        XTENSOR_ASSERT(dx.dimension() == 1);
        XTENSOR_ASSERT(dy.dimension() == 1);

        cxxblas::ger<BLAS_IDX>(
            get_blas_storage_order(result),
            (BLAS_IDX) dx.shape()[0],
            (BLAS_IDX) dy.shape()[0],
            alpha,
            dx.raw_data() + dx.raw_data_offset(),
            (BLAS_IDX) dx.strides().front(),
            dy.raw_data() + dy.raw_data_offset(),
            (BLAS_IDX) dy.strides().front(),
            result.raw_data() + result.raw_data_offset(),
            (BLAS_IDX) result.strides().front()
        );
    }
}
}
#endif
