/*
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-1999 PHP Development Team (See Credits file)      |
   +----------------------------------------------------------------------+
   | This program is free software; you can redistribute it and/or modify |
   | it under the terms of one of the following licenses:                 |
   |                                                                      |
   |  A) the GNU General Public License as published by the Free Software |
   |     Foundation; either version 2 of the License, or (at your option) |
   |     any later version.                                               |
   |                                                                      |
   |  B) the PHP License as published by the PHP Development Team and     |
   |     included in the distribution in the file: LICENSE                |
   |                                                                      |
   | This program is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |
   | GNU General Public License for more details.                         |
   |                                                                      |
   | You should have received a copy of both licenses referred to here.   |
   | If you did not, or have any questions about PHP licensing, please    |
   | contact core@php.net.                                                |
   +----------------------------------------------------------------------+
   | Authors:                                                             |
   |                                                                      |
   +----------------------------------------------------------------------+
 */

/* $Id: php3_link.h,v 1.6 1999/01/01 17:59:15 zeev Exp $ */
#ifndef _PHP3_LINK_H
#define _PHP3_LINK_H

extern void php3_link(INTERNAL_FUNCTION_PARAMETERS);
extern void php3_unlink(INTERNAL_FUNCTION_PARAMETERS);
extern void php3_readlink(INTERNAL_FUNCTION_PARAMETERS);
extern void php3_linkinfo(INTERNAL_FUNCTION_PARAMETERS);
extern void php3_symlink(INTERNAL_FUNCTION_PARAMETERS);

#endif /* _PHP3_LINK_H */
