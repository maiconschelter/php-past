/*
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997,1998 PHP Development Team (See Credits file)      |
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
   |          Stig S�ther Bakken <ssb@guardian.no>                        |
   +----------------------------------------------------------------------+
 */

/* $Id: apache.c,v 1.32 1998/05/23 13:07:03 zeev Exp $ */
#ifdef THREAD_SAFE
#include "tls.h"
#endif
#include "php.h"
#include "internal_functions.h"
#include "functions/head.h"

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <ctype.h>

#if APACHE
#include "http_request.h"

void php3_virtual(INTERNAL_FUNCTION_PARAMETERS);
void php3_getallheaders(INTERNAL_FUNCTION_PARAMETERS);
void php3_apachelog(INTERNAL_FUNCTION_PARAMETERS);

function_entry apache_functions[] = {
	{"virtual",			php3_virtual,		NULL},
	{"getallheaders",		php3_getallheaders,	NULL},
	{NULL, NULL, NULL}
};

php3_module_entry apache_module_entry = {
	"Apache", apache_functions, NULL, NULL, NULL, NULL, NULL, STANDARD_MODULE_PROPERTIES
};

/* This function is equivilent to <!--#include virtual...-->
 * in mod_include. It does an Apache sub-request. It is useful
 * for including CGI scripts or .shtml files, or anything else
 * that you'd parse through Apache (for .phtml files, you'd probably
 * want to use <?Include>. This only works when PHP is compiled
 * as an Apache module, since it uses the Apache API for doing
 * sub requests.
 */

void php3_virtual(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *filename;
	request_rec *rr = NULL;
TLS_VARS;

	if (ARG_COUNT(ht) != 1 || getParameters(ht,1,&filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(filename);
	
	if (!(rr = sub_req_lookup_uri (filename->value.str.val, GLOBAL(php3_rqst)))) {
		php3_error(E_WARNING, "Unable to include '%s' - URI lookup failed", filename->value.str.val);
		if (rr) destroy_sub_req (rr);
		RETURN_FALSE;
	}

	if (rr->status != 200) {
		php3_error(E_WARNING, "Unable to include '%s' - error finding URI", filename->value.str.val);
		if (rr) destroy_sub_req (rr);
		RETURN_FALSE;
	}

	/* Cannot include another PHP file because of global conflicts */
	if (rr->content_type &&
		!strcmp(rr->content_type, PHP3_MIME_TYPE)) {
		php3_error(E_WARNING, "Cannot include a PHP file "
			  "(use <code>&lt;?include \"%s\"&gt;</code> instead)", filename->value.str.val);
		if (rr) destroy_sub_req (rr);
		RETURN_FALSE;
	}

	if (!php3_header()) {
		RETURN_FALSE;
	}
	if (run_sub_req(rr)) {
		php3_error(E_WARNING, "Unable to include '%s' - request execution failed", filename->value.str.val);
		if (rr) destroy_sub_req (rr);
		RETURN_FALSE;
	} else {
		if (rr) destroy_sub_req (rr);
		RETURN_TRUE;
	}
}


void php3_getallheaders(INTERNAL_FUNCTION_PARAMETERS)
{
    array_header *env_arr;
    table_entry *tenv;
    int i;
	
    if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
    }
    env_arr = table_elts(php3_rqst->headers_in);
    tenv = (table_entry *)env_arr->elts;
    for (i = 0; i < env_arr->nelts; ++i) {
		if (!tenv[i].key ||
			(php3_ini.safe_mode &&
			 !strncasecmp(tenv[i].key, "authorization", 13))) {
			continue;
		}
		if (add_assoc_string(return_value, tenv[i].key,tenv[i].val, 1)==FAILURE) {
			RETURN_FALSE;
		}
    }
}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
