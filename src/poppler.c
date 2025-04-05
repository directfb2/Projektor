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
#include <poppler.h>

extern DirectLink *documentproviders;

/**********************************************************************************************************************/

typedef struct {
     IDirectFB           *idirectfb;

     PopplerDocument     *doc;

     DocumentDescription  desc;
} DocumentProvider_Poppler_data;

/**********************************************************************************************************************/

static DFBResult
DocumentProvider_Poppler_Init( DocumentProvider *thiz,
                               const char       *filename,
                               IDirectFB        *idirectfb )
{
     DFBResult                      ret = DFB_FAILURE;
     char                           uri[PATH_MAX];
     DocumentProvider_Poppler_data *data;

     data = D_CALLOC( 1, sizeof(DocumentProvider_Poppler_data) );
     if (!data)
          return D_OOM();;

     data->idirectfb = idirectfb;

     if (g_strstr_len( filename, -1, "://" )) {
          g_strlcpy( uri, filename, sizeof(uri) );
     }
     else {
          if (*filename == '/')
               g_snprintf( uri, sizeof(uri), "file://%s", filename );
          else
               g_snprintf( uri, sizeof(uri), "file://%s/%s", g_get_current_dir(), filename );
     }

     data->doc = poppler_document_new_from_file( uri, NULL, NULL );
     if (!data->doc)
          goto error;

     if (strrchr( filename, '/' ))
          snprintf( data->desc.title, DOCUMENT_DESC_TITLE_LENGTH, strrchr( filename, '/' ) + 1 );
     else
          snprintf( data->desc.title, DOCUMENT_DESC_TITLE_LENGTH, filename );

     data->desc.num_pages = poppler_document_get_n_pages( data->doc );

     thiz->priv = data;

     return DFB_OK;

error:
     D_FREE( data );

     return ret;
}

static DFBResult
DocumentProvider_Poppler_Term( DocumentProvider *thiz )
{
     DocumentProvider_Poppler_data *data = thiz->priv;

     g_object_unref( data->doc );

     D_FREE( data );

     return DFB_OK;
}

static DFBResult
DocumentProvider_Poppler_GetDescription( DocumentProvider    *thiz,
                                         DocumentDescription *ret_desc )
{
     DocumentProvider_Poppler_data *data = thiz->priv;

     if (!ret_desc)
          return DFB_INVARG;

     *ret_desc = data->desc;

     return DFB_OK;
}

static DFBResult
DocumentProvider_Poppler_RenderPage( DocumentProvider  *thiz,
                                     int                pageno,
                                     float              zoom,
                                     IDirectFBSurface **ret_surface )
{
     DFBResult                      ret = DFB_FAILURE;
     DFBSurfaceDescription          desc;
     int                            y;
     double                         width;
     double                         height;
     cairo_status_t                 status;
     int                            pitch;
     void                          *ptr;
     unsigned char                 *src;
     IDirectFBSurface              *surface;
     cairo_t                       *cairo  = NULL;
     PopplerPage                   *page   = NULL;
     cairo_surface_t               *pixmap = NULL;
     DocumentProvider_Poppler_data *data   = thiz->priv;

     page = poppler_document_get_page( data->doc, pageno - 1 );
     if (!page)
          goto out;

     poppler_page_get_size( page, &width, &height );

     desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     desc.width       = width  * zoom + 0.5f;
     desc.height      = height * zoom + 0.5f;
     desc.pixelformat = DSPF_ARGB;

     pixmap = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, desc.width, desc.height );
     status = cairo_surface_status( pixmap );
     if (status)
          goto out;

     cairo  = cairo_create( pixmap );
     status = cairo_status( cairo );
     if (status)
          goto out;

     cairo_scale( cairo, zoom, zoom );

     poppler_page_render( page, cairo );

     ret = data->idirectfb->CreateSurface( data->idirectfb, &desc, &surface );
     if (ret)
          goto out;

     surface->Lock( surface, DSLF_WRITE, &ptr, &pitch );

     src = cairo_image_surface_get_data( pixmap );

     for (y = 0; y < desc.height; y++) {
          direct_memcpy( ptr, src, desc.width * 4 );

          src += desc.width * 4;
          ptr += pitch;
     }

     surface->Unlock( surface );

     *ret_surface = surface;

out:
     if (cairo)
          cairo_destroy( cairo );

     if (pixmap)
          cairo_surface_destroy( pixmap );

     if (page)
          g_object_unref( page );

     return ret;
}

static DocumentProvider poppler_provider = {
     .impl           = "Poppler",
     .Init           = DocumentProvider_Poppler_Init,
     .Term           = DocumentProvider_Poppler_Term,
     .GetDescription = DocumentProvider_Poppler_GetDescription,
     .RenderPage     = DocumentProvider_Poppler_RenderPage,
};

__attribute__((constructor))
static void
DocumentProvider_Poppler_ctor()
{
     direct_list_append( &documentproviders, &poppler_provider.link );
}
