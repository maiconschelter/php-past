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
   | Authors: Jaakko Hyv�tti <jaakko.hyvatti@iki.fi>                      |
   |                                                                      |
   +----------------------------------------------------------------------+
 */

/* $Id: php3_sprintf.c,v 1.5 1999/01/01 17:58:52 zeev Exp $ */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>

#if BROKEN_SPRINTF

int
_php3_sprintf (char*s, const char* format, ...)
{
  va_list args;
  char *ret;

  va_start (args, format);
  s[0] = '\0';
  ret = vsprintf (s, format, args);
  va_end (args);
  if (!ret)
    return -1;
  return strlen (s);
}

#endif /* BROKEN_SPRINTF */
