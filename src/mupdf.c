/*
   This file is part of Projektor.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "documentprovider.h"
#include <direct/memcpy.h>
#include <fitz.h>

extern DirectLink *documentproviders;

/**********************************************************************************************************************/

typedef struct {
     IDirectFB           *idirectfb;

     fz_context          *ctx;
     fz_document         *doc;

     DocumentDescription  desc;
} DocumentProvider_MuPDF_data;

/**********************************************************************************************************************/

static DFBResult
DocumentProvider_MuPDF_Init( DocumentProvider *thiz,
                             const char       *filename,
                             IDirectFB        *idirectfb )
{
     DFBResult                    ret = DFB_FAILURE;
     DocumentProvider_MuPDF_data *data;

     data = D_CALLOC( 1, sizeof(DocumentProvider_MuPDF_data) );
     if (!data)
          return D_OOM();;

     data->idirectfb = idirectfb;

     data->ctx = fz_new_context( NULL, NULL, FZ_STORE_DEFAULT );
     if (!data->ctx)
          goto error;

     fz_try( data->ctx ) {
          data->doc = fz_open_document( data->ctx, filename );
     }
     fz_catch( data->ctx ) {
          goto error;
     }

     if (strrchr( filename, '/' ))
          snprintf( data->desc.title, DOCUMENT_DESC_TITLE_LENGTH, strrchr( filename, '/' ) + 1 );
     else
          snprintf( data->desc.title, DOCUMENT_DESC_TITLE_LENGTH, filename );

     data->desc.num_pages = fz_count_pages( data->doc );

     thiz->priv = data;

     return DFB_OK;

error:
     if (data->ctx)
          fz_free_context( data->ctx );

     D_FREE( data );

     return ret;
}

static DFBResult
DocumentProvider_MuPDF_Term( DocumentProvider *thiz )
{
     DocumentProvider_MuPDF_data *data = thiz->priv;

     fz_close_document( data->doc );

     fz_free_context( data->ctx );

     D_FREE( data );

     return DFB_OK;
}

static DFBResult
DocumentProvider_MuPDF_GetDescription( DocumentProvider    *thiz,
                                       DocumentDescription *ret_desc )
{
     DocumentProvider_MuPDF_data *data = thiz->priv;

     if (!ret_desc)
          return DFB_INVARG;

     *ret_desc = data->desc;

     return DFB_OK;
}

static DFBResult
DocumentProvider_MuPDF_RenderPage( DocumentProvider  *thiz,
                                   int                pageno,
                                   float              zoom,
                                   IDirectFBSurface **ret_surface )
{
     DFBResult                    ret = DFB_FAILURE;
     DFBSurfaceDescription        desc;
     int                          pitch;
     int                          y;
     fz_matrix                    matrix;
     fz_rect                      rect;
     fz_irect                     irect;
     void                        *ptr;
     unsigned char               *src;
     IDirectFBSurface            *surface;
     fz_device                   *device = NULL;
     fz_page                     *page   = NULL;
     fz_pixmap                   *pixmap = NULL;
     DocumentProvider_MuPDF_data *data   = thiz->priv;

     fz_try( data->ctx ) {
          page = fz_load_page( data->doc, pageno - 1 );
     }
     fz_catch( data->ctx ) {
          goto out;
     }

     fz_rotate( &matrix, 0 );
     fz_pre_scale( &matrix, zoom, zoom );
     fz_bound_page( data->doc, page, &rect );
     fz_transform_rect( &rect, &matrix );
     fz_round_rect( &irect, &rect );

     desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     desc.width       = irect.x1 - irect.x0;
     desc.height      = irect.y1 - irect.y0;
     desc.pixelformat = DSPF_ABGR;

     fz_try( data->ctx ) {
          pixmap = fz_new_pixmap_with_bbox( data->ctx, fz_device_rgb, &irect );

          device = fz_new_draw_device( data->ctx, pixmap );

          fz_run_page( data->doc, page, device, &matrix, NULL );
     }
     fz_catch( data->ctx ) {
          goto out;
     }

     ret = data->idirectfb->CreateSurface( data->idirectfb, &desc, &surface );
     if (ret)
          goto out;

     surface->Lock( surface, DSLF_WRITE, &ptr, &pitch );

     src = fz_pixmap_samples( data->ctx, pixmap );

     for (y = 0; y < desc.height; y++) {
          direct_memcpy( ptr, src, desc.width * 4 );

          src += desc.width * 4;
          ptr += pitch;
     }

     surface->Unlock( surface );

     *ret_surface = surface;

out:
     if (device)
          fz_free_device( device );

     if (pixmap)
          fz_drop_pixmap( data->ctx, pixmap );

     if (page)
          fz_free_page( data->doc, page );

     return ret;
}

static DocumentProvider mupdf_provider = {
     .impl           = "MuPDF",
     .Init           = DocumentProvider_MuPDF_Init,
     .Term           = DocumentProvider_MuPDF_Term,
     .GetDescription = DocumentProvider_MuPDF_GetDescription,
     .RenderPage     = DocumentProvider_MuPDF_RenderPage,
};

__attribute__((constructor))
static void
DocumentProvider_MuPDF_ctor()
{
     direct_list_append( &documentproviders, &mupdf_provider.link );
}
