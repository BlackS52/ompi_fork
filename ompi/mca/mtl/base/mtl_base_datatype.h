/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2022      Amazon.com, Inc. or its affiliates.  All Rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mca/mca.h"
#include "ompi/mca/mtl/mtl.h"
#include "ompi/mca/mtl/base/base.h"
#include "ompi/constants.h"
#include "ompi/datatype/ompi_datatype.h"
#include "opal/datatype/opal_convertor.h"
#include "opal/datatype/opal_datatype_internal.h"
#if OPAL_CUDA_SUPPORT
#include "opal/cuda/common_cuda.h"
#include "opal/datatype/opal_convertor.h"
#endif

#ifndef MTL_BASE_DATATYPE_H_INCLUDED
#define MTL_BASE_DATATYPE_H_INCLUDED

#if OPAL_CUDA_SUPPORT
static int
ompi_mtl_cuda_datatype_pack(struct opal_convertor_t *convertor,
                            void **buffer,
                            size_t *buffer_len,
                            bool *freeAfter)
{

    struct iovec iov;
    uint32_t iov_count = 1;
    int is_cuda = convertor->flags & CONVERTOR_CUDA;

#if !(OPAL_ENABLE_HETEROGENEOUS_SUPPORT)
    if (convertor->pDesc &&
	!(convertor->flags & CONVERTOR_COMPLETED) &&
	opal_datatype_is_contiguous_memory_layout(convertor->pDesc,
						  convertor->count)) {
	    *freeAfter = false;
	    *buffer = convertor->pBaseBuf;
	    *buffer_len = convertor->local_size;
	    return OPAL_SUCCESS;
    }
#endif

    opal_convertor_get_packed_size(convertor, buffer_len);
    *freeAfter  = false;
    if( 0 == *buffer_len ) {
        *buffer     = NULL;
        return OMPI_SUCCESS;
    }
    iov.iov_len = *buffer_len;
    iov.iov_base = NULL;
    /* opal_convertor_need_buffers always returns true
     * if CONVERTOR_CUDA is set, so unset temporarily
     */
    convertor->flags &= ~CONVERTOR_CUDA;

    if (opal_convertor_need_buffers(convertor)) {
        if (is_cuda) {
            convertor->flags |= CONVERTOR_CUDA;
        }
        iov.iov_base = opal_cuda_malloc(*buffer_len, convertor);
        if (NULL == iov.iov_base) return OMPI_ERR_OUT_OF_RESOURCE;
        *freeAfter = true;
    } else if (is_cuda) {
            convertor->flags |= CONVERTOR_CUDA;
    }

    opal_convertor_pack( convertor, &iov, &iov_count, buffer_len );

    *buffer = iov.iov_base;

    return OMPI_SUCCESS;
}
#endif

__opal_attribute_always_inline__ static inline int
ompi_mtl_datatype_pack(struct opal_convertor_t *convertor,
                       void **buffer,
                       size_t *buffer_len,
                       bool *freeAfter)
{
#if OPAL_CUDA_SUPPORT
    return ompi_mtl_cuda_datatype_pack(convertor,
                                       buffer,
                                       buffer_len,
                                       freeAfter);
#endif
    struct iovec iov;
    uint32_t iov_count = 1;

#if !(OPAL_ENABLE_HETEROGENEOUS_SUPPORT)
    if (convertor->pDesc &&
	!(convertor->flags & CONVERTOR_COMPLETED) &&
	opal_datatype_is_contiguous_memory_layout(convertor->pDesc,
						  convertor->count)) {
	    *freeAfter = false;
	    *buffer = convertor->pBaseBuf;
	    *buffer_len = convertor->local_size;
	    return OPAL_SUCCESS;
    }
#endif

    opal_convertor_get_packed_size(convertor, buffer_len);
    *freeAfter  = false;
    if( 0 == *buffer_len ) {
        *buffer     = NULL;
        return OMPI_SUCCESS;
    }
    iov.iov_len = *buffer_len;
    iov.iov_base = NULL;
    if (opal_convertor_need_buffers(convertor)) {
        iov.iov_base = malloc(*buffer_len);
        if (NULL == iov.iov_base) return OMPI_ERR_OUT_OF_RESOURCE;
        *freeAfter = true;
    }

    opal_convertor_pack( convertor, &iov, &iov_count, buffer_len );

    *buffer = iov.iov_base;

    return OMPI_SUCCESS;
}

