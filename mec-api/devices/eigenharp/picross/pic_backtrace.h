/*
 Copyright 2009 Eigenlabs Ltd.  http://www.eigenlabs.com

 This file is part of EigenD.

 EigenD is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EigenD is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with EigenD.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __PICROSS_PIC_BACKTRACE__
#define __PICROSS_PIC_BACKTRACE__

#include "pic_exports.h"

#ifdef __cplusplus
extern "C" {
#endif

PIC_DECLSPEC_FUNC(void) pic_backtrace(char *buffer, unsigned size);
PIC_DECLSPEC_FUNC(void) pic_breakpoint(void);

#ifdef __cplusplus
}
#endif

#endif
