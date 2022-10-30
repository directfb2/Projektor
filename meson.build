#  This file is part of Projektor.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

project('Projektor', 'c',
        version: '0.9.1')

projektordatadir = join_paths(get_option('prefix'), get_option('datadir'), 'projektor')

add_global_arguments('-DDATADIR="@0@"'.format(projektordatadir), language: 'c')

lite_dep = dependency('lite')

enable_djvu    = get_option('djvu')
enable_mupdf   = get_option('mupdf')
enable_poppler = get_option('poppler')

djvu_dep = []
if enable_mupdf
  djvu_dep = dependency('ddjvuapi', required: false)

  if not djvu_dep.found()
    warning('DjVu document renderer will not be built.')
    enable_djvu = false
  endif
endif

mupdf_dep = []
if enable_mupdf
  mupdf_dep = dependency('mupdf', required: false)

  if not mupdf_dep.found()
    warning('MuPDF document renderer will not be built.')
    enable_mupdf = false
  endif
endif

poppler_dep = []
if enable_poppler
  poppler_dep = dependency('poppler-glib', required: false)

  if not poppler_dep.found()
    warning('Poppler document renderer will not be built.')
    enable_poppler = false
  endif
endif

if not enable_mupdf and not enable_poppler
  error('No document renderer found.')
endif

subdir('data')
subdir('src')
