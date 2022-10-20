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
#include <lite/label.h>
#include <lite/lite.h>
#include <lite/progressbar.h>
#include <lite/textline.h>
#include <lite/window.h>

DirectLink *documentproviders;

/**********************************************************************************************************************/

typedef struct {
     LiteBox          box;

     DFBColor         background;
     LiteLabel       *label_page;
     LiteLabel       *label_zoom;
     LiteLabel       *label_title;
     LiteProgressBar *progressbar;
} StatusBar;

static DFBResult
StatusBarInit( StatusBar  *statusbar,
               int         x,
               int         y,
               int         width,
               int         height,
               LiteWindow *window )
{
     DFBResult    ret;
     DFBRectangle rect_page     = {           0, 1,         140, height - 2 };
     DFBRectangle rect_zoom     = {         180, 1,          80, height - 2 };
     DFBRectangle rect_title    = {         300, 1, width - 440, height - 2 };
     DFBRectangle rect_progress = { width - 230, 0,         230, height };
     DFBColor     white         = { 0xff, 0xf0, 0xf0, 0xf0 };

     memset( statusbar, 0, sizeof(StatusBar) );

     /* Set parent box. */
     statusbar->box.parent = LITE_BOX(window);

     /* Set bounds within the parent. */
     statusbar->box.rect.x = x;
     statusbar->box.rect.y = y;
     statusbar->box.rect.w = width;
     statusbar->box.rect.h = height;

     /* Set background color. */
     statusbar->background.a   = 0xd0;
     statusbar->background.r   = 0x00;
     statusbar->background.g   = 0x23;
     statusbar->background.b   = 0x42;
     statusbar->box.background = &statusbar->background;

     /* Initialize the box. */
     ret = lite_init_box( &statusbar->box );
     if (ret)
          return ret;

     /* Setup the page label. */
     ret = lite_new_label( &statusbar->box, &rect_page, liteNoLabelTheme, rect_page.h - 3, &statusbar->label_page );
     if (ret)
          return ret;

     lite_set_label_alignment( statusbar->label_page, LITE_LABEL_RIGHT );
     lite_set_label_color( statusbar->label_page, &white );

     /* Setup the zoom label. */
     lite_new_label( &statusbar->box, &rect_zoom, liteNoLabelTheme, rect_zoom.h - 3, &statusbar->label_zoom );
     if (ret)
          return ret;

     lite_set_label_alignment( statusbar->label_zoom, LITE_LABEL_RIGHT );
     lite_set_label_color( statusbar->label_zoom, &white );

     /* Setup the title label. */
     lite_new_label( &statusbar->box, &rect_title, liteNoLabelTheme, rect_title.h - 3, &statusbar->label_title );
     lite_set_label_alignment( statusbar->label_title, LITE_LABEL_CENTER );
     lite_set_label_color( statusbar->label_title, &white );

     /* Setup the progress bar. */
     lite_new_progressbar( &statusbar->box, &rect_progress, liteNoProgressBarTheme, &statusbar->progressbar );
     if (ret)
          return ret;

     lite_set_progressbar_images( statusbar->progressbar, DATADIR"/progress_on.dfiff", DATADIR"/progress_off.dfiff" );

     return DFB_OK;
}

static DFBResult
StatusBarSetProgress( StatusBar *statusbar,
                      float      value )
{
     if (getenv( "PROJEKTOR_NO_PROGRESS" ))
          return DFB_OK;

     return lite_set_progressbar_value( statusbar->progressbar, value );
}

static DFBResult
StatusBarSetPage( StatusBar *statusbar,
                  int        pageno,
                  int        num_pages )
{
     char text[16];

     snprintf( text, sizeof(text), "%d/%d", pageno, num_pages );

     return lite_set_label_text( statusbar->label_page, text );
}

static DFBResult
StatusBarSetZoom( StatusBar *statusbar,
                  int        percentage )
{
     char text[8];

     snprintf( text, sizeof(text), "%d%%", percentage );

     return lite_set_label_text( statusbar->label_zoom, text );
}

static DFBResult
StatusBarSetTitle( StatusBar  *statusbar,
                   const char *title )
{
     return lite_set_label_text( statusbar->label_title, title );
}

/**********************************************************************************************************************/

typedef struct {
     LiteBox           box;

     DFBColor          background;
     DFBPoint          offset;
     DFBPoint          offset_max;
     DFBRectangle      image_rect;
     IDirectFBSurface *image;
} PageView;

