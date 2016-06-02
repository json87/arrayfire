/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <Array.hpp>
#include <math.hpp>

namespace cpu
{
namespace kernel
{

template<typename T>
void stridedCopy(T* dst, af::dim4 const & ostrides, T const * src,
                 af::dim4 const & dims, af::dim4 const & strides, unsigned dim)
{
    if(dim == 0) {
        if(strides[dim] == 1) {
            //FIXME: Check for errors / exceptions
            memcpy(dst, src, dims[dim] * sizeof(T));
        } else {
            for(dim_t i = 0; i < dims[dim]; i++) {
                dst[i] = src[strides[dim]*i];
            }
        }
    } else {
        for(dim_t i = dims[dim]; i > 0; i--) {
            stridedCopy<T>(dst, ostrides, src, dims, strides, dim - 1);
            src += strides[dim];
            dst += ostrides[dim];
        }
    }
}

template<typename OutT, typename InT>
void copyElemwise(Array<OutT> dst, Array<InT> const src, OutT default_value, double factor)
{
    af::dim4 src_dims       = src.dims();
    af::dim4 dst_dims       = dst.dims();
    af::dim4 src_strides    = src.strides();
    af::dim4 dst_strides    = dst.strides();

    InT const * const src_ptr = src.get();
    OutT * dst_ptr      = dst.get();

    dim_t trgt_l = std::min(dst_dims[3], src_dims[3]);
    dim_t trgt_k = std::min(dst_dims[2], src_dims[2]);
    dim_t trgt_j = std::min(dst_dims[1], src_dims[1]);
    dim_t trgt_i = std::min(dst_dims[0], src_dims[0]);

    for(dim_t l=0; l<dst_dims[3]; ++l) {

        dim_t src_loff = l*src_strides[3];
        dim_t dst_loff = l*dst_strides[3];
        bool isLvalid = l<trgt_l;

        for(dim_t k=0; k<dst_dims[2]; ++k) {

            dim_t src_koff = k*src_strides[2];
            dim_t dst_koff = k*dst_strides[2];
            bool isKvalid = k<trgt_k;

            for(dim_t j=0; j<dst_dims[1]; ++j) {

                dim_t src_joff = j*src_strides[1];
                dim_t dst_joff = j*dst_strides[1];
                bool isJvalid = j<trgt_j;

                for(dim_t i=0; i<dst_dims[0]; ++i) {
                    OutT temp = default_value;
                    if (isLvalid && isKvalid && isJvalid && i<trgt_i) {
                        dim_t src_idx = i*src_strides[0] + src_joff + src_koff + src_loff;
                        temp = OutT(src_ptr[src_idx])*OutT(factor);
                    }
                    dim_t dst_idx = i*dst_strides[0] + dst_joff + dst_koff + dst_loff;
                    dst_ptr[dst_idx] = temp;
                }
            }
        }
    }
}

template<typename OutT, typename InT>
struct CopyImpl
{
    static void copy(Array<OutT> dst, Array<InT> const src)
    {
        copyElemwise(dst, src, scalar<OutT>(0), 1.0);
    }
};

template<typename T>
struct CopyImpl<T, T>
{
    static void copy(Array<T> dst, Array<T> const src)
    {
        af::dim4 src_dims       = src.dims();
        af::dim4 dst_dims       = dst.dims();
        af::dim4 src_strides    = src.strides();
        af::dim4 dst_strides    = dst.strides();

        T const * const src_ptr = src.get();
        T * dst_ptr = dst.get();

        // find the major-most dimension, which is linear in both arrays
        int linear_end = 0;
        dim_t count = 1;
        while (linear_end < 4
                && count == src_strides[linear_end]
                && count == dst_strides[linear_end]) {
            ++linear_end;
            count *= src_dims[linear_end];
        }

        // traverse through the array using strides only until neccessary
        if (linear_end == 4) {
            std::memcpy(dst_ptr, src_ptr, sizeof(T) * src.elements());

        } else {
            for(dim_t l=0; l<dst_dims[3]; ++l) {
                dim_t src_loff = l*src_strides[3];
                dim_t dst_loff = l*dst_strides[3];

                if (linear_end == 3) {
                    dim_t dst_idx = dst_loff;
                    dim_t src_idx = src_loff;
                    std::memcpy(dst_ptr + dst_idx, src_ptr + src_idx, sizeof(T) * src_strides[3]);

                } else {
                    for(dim_t k=0; k<dst_dims[2]; ++k) {
                        dim_t src_koff = k*src_strides[2];
                        dim_t dst_koff = k*dst_strides[2];

                        if (linear_end == 2) {
                            dim_t dst_idx = dst_koff + dst_loff;
                            dim_t src_idx = src_koff + src_loff;
                            std::memcpy(dst_ptr + dst_idx, src_ptr + src_idx, sizeof(T) * src_strides[2]);

                        } else {
                            for(dim_t j=0; j<dst_dims[1]; ++j) {
                                dim_t src_joff = j*src_strides[1];
                                dim_t dst_joff = j*dst_strides[1];

                                if (linear_end == 1) {
                                    dim_t dst_idx = dst_joff + dst_koff + dst_loff;
                                    dim_t src_idx = src_joff + src_koff + src_loff;
                                    std::memcpy(dst_ptr + dst_idx, src_ptr + src_idx, sizeof(T) * src_strides[1]);

                                } else {
                                    for(dim_t i=0; i<dst_dims[0]; ++i) {
                                        dim_t dst_idx = i*dst_strides[0] + dst_joff + dst_koff + dst_loff;
                                        dim_t src_idx = i*src_strides[0] + src_joff + src_koff + src_loff;
                                        dst_ptr[dst_idx] = src_ptr[src_idx];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};

template<typename OutT, typename InT>
void copy(Array<OutT> dst, Array<InT> const src)
{
    CopyImpl<OutT, InT>::copy(dst, src);
}

}
}
