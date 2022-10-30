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
#include <libdjvu/ddjvuapi.h>

extern DirectLink *documentproviders;

/**********************************************************************************************************************/

typedef struct {
     IDirectFB           *idirectfb;

     ddjvu_context_t     *ctx;
     ddjvu_document_t    *doc;

     DocumentDescription  desc;
} DocumentProvider_DjVu_data;

/**********************************************************************************************************************/

static DFBResult
DocumentProvider_DjVu_Init( DocumentProvider *thiz,
                            const char       *filename,
                            IDirectFB        *idirectfb )
{
     DFBResult                    ret = DFB_FAILURE;
     DocumentProvider_DjVu_data *data;

     data = D_CALLOC( 1, sizeof(DocumentProvider_DjVu_data) );
     if (!data)
          return D_OOM();;

     data->idirectfb = idirectfb;

     data->ctx = ddjvu_context_create( "DjVu" );
     if (!data->ctx)
          goto error;

     data->doc = ddjvu_document_create_by_filename( data->ctx, filename, TRUE );
     if (!data->ctx)
          goto error;

     ddjvu_message_wait( data->ctx );
     ddjvu_message_pop( data->ctx );

     if (strrchr( filename, '/' ))
          snprintf( data->desc.title, DOCUMENT_DESC_TITLE_LENGTH, strrchr( filename, '/' ) + 1 );
     else
          snprintf( data->desc.title, DOCUMENT_DESC_TITLE_LENGTH, filename );

     data->desc.num_pages = ddjvu_document_get_pagenum( data->doc );

     thiz->priv = data;

     return DFB_OK;

error:
     if (data->ctx)
          ddjvu_context_release( data->ctx );

     D_FREE( data );

     return ret;
}

static DFBResult
DocumentProvider_DjVu_Term( DocumentProvider *thiz )
{
     DocumentProvider_DjVu_data *data = thiz->priv;

     ddjvu_document_release( data->doc );

     ddjvu_context_release( data->ctx );

     D_FREE( data );

     return DFB_OK;
}

static DFBResult
DocumentProvider_DjVu_GetDescription( DocumentProvider    *thiz,
                                      DocumentDescription *ret_desc )
{
     DocumentProvider_DjVu_data *data = thiz->priv;

     if (!ret_desc)
          return DFB_INVARG;

     *ret_desc = data->desc;

     return DFB_OK;
}

static DFBResult
DocumentProvider_DjVu_RenderPage( DocumentProvider  *thiz,
                                  int                pageno,
                                  float              zoom,
                                  IDirectFBSurface **ret_surface )
{
     DFBResult                   ret = DFB_FAILURE;
     DFBSurfaceDescription       desc;
     int                         pitch;
     int                         y;
     int                         dpi;
     ddjvu_rect_t                rect;
     void                       *ptr;
     unsigned char              *src;
     IDirectFBSurface           *surface;
     ddjvu_format_t             *format = NULL;
     ddjvu_page_t               *page   = NULL;
     char                       *pixmap = NULL;
     DocumentProvider_DjVu_data *data   = thiz->priv;

     page = ddjvu_page_create_by_pageno( data->doc, pageno - 1 );
     if (!page)
          goto out;

     while (!ddjvu_page_decoding_done( page )) {
          ddjvu_message_wait( data->ctx );
          ddjvu_message_pop( data->ctx );
     }

     dpi = ddjvu_page_get_resolution( page );

     desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     desc.width       = ddjvu_page_get_width( page )  * 100 * zoom / dpi;
     desc.height      = ddjvu_page_get_height( page ) * 100 * zoom / dpi;
     desc.pixelformat = DSPF_RGB24;

     pixmap = D_MALLOC( desc.width * desc.height * 4 );
     if (!pixmap)
          goto out;

     format = ddjvu_format_create( DDJVU_FORMAT_BGR24, 0, NULL );
     if (!format)
          goto out;

     ddjvu_format_set_row_order( format, 1 );

     rect.x = 0;
     rect.y = 0;
     rect.w = desc.width;
     rect.h = desc.height;

     ddjvu_page_render( page, DDJVU_RENDER_COLOR, &rect, &rect, format, desc.width * 3, pixmap );

     ret = data->idirectfb->CreateSurface( data->idirectfb, &desc, &surface );
     if (ret)
          goto out;

     surface->Lock( surface, DSLF_WRITE, &ptr, &pitch );

     src = (unsigned char*) pixmap;

     for (y = 0; y < desc.height; y++) {
          direct_memcpy( ptr, src, desc.width * 3 );

          src += desc.width * 3;
          ptr += pitch;
     }

     surface->Unlock( surface );

     *ret_surface = surface;

out:
     if (format)
          ddjvu_format_release( format );

     if (pixmap)
          D_FREE( pixmap );

     if (page)
          ddjvu_page_release( page );

     return ret;
}

static DocumentProvider djvu_provider = {
     .impl           = "DjVu",
     .Init           = DocumentProvider_DjVu_Init,
     .Term           = DocumentProvider_DjVu_Term,
     .GetDescription = DocumentProvider_DjVu_GetDescription,
     .RenderPage     = DocumentProvider_DjVu_RenderPage,
};

__attribute__((constructor))
static void
DocumentProvider_DjVu_ctor()
{
     direct_list_append( &documentproviders, &djvu_provider.link );
}