#if OPAL_CUDA_SUPPORT
static int
ompi_mtl_cuda_datatype_recv_buf(struct opal_convertor_t *convertor,
                                void ** buffer,
                                size_t *buffer_len,
                                bool *free_on_error)
{
    int is_cuda = convertor->flags & CONVERTOR_CUDA;
    opal_convertor_get_packed_size(convertor, buffer_len);
    *free_on_error = false;
    if( 0 == *buffer_len ) {
        *buffer = NULL;
        *buffer_len = 0;
        return OMPI_SUCCESS;
    }
    /* opal_convertor_need_buffers always returns true
     * if CONVERTOR_CUDA is set, so unset temporarily
     */
    convertor->flags &= ~CONVERTOR_CUDA;
    if (opal_convertor_need_buffers(convertor)) {
        if (is_cuda) {
            convertor->flags |= CONVERTOR_CUDA;
        }
        *buffer = opal_cuda_malloc(*buffer_len, convertor);
        *free_on_error = true;
    } else {
        if (is_cuda) {
            convertor->flags |= CONVERTOR_CUDA;
        }
        *buffer = convertor->pBaseBuf +
            convertor->use_desc->desc[convertor->use_desc->used].end_loop.first_elem_disp;
    }
    return OMPI_SUCCESS;

}
#endif

__opal_attribute_always_inline__ static inline int
ompi_mtl_datatype_recv_buf(struct opal_convertor_t *convertor,
                           void ** buffer,
                           size_t *buffer_len,
                           bool *free_on_error)
{
#if OPAL_CUDA_SUPPORT
    return ompi_mtl_cuda_datatype_recv_buf(convertor,
                                           buffer,
                                           buffer_len,
                                           free_on_error);
#endif

    opal_convertor_get_packed_size(convertor, buffer_len);
    *free_on_error = false;
    if( 0 == *buffer_len ) {
        *buffer = NULL;
        *buffer_len = 0;
        return OMPI_SUCCESS;
    }
    if (opal_convertor_need_buffers(convertor)) {
        *buffer = malloc(*buffer_len);
        *free_on_error = true;
    } else {
        *buffer = convertor->pBaseBuf +
            convertor->use_desc->desc[convertor->use_desc->used].end_loop.first_elem_disp;
    }
    return OMPI_SUCCESS;
}

#if OPAL_CUDA_SUPPORT
static int
ompi_mtl_cuda_datatype_unpack(struct opal_convertor_t *convertor,
                              void *buffer,
                              size_t buffer_len) {
    struct iovec iov;
    uint32_t iov_count = 1;
    int is_cuda = convertor->flags & CONVERTOR_CUDA;
    /* opal_convertor_need_buffers always returns true
     * if CONVERTOR_CUDA is set, so unset temporarily
     */
     convertor->flags &= ~CONVERTOR_CUDA;

    if (buffer_len > 0 && opal_convertor_need_buffers(convertor)) {
        iov.iov_len = buffer_len;
        iov.iov_base = buffer;

        if (is_cuda) {
            convertor->flags |= CONVERTOR_CUDA;
        }
        opal_convertor_unpack(convertor, &iov, &iov_count, &buffer_len );

        opal_cuda_free(buffer, convertor);
    } else if (is_cuda) {
        convertor->flags |= CONVERTOR_CUDA;
    }

    return OMPI_SUCCESS;

}
#endif

__opal_attribute_always_inline__ static inline int
ompi_mtl_datatype_unpack(struct opal_convertor_t *convertor,
                         void *buffer,
                         size_t buffer_len)
{
#if OPAL_CUDA_SUPPORT
    return ompi_mtl_cuda_datatype_unpack(convertor,
                                         buffer,
                                         buffer_len);
#endif
    struct iovec iov;
    uint32_t iov_count = 1;

    if (buffer_len > 0 && opal_convertor_need_buffers(convertor)) {
        iov.iov_len = buffer_len;
        iov.iov_base = buffer;

        opal_convertor_unpack(convertor, &iov, &iov_count, &buffer_len );

        free(buffer);
    }

    return OMPI_SUCCESS;
}

#endif /* MTL_BASE_DATATYPE_H_INCLUDED */