static DFBResult
PageView_Draw( LiteBox         *box,
               const DFBRegion *region,
               DFBBoolean       clear )
{
     PageView         *pageview = (PageView*) box;
     IDirectFBSurface *surface  = box->surface;

     if (pageview->image) {
          surface->Blit( surface, pageview->image, NULL,
                         pageview->image_rect.x - pageview->offset.x,
                         pageview->image_rect.y - pageview->offset.y );
     }

     return DFB_OK;
}

static DFBResult
PageView_Destroy( LiteBox *box )
{
     PageView *pageview = (PageView*) box;

     if (pageview->image)
          pageview->image->Release( pageview->image );

     return lite_destroy_box( box );
}

static DFBResult
PageViewInit( PageView   *pageview,
              int         x,
              int         y,
              int         width,
              int         height,
              LiteWindow *window )
{
     DFBResult ret;

     memset( pageview, 0, sizeof(PageView) );

     /* Set parent box. */
     pageview->box.parent = LITE_BOX(window);

     /* Set bounds within the parent. */
     pageview->box.rect.x = x;
     pageview->box.rect.y = y;
     pageview->box.rect.w = width;
     pageview->box.rect.h = height;

     /* Set background color. */
     pageview->background.a   = 0xff;
     pageview->background.r   = 0x00;
     pageview->background.g   = 0x00;
     pageview->background.b   = 0x23;
     pageview->box.background = &pageview->background;

     /* Initialize the box. */
     ret = lite_init_box( &pageview->box );
     if (ret)
          return ret;

     /* Set callbacks. */
     pageview->box.Draw    = PageView_Draw;
     pageview->box.Destroy = PageView_Destroy;

     return DFB_OK;
}

static DFBResult
PageViewSetImage( PageView         *pageview,
                  IDirectFBSurface *image )
{
     DFBResult ret;
     int       width;
     int       height;

     ret = image->AddRef( image );
     if (ret)
          return ret;

     if (pageview->image)
          pageview->image->Release( pageview->image );

     pageview->image = image;

     image->GetSize( image, &width, &height );

     if (width > pageview->box.rect.w) {
          pageview->image_rect.x = 0;
          pageview->image_rect.w = pageview->box.rect.w;
          pageview->offset_max.x = width - pageview->box.rect.w;
     }
     else {
          pageview->image_rect.x = (pageview->box.rect.w - width) / 2;
          pageview->image_rect.w = width;
          pageview->offset_max.x = 0;
     }

     if (height > pageview->box.rect.h) {
          pageview->image_rect.y = 0;
          pageview->image_rect.h = pageview->box.rect.h;
          pageview->offset_max.y = height - pageview->box.rect.h;
     }
     else {
          pageview->image_rect.y = (pageview->box.rect.h - height) / 2;
          pageview->image_rect.h = height;
          pageview->offset_max.y = 0;
     }

     if (pageview->offset.x > pageview->offset_max.x)
          pageview->offset.x = pageview->offset_max.x;

     if (pageview->offset.y > pageview->offset_max.y)
          pageview->offset.y = pageview->offset_max.y;

     lite_update_box( &pageview->box, NULL );

     return DFB_OK;
}

static DFBResult
PageViewScroll( PageView *pageview,
                int       dx,
                int       dy )
{
     DFBPoint offset = pageview->offset;

     offset.x += dx;
     offset.y += dy;

     if (offset.x < 0)
          offset.x = 0;

     if (offset.y < 0)
          offset.y = 0;

     if (offset.x > pageview->offset_max.x)
          offset.x = pageview->offset_max.x;

     if (offset.y > pageview->offset_max.y)
          offset.y = pageview->offset_max.y;

     if (pageview->offset.x != offset.x || pageview->offset.y != offset.y) {
          pageview->offset = offset;

          lite_update_box( &pageview->box, NULL );
     }

     return DFB_OK;
}

/**********************************************************************************************************************/

typedef struct {
     LiteWindow *window;

     PageView    pageview;
     StatusBar   statusbar;
} MainWindow;

static DFBResult
MainWindowInit( MainWindow *mainwin,
                int         width,
                int         height )
{
     DFBResult    ret;
     DFBRectangle rect = { 0, 0, width, height };

     /* Create a window. */
     ret = lite_new_window( NULL, &rect, DWCAPS_NONE, liteNoWindowTheme, "Projektor", &mainwin->window );
     if (ret)
          return ret;

     /* Initialize page view. */
     ret = PageViewInit( &mainwin->pageview, 0, 0, width, height - 23, mainwin->window );
     if (ret)
          return ret;

     /* Initialize status bar. */
     ret = StatusBarInit( &mainwin->statusbar, 0, height - 23, width, 23, mainwin->window );
     if (ret)
          return ret;

     /* Show the window. */
     lite_set_window_opacity( mainwin->window, liteFullWindowOpacity );

     return DFB_OK;
}

