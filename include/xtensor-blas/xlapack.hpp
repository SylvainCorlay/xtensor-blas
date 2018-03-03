/***************************************************************************
* Copyright (c) 2016, Wolf Vollprecht, Johan Mabille and Sylvain Corlay    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XLAPACK_HPP
#define XLAPACK_HPP

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include "xtl/xcomplex.hpp"

#include "xtensor/xarray.hpp"
#include "xtensor/xcomplex.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xstorage.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xutils.hpp"

#include "flens/cxxlapack/cxxlapack.cxx"

#include "xblas_config.hpp"
#include "xblas_utils.hpp"

namespace xt
{

namespace lapack
{
    /**
     * Interface to LAPACK gesv.
     */
    template <class E, class F>
    int gesv(E& A, F& b)
    {
        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);
        XTENSOR_ASSERT(b.dimension() <= 2);
        XTENSOR_ASSERT(b.layout() == layout_type::column_major);

        uvector<XBLAS_INDEX> piv(A.shape()[0]);

        XBLAS_INDEX b_dim = b.dimension() > 1 ? (XBLAS_INDEX) b.shape().back() : 1;
        XBLAS_INDEX b_stride = b_dim == 1 ? (XBLAS_INDEX) b.shape().front() : (XBLAS_INDEX) b.strides().back();

        int info = cxxlapack::gesv<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            b_dim,
            A.raw_data(),
            (XBLAS_INDEX) A.strides()[1],
            piv.data(),
            b.raw_data(),
            b_stride
        );

        return info;
    }

    template <class E, class F>
    auto getrf(E& A, F& piv)
    {
        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        int info = cxxlapack::getrf<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            piv.data()
        );

        return info;
    }

    template <class E, class T>
    inline auto orgqr(E& A, T& tau, XBLAS_INDEX n = -1)
    {
        using value_type = typename E::value_type;

        uvector<value_type> work(1);

        if (n == -1)
        {
            n = (XBLAS_INDEX) A.shape()[1];
        }

        int info = cxxlapack::orgqr<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            n,
            (XBLAS_INDEX) tau.size(),
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            tau.raw_data(),
            work.data(),
            (XBLAS_INDEX) -1
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for orgqr.");
        }

        work.resize((std::size_t) work[0]);

        info = cxxlapack::orgqr<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            n,
            (XBLAS_INDEX) tau.size(),
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            tau.raw_data(),
            work.data(),
            (XBLAS_INDEX) work.size()
        );

        return info;
    }

    template <class E, class T>
    inline auto ungqr(E& A, T& tau, XBLAS_INDEX n = -1)
    {
        using value_type = typename E::value_type;

        uvector<value_type> work(1);

        if (n == -1)
        {
            n = (XBLAS_INDEX) A.shape()[1];
        }

        int info = cxxlapack::ungqr<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            n,
            (XBLAS_INDEX) tau.size(),
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            tau.raw_data(),
            work.data(),
            (XBLAS_INDEX) -1
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for ungqr.");
        }

        work.resize((std::size_t) std::real(work[0]));

        info = cxxlapack::ungqr<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            n,
            (XBLAS_INDEX) tau.size(),
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            tau.raw_data(),
            work.data(),
            (XBLAS_INDEX) work.size()
        );

        return info;
    }

    template <class E, class T>
    int geqrf(E& A, T& tau)
    {
        using value_type = typename E::value_type;

        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        uvector<value_type> work(1);

        int info = cxxlapack::geqrf<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            tau.raw_data(),
            work.data(),
            (XBLAS_INDEX) -1
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for geqrf.");
        }

        work.resize((std::size_t) std::real(work[0]));

        info = cxxlapack::geqrf<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            tau.raw_data(),
            work.data(),
            (XBLAS_INDEX) work.size()
        );

        return info;
    }

    namespace detail
    {
        template <class S>
        inline XBLAS_INDEX select_stride(const S& stride)
        {
            return stride == 0 ? 1 : static_cast<XBLAS_INDEX>(stride);
        }

        template <class U, class VT>
        inline auto init_u_vt(U& u, VT& vt, char jobz, std::size_t m, std::size_t n)
        {
            // rules for sgesdd
            // u:
            //   if jobz == 'O' and M >= N, u is not referenced
            //   if jobz == 'N', u is also not referenced
            // vt:
            //   if jobz == 'O' and M < N vt is not referenced
            //   if jobz == 'N', vt is also not referenced
            if (jobz == 'A' || (jobz == 'O' && m < n))
            {
                u.resize({m, m});
            }
            if (jobz == 'A' || (jobz == 'O' && m >= n))
            {
                vt.resize({n, n});
            }
            if (jobz == 'S')
            {
                u.resize({m, std::min(m, n)});
                vt.resize({std::min(m, n), n});
            }
            if (jobz == 'N')
            {
                // u AND vt are unreferenced -- can't use strides().back()...
                return std::make_pair(1, 1);
            }
            if (jobz == 'O')
            {
                // u OR vt are unreferenced -- can't use strides().back()...
                return m >= n ? std::make_pair(1, select_stride(vt.strides().back())) :
                                std::make_pair(select_stride(u.strides().back()), 1);
            }
            return std::make_pair(select_stride(u.strides().back()), select_stride(vt.strides().back()));
        }
    }

    template <class E, std::enable_if_t<!xtl::is_complex<typename E::value_type>::value>* = nullptr>
    auto gesdd(E& A, char jobz = 'A')
    {
        using value_type = typename E::value_type;
        using xtype1 = xtensor<value_type, 1, layout_type::column_major>;
        using xtype2 = xtensor<value_type, 2, layout_type::column_major>;

        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        uvector<value_type> work(1);

        std::size_t m = A.shape()[0];
        std::size_t n = A.shape()[1];

        xtype1 s;
        s.resize({ std::max(std::size_t(1), std::min(m, n)) });

        xtype2 u, vt;
    
        XBLAS_INDEX u_stride, vt_stride, A_stride;
        std::tie(u_stride, vt_stride) = detail::init_u_vt(u, vt, jobz, m, n);
        A_stride = detail::select_stride(A.strides().back());

        uvector<XBLAS_INDEX> iwork(8 * std::min(m, n));

        int info = cxxlapack::gesdd<XBLAS_INDEX>(
            jobz,
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            A_stride,
            s.raw_data(),
            u.raw_data(),
            u_stride,
            vt.raw_data(),
            vt_stride,
            work.data(),
            (XBLAS_INDEX) -1,
            iwork.data()
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for real gesdd.");
        }

        work.resize((std::size_t) work[0]);

        info = cxxlapack::gesdd<XBLAS_INDEX>(
            jobz,
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            A_stride,
            s.raw_data(),
            u.raw_data(),
            u_stride,
            vt.raw_data(),
            vt_stride,
            work.data(),
            (XBLAS_INDEX) work.size(),
            iwork.data()
        );

        return std::make_tuple(info, u, s, vt);
    }

    // Complex variant of gesdd
    template <class E, std::enable_if_t<xtl::is_complex<typename E::value_type>::value>* = nullptr>
    auto gesdd(E& A, char jobz = 'A')
    {
        using value_type = typename E::value_type;
        using underlying_value_type = typename value_type::value_type;
        using xtype1 = xtensor<underlying_value_type, 1, layout_type::column_major>;
        using xtype2 = xtensor<value_type, 2, layout_type::column_major>;

        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        std::size_t m = A.shape()[0];
        std::size_t n = A.shape()[1];

        uvector<value_type> work(1);
        uvector<underlying_value_type> rwork(1);
        uvector<XBLAS_INDEX> iwork(8 * std::min(m, n));

        std::size_t mx = std::max(m, n);
        std::size_t mn = std::min(m, n);
        if (jobz == 'N')
        {
            rwork.resize(5 * mn);
        }
        else if (mx > mn)
        {
            // TODO verify size
            rwork.resize(5 * mn * mn + 5 * mn);
        }
        else
        {
            // TODO verify size
            rwork.resize(std::max(5 * mn * mn + 5 * mn, 2 * mx * mn + 2 * mn * mn + mn));
        }

        xtype1 s;
        s.resize({ std::max(std::size_t(1), std::min(m, n)) });

        xtype2 u, vt;

        XBLAS_INDEX u_stride, vt_stride, A_stride;
        std::tie(u_stride, vt_stride) = detail::init_u_vt(u, vt, jobz, m, n);
        A_stride = detail::select_stride(A.strides().back());

        int info = cxxlapack::gesdd<XBLAS_INDEX>(
            jobz,
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            A_stride,
            s.raw_data(),
            u.raw_data(),
            u_stride,
            vt.raw_data(),
            vt_stride,
            work.data(),
            (XBLAS_INDEX) -1,
            rwork.data(),
            iwork.data()
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for complex gesdd.");
        }
        work.resize((std::size_t) std::real(work[0]));

        info = cxxlapack::gesdd<XBLAS_INDEX>(
            jobz,
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            A.raw_data(),
            A_stride,
            s.raw_data(),
            u.raw_data(),
            u_stride,
            vt.raw_data(),
            vt_stride,
            work.data(),
            (XBLAS_INDEX) work.size(),
            rwork.data(),
            iwork.data()
        );

        return std::make_tuple(info, u, s, vt);
    }


    template <class E>
    int potr(E& A, char uplo = 'L')
    {
        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        int info = cxxlapack::potrf<XBLAS_INDEX>(
            uplo,
            (XBLAS_INDEX) A.shape()[0],
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back()
        );

        return info;
    }

    /**
     * Interface to LAPACK getri.
     *
     * @param A matrix to invert
     * @return inverse of A
     */
    template <class E>
    int getri(E& A, uvector<XBLAS_INDEX>& piv)
    {
        using value_type = typename E::value_type;

        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        uvector<value_type> work(1);

        // get work size
        int info = cxxlapack::getri<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            piv.data(),
            work.data(),
            -1
        );

        if (info > 0)
        {
            throw std::runtime_error("Could not find workspace size for getri.");
        }

        work.resize(std::size_t(work[0]));

        info = cxxlapack::getri<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            piv.data(),
            work.data(),
            (XBLAS_INDEX) work.size()
        );

        return info;
    }

    /**
     * Interface to LAPACK geev.
     * @returns info
     */
    template <class E, class W, class V>
    int geev(E& A, char jobvl, char jobvr, W& wr, W& wi, V& VL, V& VR)
    {
        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        using value_type = typename E::value_type;
        using xtype = xtensor<value_type, 2, layout_type::column_major>;

        const auto N = A.shape()[0];
        uvector<value_type> work(1);

        int info = cxxlapack::geev<XBLAS_INDEX>(
            jobvl,
            jobvr,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            wr.raw_data(),
            wi.raw_data(),
            VL.raw_data(),
            (XBLAS_INDEX) VL.strides().back(),
            VR.raw_data(),
            (XBLAS_INDEX) VR.strides().back(),
            work.data(),
            -1
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for geev.");
        }

        work.resize(std::size_t(work[0]));

        info = cxxlapack::geev<XBLAS_INDEX>(
            jobvl,
            jobvr,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            wr.raw_data(),
            wi.raw_data(),
            VL.raw_data(),
            (XBLAS_INDEX) VL.strides().back(),
            VR.raw_data(),
            (XBLAS_INDEX) VR.strides().back(),
            work.data(),
            (XBLAS_INDEX) work.size()
        );

        return info;
    }

    /**
     * Interface to LAPACK syevd.
     * @returns info
     */
    template <class E, class W>
    int syevd(E& A, char jobz, char uplo, W& w)
    {
        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        using value_type = typename E::value_type;
        using xtype = xtensor<value_type, 2, layout_type::column_major>;

        auto N = A.shape()[0];
        uvector<value_type> work(1);
        uvector<XBLAS_INDEX> iwork(1);

        int info = cxxlapack::syevd<XBLAS_INDEX>(
            jobz,
            uplo,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            w.raw_data(),
            work.data(),
            (XBLAS_INDEX) -1,
            iwork.data(),
            (XBLAS_INDEX) -1
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for syevd.");
        }

        work.resize(std::size_t(work[0]));
        iwork.resize(std::size_t(iwork[0]));

        info = cxxlapack::syevd<XBLAS_INDEX>(
            jobz,
            uplo,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            w.raw_data(),
            work.data(),
            (XBLAS_INDEX) work.size(),
            iwork.data(),
            (XBLAS_INDEX) iwork.size()
        );

        return info;
    }

    /**
     * Complex version of geev
     */
    template <class E, class W, class V>
    int geev(E& A, char jobvl, char jobvr, W& w, V& VL, V& VR)
    {
        // TODO implement for complex numbers

        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        using value_type = typename E::value_type;
        using underlying_value_type = typename value_type::value_type;
        using xtype = xtensor<value_type, 2, layout_type::column_major>;

        const auto N = A.shape()[0];
        uvector<value_type> work(1);
        uvector<underlying_value_type> rwork(2 * N);

        int info = cxxlapack::geev<XBLAS_INDEX>(
            jobvl,
            jobvr,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            w.raw_data(),
            VL.raw_data(),
            (XBLAS_INDEX) VL.strides().back(),
            VR.raw_data(),
            (XBLAS_INDEX) VR.strides().back(),
            work.data(),
            -1,
            rwork.data()
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for geev.");
        }

        work.resize(std::size_t(std::real(work[0])));

        info = cxxlapack::geev<XBLAS_INDEX>(
            jobvl,
            jobvr,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            w.raw_data(),
            VL.raw_data(),
            (XBLAS_INDEX) VL.strides().back(),
            VR.raw_data(),
            (XBLAS_INDEX) VR.strides().back(),
            work.data(),
            (XBLAS_INDEX) work.size(),
            rwork.data()
        );

        return info;
    }

    template <class E, class W>
    int heevd(E& A, char jobz, char uplo, W& w)
    {
        XTENSOR_ASSERT(A.dimension() == 2);
        XTENSOR_ASSERT(A.layout() == layout_type::column_major);

        using value_type = typename E::value_type;
        using underlying_value_type = typename value_type::value_type;
        using xtype = xtensor<value_type, 2, layout_type::column_major>;

        auto N = A.shape()[0];
        uvector<value_type> work(1);
        uvector<underlying_value_type> rwork(1);
        uvector<XBLAS_INDEX> iwork(1);

        int info = cxxlapack::heevd<XBLAS_INDEX>(
            jobz,
            uplo,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            w.raw_data(),
            work.data(),
            (XBLAS_INDEX) -1,
            rwork.data(),
            (XBLAS_INDEX) -1,
            iwork.data(),
            (XBLAS_INDEX) -1
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for heevd.");
        }

        work.resize(std::size_t(std::real(work[0])));
        rwork.resize(std::size_t(rwork[0]));
        iwork.resize(std::size_t(iwork[0]));

        info = cxxlapack::heevd<XBLAS_INDEX>(
            jobz,
            uplo,
            (XBLAS_INDEX) N,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            w.raw_data(),
            work.data(),
            (XBLAS_INDEX) work.size(),
            rwork.data(),
            (XBLAS_INDEX) rwork.size(),
            iwork.data(),
            (XBLAS_INDEX) iwork.size()
        );

        return info;
    }

    template <class E, class F, class S, std::enable_if_t<!xtl::is_complex<typename E::value_type>::value>* = nullptr>
    int gelsd(E& A, F& b, S& s, XBLAS_INDEX& rank, double rcond)
    {
        using value_type = typename E::value_type;

        uvector<value_type> work(1);
        uvector<XBLAS_INDEX> iwork(1);

        XBLAS_INDEX b_dim = b.dimension() > 1 ? (XBLAS_INDEX) b.shape().back() : 1;
        XBLAS_INDEX b_stride = b_dim == 1 ? (XBLAS_INDEX) b.shape().front() : (XBLAS_INDEX) b.strides().back();

        int info = cxxlapack::gelsd<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            b_dim,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            b.raw_data(),
            b_stride,
            s.raw_data(),
            rcond,
            rank,
            work.data(),
            (XBLAS_INDEX) -1,
            iwork.data()
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for gelsd.");
        }

        work.resize(std::size_t(work[0]));
        iwork.resize(std::size_t(iwork[0]));

        info = cxxlapack::gelsd<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            b_dim,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            b.raw_data(),
            b_stride,
            s.raw_data(),
            rcond,
            rank,
            work.data(),
            (XBLAS_INDEX) work.size(),
            iwork.data()
        );

        return info;
    }

    template <class E, class F, class S, std::enable_if_t<xtl::is_complex<typename E::value_type>::value>* = nullptr>
    int gelsd(E& A, F& b, S& s, XBLAS_INDEX& rank, double rcond = -1)
    {
        using value_type = typename E::value_type;
        using underlying_value_type = typename value_type::value_type;

        uvector<value_type> work(1);
        uvector<underlying_value_type> rwork(1);
        uvector<XBLAS_INDEX> iwork(1);

        XBLAS_INDEX b_dim = b.dimension() > 1 ? (XBLAS_INDEX) b.shape().back() : 1;
        XBLAS_INDEX b_stride = b_dim == 1 ? (XBLAS_INDEX) b.shape().front() : (XBLAS_INDEX) b.strides().back();

        int info = cxxlapack::gelsd<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            b_dim,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            b.raw_data(),
            b_stride,
            s.raw_data(),
            rcond,
            rank,
            work.data(),
            (XBLAS_INDEX) -1,
            rwork.data(),
            iwork.data()
        );

        if (info != 0)
        {
            throw std::runtime_error("Could not find workspace size for gelsd.");
        }

        work.resize(std::size_t(std::real(work[0])));
        rwork.resize(std::size_t(rwork[0]));
        iwork.resize(std::size_t(iwork[0]));

        info = cxxlapack::gelsd<XBLAS_INDEX>(
            (XBLAS_INDEX) A.shape()[0],
            (XBLAS_INDEX) A.shape()[1],
            b_dim,
            A.raw_data(),
            (XBLAS_INDEX) A.strides().back(),
            b.raw_data(),
            b_stride,
            s.raw_data(),
            rcond,
            rank,
            work.data(),
            (XBLAS_INDEX) work.size(),
            rwork.data(),
            iwork.data()
        );

        return info;
    }
}

}
#endif
