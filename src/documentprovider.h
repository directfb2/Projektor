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

#include <direct/list.h>
#include <directfb.h>

/*
 * Information about a document.
 */

#define DOCUMENT_DESC_TITLE_LENGTH 255

typedef struct {
     char title[DOCUMENT_DESC_TITLE_LENGTH];
     int  num_pages;
     int  width;
     int  height;
} DocumentDescription;

/*
 * Document provider interface.
 */

typedef struct _DocumentProvider DocumentProvider;

struct _DocumentProvider {
     DirectLink   link;
     const char  *impl;
     void        *priv;

     DFBResult  (*Init)          ( DocumentProvider *thiz, const char *filename, IDirectFB *idirectfb );
     DFBResult  (*Term)          ( DocumentProvider *thiz );
     DFBResult  (*GetDescription)( DocumentProvider *thiz, DocumentDescription *ret_desc );
     DFBResult  (*RenderPage)    ( DocumentProvider *thiz, int pageno, float zoom, IDirectFBSurface **ret_surface );
};