/**********************************************************************************************************************/

typedef struct {
     MainWindow           mainwin;

     DocumentProvider    *provider;

     DocumentDescription  desc;

     bool                 error;
     bool                 quit;

     int                  pageno;
     float                zoom;
     float                zoom_prev;

     LiteTextLine        *textline;
} Projektor;

static DFBResult ProjektorKeyboardFunc( DFBWindowEvent *evt, void *data );

static DFBResult
ProjektorInit( Projektor  *projektor,
               const char *renderer,
               const char *filename,
               int         width,
               int         height,
               float       zoom )
{
     DFBResult         ret;
     DocumentProvider *provider;

     /* Get the display layer size. */
     if (!width || !height)
          lite_get_layer_size( &width, &height );

     /* Create the main window. */
     ret = MainWindowInit( &projektor->mainwin, width, height );
     if (ret)
          return ret;

     /* Install raw keyboard event callback. */
     lite_on_raw_window_keyboard( projektor->mainwin.window, ProjektorKeyboardFunc, projektor );

     /* Select document provider. */
     direct_list_foreach (provider, documentproviders) {
          if (!strcasecmp( provider->impl, renderer )) {
               projektor->provider = provider;
               break;
          }
     }

     /* Initialize document provider. */
     ret = provider->Init( provider, filename, lite_get_dfb_interface() );
     if (ret) {
          StatusBarSetTitle( &projektor->mainwin.statusbar, "Cannot open file" );
          return ret;
     }

     /* Get document description. */
     provider->GetDescription( provider, &projektor->desc );

     /* Set status bar. */
     StatusBarSetTitle( &projektor->mainwin.statusbar, projektor->desc.title );
     StatusBarSetZoom( &projektor->mainwin.statusbar, 100 * zoom );

     projektor->error     = false;
     projektor->quit      = false;
     projektor->pageno    = 0;
     projektor->zoom      = zoom;
     projektor->zoom_prev = zoom;

     /* No goto page text line at startup. */
     projektor->textline = NULL;

     return DFB_OK;
}

static DFBResult
ProjektorGotoPage( Projektor *projektor,
                   int        pageno )
{
     DFBResult         ret;
     IDirectFBSurface *image;
     DocumentProvider *provider = projektor->provider;

     if (pageno < 1)
          pageno = 1;

     if (pageno > projektor->desc.num_pages)
          pageno = projektor->desc.num_pages;

     if (pageno == projektor->pageno)
          return DFB_OK;

     ret = provider->RenderPage( provider, pageno, projektor->zoom, &image );
     if (ret) {
          StatusBarSetTitle( &projektor->mainwin.statusbar, "Cannot render page" );
          projektor->error = true;
          return ret;
     }

     if (projektor->error)
          StatusBarSetTitle( &projektor->mainwin.statusbar, projektor->desc.title );

     PageViewSetImage( &projektor->mainwin.pageview, image );

     image->Release( image );

     /* Update status bar. */
     StatusBarSetProgress( &projektor->mainwin.statusbar, (float) (pageno - 1) / (projektor->desc.num_pages - 1) );
     StatusBarSetPage( &projektor->mainwin.statusbar, pageno, projektor->desc.num_pages );

     projektor->pageno = pageno;

     return DFB_OK;
}

static DFBResult
ProjektorSetZoom( Projektor *projektor,
                  float      zoom )
{
     DFBResult         ret;
     IDirectFBSurface *image;
     DocumentProvider *provider = projektor->provider;

     if (zoom < 0.25f)
          zoom = 0.25f;

     if (zoom > 2.5f)
          zoom = 2.5f;

     if (zoom == projektor->zoom)
          return DFB_OK;

     ret = provider->RenderPage( provider, projektor->pageno, zoom, &image );
     if (ret) {
          StatusBarSetTitle( &projektor->mainwin.statusbar, "Cannot render page" );
          projektor->error = true;
          return ret;
     }

     if (projektor->error)
          StatusBarSetTitle( &projektor->mainwin.statusbar, projektor->desc.title );

     PageViewSetImage( &projektor->mainwin.pageview, image );

     image->Release( image );

     /* Update status bar. */
     StatusBarSetZoom( &projektor->mainwin.statusbar, 100 * zoom );

     projektor->zoom = zoom;

     return DFB_OK;
}

