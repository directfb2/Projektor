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

djvu_source = []
if enable_djvu
  djvu_source = 'djvu.c'
endif

mupdf_source = []
if enable_mupdf
  mupdf_source = 'mupdf.c'
endif

poppler_source = []
if enable_poppler
  poppler_source = 'poppler.c'
endif

executable('projektor',
           'projektor.c', djvu_source, mupdf_source, poppler_source,
           dependencies: [lite_dep, djvu_dep, mupdf_dep, poppler_dep],
           install: true)
