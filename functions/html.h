/* 
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2000 PHP Development Team (See Credits file)      |
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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   +----------------------------------------------------------------------+
 */

/* $Id: html.h,v 1.12 2000/09/12 17:23:30 rasmus Exp $ */

#ifndef _HTML_H
#define _HTML_H

#define ENT_COMPAT      1
#define ENT_QUOTES      2
#define ENT_NOQUOTES    4

extern void php3_htmlspecialchars(INTERNAL_FUNCTION_PARAMETERS);
extern void php3_htmlentities(INTERNAL_FUNCTION_PARAMETERS);
extern void php3_gamma_correct_tag(INTERNAL_FUNCTION_PARAMETERS);
PHPAPI char * _php3_htmlentities(char *s, int i, int all, int quote_style);

#endif /* _HTML_H */