static DFBResult
ProjektorSetOptimal( Projektor *projektor )
{
     DFBResult         ret;
     float             zw, zh;
     float             zoom;
     PageView         *pageview = &projektor->mainwin.pageview;
     DocumentProvider *provider = projektor->provider;

     /* Get document description. */
     provider->GetDescription( provider, &projektor->desc );

     zw = (float) pageview->box.rect.w / (projektor->desc.width / projektor->zoom);
     zh = (float) pageview->box.rect.h / (projektor->desc.height / projektor->zoom);

     zoom = (zw < zh) ? zw : zh;

     if ((int) (100 * zoom / 25 + 0.5f) != (int) (100 * projektor->zoom / 25 + 0.5f)) {
          projektor->zoom_prev = projektor->zoom;

          ret = ProjektorSetZoom( projektor, zoom );
     }
     else
          ret = ProjektorSetZoom( projektor, projektor->zoom_prev );

     return ret;
}

static DFBResult
ProjektorEventLoop( Projektor *projektor )
{
     while (!projektor->quit) {
          /* Run window event loop for 200 ms. */
          lite_window_event_loop( projektor->mainwin.window, 200 );
     }

     return DFB_OK;
}

static DFBResult
ProjektorKeyboardFunc( DFBWindowEvent *evt,
                       void           *data )
{
     Projektor *projektor = data;
     PageView  *pageview  = &projektor->mainwin.pageview;

     switch (evt->key_symbol) {
          case DIKS_CURSOR_UP:
               if (evt->type == DWET_KEYDOWN)
                    PageViewScroll( pageview, 0, -pageview->box.rect.h / 5 );

               return DFB_BUSY;

          case DIKS_CURSOR_DOWN:
               if (evt->type == DWET_KEYDOWN)
                    PageViewScroll( pageview, 0, pageview->box.rect.h / 5 );

               return DFB_BUSY;

          case DIKS_CURSOR_LEFT:
               if (projektor->textline)
                    return DFB_OK;

               if (evt->type == DWET_KEYDOWN)
                    PageViewScroll( pageview, -pageview->box.rect.w / 5, 0 );

               return DFB_BUSY;

          case DIKS_CURSOR_RIGHT:
               if (projektor->textline)
                    return DFB_OK;

               if (evt->type == DWET_KEYDOWN)
                    PageViewScroll( pageview, pageview->box.rect.w / 5, 0 );

               return DFB_BUSY;

          case DIKS_PAGE_UP:
          case DIKS_CHANNEL_UP:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorGotoPage( projektor, projektor->pageno - 1 );

               return DFB_BUSY;

          case DIKS_PAGE_DOWN:
          case DIKS_CHANNEL_DOWN:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorGotoPage( projektor, projektor->pageno + 1 );

               return DFB_BUSY;

          case DIKS_HOME:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorGotoPage( projektor, 1 );

               return DFB_BUSY;

          case DIKS_END:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorGotoPage( projektor, projektor->desc.num_pages );

               return DFB_BUSY;

          case DIKS_PLUS_SIGN:
          case DIKS_VOLUME_UP:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorSetZoom( projektor, projektor->zoom + 0.25f );

               return DFB_BUSY;

          case DIKS_MINUS_SIGN:
          case DIKS_VOLUME_DOWN:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorSetZoom( projektor, projektor->zoom - 0.25f );

               return DFB_BUSY;

          case DIKS_SPACE:
          case DIKS_MENU:
               if (evt->type == DWET_KEYDOWN)
                    ProjektorSetOptimal( projektor );

               return DFB_BUSY;

          case DIKS_0:
          case DIKS_1:
          case DIKS_2:
          case DIKS_3:
          case DIKS_4:
          case DIKS_5:
          case DIKS_6:
          case DIKS_7:
          case DIKS_8:
          case DIKS_9:
               if (projektor->textline)
                    return DFB_OK;

               if (evt->type == DWET_KEYDOWN) {
                    DFBResult    ret;
                    char         initial_text[2] = { evt->key_symbol, 0 };
                    DFBRectangle rect            = {
                         (projektor->mainwin.window->box.rect.w - 54) / 2,
                         (projektor->mainwin.window->box.rect.h - 27) / 2,
                         54,
                         27
                    };

                    ret = lite_new_textline( LITE_BOX(projektor->mainwin.window), &rect, liteNoTextLineTheme,
                                             &projektor->textline );
                    if (ret)
                         break;

                    lite_set_textline_text( projektor->textline, initial_text );

                    lite_focus_box( LITE_BOX(projektor->textline) );
               }

               return DFB_BUSY;

          case DIKS_BACKSPACE:
          case DIKS_DELETE:
               if (projektor->textline)
                    return DFB_OK;

               return DFB_BUSY;

          case DIKS_ENTER:
          case DIKS_OK:
               if (projektor->textline) {
                    char *text;

                    lite_get_textline_text( projektor->textline, &text );

                    lite_destroy_box( LITE_BOX(projektor->textline) );
                    projektor->textline = NULL;

                    ProjektorGotoPage( projektor, atoi( text ) );

                    D_FREE( text );
               }

               return DFB_BUSY;

          case DIKS_ESCAPE:
          case DIKS_MUTE:
               if (projektor->textline) {
                    lite_destroy_box( LITE_BOX(projektor->textline) );
                    projektor->textline = NULL;
               }
               else
                    projektor->quit = true;

               return DFB_BUSY;

          default:
               break;
     }

     /* Prevent non-numeric characters. */
     if (projektor->textline)
          return DFB_BUSY;

     return DFB_OK;
}

static void
ProjektorTerm( Projektor *projektor )
{
     DocumentProvider *provider = projektor->provider;

     /* Deinitialize document provider. */
     provider->Term( provider );
}

/**********************************************************************************************************************/

static void print_usage()
{
     DocumentProvider *provider;

     printf( "DirectFB Document Viewer\n\n" );
     printf( "Usage: projektor [options] filename\n\n" );
     printf( "Options:\n\n" );
     printf( "  -o, --optimal                    Use optimal zoom factor.\n" );
     printf( "  -r, --renderer <renderer>        Set document renderer.\n" );
     printf( "  -s, --size     <width>x<height>  Set viewer size.\n" );
     printf( "  -z, --zoom     <zoom>            Set zoom factor.\n" );
     printf( "  -h, --help                       Print usage information.\n\n" );
     printf( "Supported renderers:\n\n" );
     direct_list_foreach (provider, documentproviders) {
          printf( "  %s\n", provider->impl );
     }
     printf( "\n" );
}

int main( int argc, char *argv[] )
{
     DFBResult   ret;
     Projektor   projektor;
     int         n;
     int         width    = 0;
     int         height   = 0;
     float       zoom     = 1.0f;
     bool        optimal  = false;
     const char *renderer = NULL;
     const char *filename = NULL;

     /* Parse command line. */
     for (n = 1; n < argc; n++) {
          if (strcmp( argv[n], "-h" ) == 0 || strcmp( argv[n], "--help" ) == 0) {
               print_usage();
               return 0;
          }

          if (strcmp( argv[n], "-o" ) == 0 || strcmp( argv[n], "--optimal" ) == 0) {
               optimal = true;
               continue;
          }

          if (strcmp( argv[n], "-r" ) == 0 || strcmp( argv[n], "--renderer" ) == 0) {
               DocumentProvider *provider;

               if (++n == argc) {
                    print_usage (argv[0]);
                    return 1;
               }

               direct_list_foreach (provider, documentproviders) {
                    if (!strcasecmp( provider->impl, argv[n] )) {
                         renderer = argv[n];
                         break;
                    }
               }

               if (!renderer) {
                    DirectFBError( "Invalid renderer", DFB_FAILURE );
                    return 1;
               }

               continue;
          }

          if (strcmp( argv[n], "-s" ) == 0 || strcmp( argv[n], "--size" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return 1;
               }

               if (sscanf( argv[n], "%dx%d", &width, &height ) != 2 || width < 1 || height < 1) {
                    DirectFBError( "Invalid size", DFB_FAILURE );
                    return 1;
               }

               continue;
          }

          if (strcmp( argv[n], "-z" ) == 0 || strcmp( argv[n], "--zoom" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               if (sscanf( argv[n], "%f", &zoom ) != 1 || zoom < 0.25 || zoom > 2.5f) {
                    DirectFBError( "Invalid zoom factor", DFB_FAILURE );
                    return 1;
               }

               continue;
          }

          if (*argv[n] != '-' && !filename) {
               filename = argv[n];
               continue;
          }

          print_usage();
          return 1;
     }

     if (!filename) {
          print_usage();
          return 1;
     }

     if (!renderer)
          renderer = ((DocumentProvider*) documentproviders)->impl;

     /* Initialization. */
     if (lite_open( &argc, &argv ))
          return 1;

     ret = ProjektorInit( &projektor, renderer, filename, width, height, zoom );
     if (ret) {
          lite_close();
          return 1;
     }

     /* Render first page. */
     ret = ProjektorGotoPage( &projektor, 1 );
     if (ret)
          goto out;

     if (optimal) {
          ret = ProjektorSetOptimal( &projektor );
          if (ret)
               goto out;
     }

     /* Run the window event loop. */
     ProjektorEventLoop( &projektor );

out:
     /* Deinitialization. */
     ProjektorTerm( &projektor );

     lite_close();

     return !ret ? 0 : 1;
}
