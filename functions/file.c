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
   +----------------------------------------------------------------------+
 */
/* $Id: file.c,v 1.173 1998/05/22 12:54:44 zeev Exp $ */
#ifdef THREAD_SAFE
#include "tls.h"
#endif
#include "php.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if MSVC5
#include <windows.h>
#include <winsock.h>
#define O_RDONLY _O_RDONLY
#include "win32/param.h"
#include "win32/winutil.h"
#else
#include <sys/param.h>
#include <sys/socket.h>
/* #include <sys/uio.h> */
#endif
#include "head.h"
#include "internal_functions.h"
#include "safe_mode.h"
#include "php3_list.h"
#include "php3_string.h"
#include "file.h"
#if HAVE_PWD_H
#if MSVC5
#include "win32/pwd.h"
#else
#include <pwd.h>
#endif
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include "snprintf.h"
#include "fsock.h"
#include "fopen-wrappers.h"

#if MISSING_FCLOSE_DECL
extern int fclose();
#endif

static void _php3_closesocket(int *);

#ifndef THREAD_SAFE
static int fgetss_state = 0;
int le_fp,le_pp;
int wsa_fp; /*to handle reading and writing to windows sockets*/
static int pclose_ret;
#endif

function_entry php3_file_functions[] = {
	{"pclose",		php3_pclose,	NULL},
	{"popen",		php3_popen,		NULL},
	{"readfile",	php3_readfile,	NULL},
	{"rewind",		php3_rewind,	NULL},
	{"rmdir",		php3_rmdir,		NULL},
	{"umask",		php3_fileumask,	NULL},
	{"fclose",		php3_fclose,	NULL},
	{"feof",		php3_feof,		NULL},
	{"fgetc",		php3_fgetc,		NULL},
	{"fgets",		php3_fgets,		NULL},
	{"fgetss",		php3_fgetss,	NULL},
	{"fread",		php3_fread,		NULL},
	{"fopen",		php3_fopen,		NULL},
	{"fpassthru",	php3_fpassthru,	NULL},
	{"fseek",		php3_fseek,		NULL},
	{"ftell",		php3_ftell,		NULL},
	{"fwrite",		php3_fwrite,	NULL},
	{"fputs",		php3_fwrite,	NULL},
	{"mkdir",		php3_mkdir,		NULL},
	{"rename",		php3_rename,	NULL},
	{"copy",		php3_file_copy,	NULL},
	{"tempnam",		php3_tempnam,	NULL},
	{"file",		php3_file,		NULL},
	{"set_socket_blocking",	php3_set_socket_blocking,	NULL},
#if (0 && HAVE_SYS_TIME_H && HAVE_SETSOCKOPT && defined(SO_SNDTIMEO) && defined(SO_RCVTIMEO))
	{"set_socket_timeout",	php3_set_socket_timeout,	NULL},
#endif
	{NULL, NULL, NULL}
};

php3_module_entry php3_file_module_entry = {
	"PHP_file", php3_file_functions, php3_minit_file, NULL, NULL, NULL, NULL, STANDARD_MODULE_PROPERTIES
};

void php3_file(INTERNAL_FUNCTION_PARAMETERS) {
	pval *filename, *arg2;
	FILE *fp;
	char *slashed, buf[8192];
	register int i=0;
	int use_include_path = 0;

	int issock=0, socketd=0;
	TLS_VARS;
	
	/* check args */
	switch (ARG_COUNT(ht)) {
	case 1:
		if (getParameters(ht,1,&filename) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (getParameters(ht,2,&filename,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long(arg2);
		use_include_path = arg2->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string(filename);

	fp = php3_fopen_wrapper(filename->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd);
	if (!fp && !socketd) {
		if (issock != BAD_URL) {
			php3_strip_url_passwd(filename->value.str.val);
			php3_error(E_WARNING,"File(\"%s\") - %s",filename->value.str.val,strerror(errno));
		}
		RETURN_FALSE;
	}

	/* Initialize return array */
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}	

	/* Now loop through the file and do the magic quotes thing if needed */
	memset(buf,0,8191);
	while(issock?SOCK_FGETS(buf,8191,socketd):(int)fgets(buf,8191,fp)) {
		if (php3_ini.magic_quotes_runtime) {
			int len;
			
			slashed = _php3_addslashes(buf,0,&len,0); /* 0 = don't free source string */
            add_index_stringl(return_value, i++, slashed, len, 0);
		} else {
			add_index_string(return_value, i++, buf, 1);
		}
	}
	if (issock) {
#if WIN32|WINNT
		closesocket(socketd);
#else
		close(socketd);
#endif
	} else {
		fclose(fp);
	}
}


static void __pclose(FILE *pipe)
{
TLS_VARS;
	GLOBAL(pclose_ret) = pclose(pipe);
}

static void _php3_closesocket(int *sock) {
	int socketd=*sock;
	if (socketd){
#if WIN32|WINNT
		closesocket(socketd);
#else
		close(socketd);
#endif
		efree(sock);
	}
}


int php3_minit_file(INIT_FUNC_ARGS)
{
	TLS_VARS;
	
	GLOBAL(le_fp) = register_list_destructors(fclose,NULL);
	GLOBAL(le_pp) = register_list_destructors(__pclose,NULL);
	GLOBAL(wsa_fp) = register_list_destructors(_php3_closesocket,NULL);
	return SUCCESS;
}


void php3_tempnam(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1, *arg2;
	char *d;
	char *t;
	char p[64];
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);
	convert_to_string(arg2);
	d = estrndup(arg1->value.str.val,arg1->value.str.len);
	strncpy(p,arg2->value.str.val,sizeof(p));

	t = tempnam(d,p);
	efree(d);
	RETURN_STRING(t,1);
}

void php3_fopen(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1, *arg2, *arg3;
	FILE *fp;
	char *p;
	int *sock;
	int id;
	int use_include_path = 0;
	int issock=0, socketd=0;
	TLS_VARS;
	
	switch(ARG_COUNT(ht)) {
	case 2:
		if (getParameters(ht,2,&arg1,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 3:
		if (getParameters(ht,3,&arg1,&arg2,&arg3) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long(arg3);
		use_include_path = arg3->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);
	convert_to_string(arg2);
	p = estrndup(arg2->value.str.val,arg2->value.str.len);

	/*
	 * We need a better way of returning error messages from
	 * php3_fopen__wrapper().
	 */
	fp = php3_fopen_wrapper(arg1->value.str.val, p, use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd);
	if (!fp && !socketd) {
		if (issock != BAD_URL) {
			php3_strip_url_passwd(arg1->value.str.val);
			php3_error(E_WARNING,"fopen(\"%s\",\"%s\") - %s",
						arg1->value.str.val, p, strerror(errno));
		}
		efree(p);
		RETURN_FALSE;
	}
	GLOBAL(fgetss_state)=0;
	if (issock) {
		sock=emalloc(sizeof(int));
		*sock=socketd;
		id = php3_list_insert(sock,GLOBAL(wsa_fp));
	} else {
		id = php3_list_insert(fp,GLOBAL(le_fp));
	}
	efree(p);
	RETURN_LONG(id);
}	

void php3_fclose(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	int id, type;
	FILE *fp;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id=arg1->value.lval;
	fp = php3_list_find(id,&type);
	if (!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	php3_list_delete(id);
	RETURN_TRUE;
}

void php3_popen(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1, *arg2;
	FILE *fp;
	int id;
	char *p;
	char *b, buf[1024];
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);
	convert_to_string(arg2);
	p = estrndup(arg2->value.str.val,arg2->value.str.len);
	if (php3_ini.safe_mode){
	b = strchr(arg1->value.str.val,' ');
	if (!b) {
		b = strrchr(arg1->value.str.val,'/');
	} else {
		char *c;
		c = arg1->value.str.val;
		while((*b!='/')&&(b!=c)) b--;
		if (b==c) b=NULL;
	}
	if (b) {
		snprintf(buf,sizeof(buf),"%s%s",php3_ini.safe_mode_exec_dir,b);
	} else {
		snprintf(buf,sizeof(buf),"%s/%s",php3_ini.safe_mode_exec_dir,arg1->value.str.val);
	}
	fp = popen(buf,p);
	if (!fp) {
		php3_error(E_WARNING,"popen(\"%s\",\"%s\") - %s",buf,p,strerror(errno));
		RETURN_FALSE;
	}
	} else {
	fp = popen(arg1->value.str.val,p);
	if (!fp) {
		php3_error(E_WARNING,"popen(\"%s\",\"%s\") - %s",arg1->value.str.val,p,strerror(errno));
		efree(p);
		RETURN_FALSE;
	}
	}
/* #endif */
	id = php3_list_insert(fp,GLOBAL(le_pp));
	efree(p);
	RETURN_LONG(id);
}

void php3_pclose(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	int id,type;
	FILE *fp;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id = arg1->value.lval;

	fp = php3_list_find(id,&type);
	if (!fp || type!=GLOBAL(le_pp)) {
		php3_error(E_WARNING,"Unable to find pipe identifier %d",id);
		RETURN_FALSE;
	}
	php3_list_delete(id);
	RETURN_LONG(GLOBAL(pclose_ret));
}

void php3_feof(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	FILE *fp;
	int id, type;
	int issock=0;
	int socketd=0, *sock;
	unsigned int temp;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id = arg1->value.lval;
	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd=*sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		/* we're at the eof if the file doesn't exist */
		RETURN_TRUE;
	}
	if ((issock?!(recv(socketd,(char *)&temp,1,MSG_PEEK)):feof(fp))) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

void php3_set_socket_blocking(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *arg1, *arg2;
	int id, type, block;
	int flags;
	int socketd=0, *sock;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	convert_to_long(arg2);
	id = arg1->value.lval;
	block = arg2->value.lval;
	
	sock = php3_list_find(id,&type);
	if (type!=GLOBAL(wsa_fp)) {
		php3_error(E_WARNING,"%d is not a socket id",id);
		RETURN_FALSE;
	}
	socketd=*sock;
#if WIN32|WINNT
	/* with ioctlsocket, a non-zero sets nonblocking, a zero sets blocking */
	flags=block;
	if (ioctlsocket(socketd,FIONBIO,&flags)==SOCKET_ERROR){
		php3_error(E_WARNING,"%s",WSAGetLastError());
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
#else
	flags = fcntl(socketd, F_GETFL);
# ifdef O_NONBLOCK
	/* POSIX version */
	if (block) {
		if ((flags & O_NONBLOCK)) {
			flags ^= O_NONBLOCK;
		}
	} else {
		if (!(flags & O_NONBLOCK)) {
			flags |= O_NONBLOCK;
		}
	}
# else
#  ifdef O_NDELAY
	/* old non-POSIX version */
	if (block) {
		flags |= O_NDELAY;
	} else {
		flags ^= O_NDELAY;
	}
#  endif
# endif
	fcntl(socketd,F_SETFL,flags);
	/* FIXME: Shouldnt we return true on this function? */
#endif
}


#if (0 && HAVE_SYS_TIME_H && HAVE_SETSOCKOPT && defined(SO_SNDTIMEO) && defined(SO_RCVTIMEO))
/* this doesn't work, as it appears those properties are read-only :( */
void php3_set_socket_timeout(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *socket,*timeout;
	int type, *sock;
	struct timeval t;

	if (ARG_COUNT(ht)!=2 || getParameters(ht, 2, &socket, &timeout)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(socket);
	convert_to_long(timeout);
	
	sock = php3_list_find(socket->value.lval, &type);
	if (type!=GLOBAL(wsa_fp)) {
		php3_error(E_WARNING,"%d is not a socket id",socket->value.lval);
		RETURN_FALSE;
	}
	t.tv_sec = timeout->value.lval;
	t.tv_usec = 0;
	setsockopt(*sock,SOL_SOCKET,SO_SNDTIMEO,(void *) &t,sizeof(struct timeval));
	setsockopt(*sock,SOL_SOCKET,SO_RCVTIMEO,(void *) &t,sizeof(struct timeval));
	RETURN_TRUE;
}
#endif


void php3_fgets(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *arg1, *arg2;
	FILE *fp;
	int id, len, type;
	char *buf;
	int issock=0;
	int *sock, socketd=0;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	convert_to_long(arg2);
	id = arg1->value.lval;
	len = arg2->value.lval;

	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd=*sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	buf = emalloc(sizeof(char) * (len + 1));
	/* needed because recv doesnt put a null at the end*/
	memset(buf,0,len+1);
	if (!(issock?SOCK_FGETS(buf,len,socketd):(int)fgets(buf,len,fp))) {
		efree(buf);
		RETVAL_FALSE;
	} else {
		if (php3_ini.magic_quotes_runtime) {
			return_value->value.str.val = _php3_addslashes(buf,0,&return_value->value.str.len,1);
		} else {
			return_value->value.str.val = buf;
			return_value->value.str.len = strlen(return_value->value.str.val);
		}
		return_value->type = IS_STRING;
	}
	return;
}

void php3_fgetc(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	FILE *fp;
	int id, type;
	char *buf;
	int issock=0;
	int *sock, socketd=0;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id = arg1->value.lval;

	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd = *sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	buf = emalloc(sizeof(char) * 2);
	if (!(issock?(SOCK_FGETC(buf,socketd)):(*buf=fgetc(fp)))) {
		efree(buf);
		RETVAL_FALSE;
	} else {
		buf[1]='\0';
		return_value->value.str.val = buf; 
		return_value->value.str.len = 1; 
		return_value->type = IS_STRING;
	}
	return;
}

/* Strip any HTML tags while reading */
void php3_fgetss(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *fd, *bytes;
	FILE *fp;
	int id, len, br, type;
	char *buf, *p, *rbuf, *rp, c, lc;
	int issock=0;
	int *sock,socketd=0;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &fd, &bytes) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long(fd);
	convert_to_long(bytes);

	id = fd->value.lval;
	len = bytes->value.lval;

	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd=*sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING, "Unable to find file identifier %d", id);
		RETURN_FALSE;
	}

	buf = emalloc(sizeof(char) * (len + 1));
	/*needed because recv doesnt set null char at end*/
	memset(buf,0,len+1);
	if (!(issock?SOCK_FGETS(buf,len,socketd):(int)fgets(buf, len, fp))) {
		efree(buf);
		RETURN_FALSE;
	}

	rbuf = estrdup(buf);
	c = *buf;
	lc = '\0';
	p = buf;
	rp = rbuf;
	br = 0;

	while (c) {
		switch (c) {
			case '<':
				if (GLOBAL(fgetss_state) == 0) {
					lc = '<';
					GLOBAL(fgetss_state) = 1;
				}
				break;

			case '(':
				if (GLOBAL(fgetss_state) == 2) {
					if (lc != '\"') {
						lc = '(';
						br++;
					}
				} else if (GLOBAL(fgetss_state) == 0) {
					*(rp++) = c;
				}
				break;	

			case ')':
				if (GLOBAL(fgetss_state) == 2) {
					if (lc != '\"') {
						lc = ')';
						br--;
					}
				} else if (GLOBAL(fgetss_state) == 0) {
					*(rp++) = c;
				}
				break;	

			case '>':
				if (GLOBAL(fgetss_state) == 1) {
					lc = '>';
					GLOBAL(fgetss_state) = 0;
				} else if (GLOBAL(fgetss_state) == 2) {
					if (!br && lc != '\"') {
						GLOBAL(fgetss_state) = 0;
					}
				}
				break;

			case '\"':
				if (GLOBAL(fgetss_state) == 2) {
					if (lc == '\"') {
						lc = '\0';
					} else if (lc != '\\') {
						lc = '\"';
					}
				} else if (GLOBAL(fgetss_state) == 0) {
					*(rp++) = c;
				}
				break;

			case '?':
				if (GLOBAL(fgetss_state)==1) {
					br=0;
					GLOBAL(fgetss_state)=2;
					break;
				}
				/* fall-through */

			default:
				if (GLOBAL(fgetss_state) == 0) {
					*(rp++) = c;
				}	
		}
		c = *(++p);
	}	
	*rp = '\0';
	efree(buf);
	RETVAL_STRING(rbuf,1);
	efree(rbuf);
}


void php3_fwrite(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *arg1, *arg2, *arg3=NULL;
	FILE *fp;
	int ret,id,type;
	int num_bytes;
	int issock=0;
	int *sock, socketd=0;
	TLS_VARS;
	
	switch (ARG_COUNT(ht)) {
		case 2:
			if (getParameters(ht, 2, &arg1, &arg2)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string(arg2);
			num_bytes = arg2->value.str.len;
			break;
		case 3:
			if (getParameters(ht, 3, &arg1, &arg2, &arg3)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string(arg2);
			convert_to_long(arg3);
			num_bytes = MIN(arg3->value.lval, arg2->value.str.len);
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}				
	convert_to_long(arg1);
	id = arg1->value.lval;	

	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd=*sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	/* strip slashes only if the length wasn't specified explicitly */
	if (!arg3 && php3_ini.magic_quotes_runtime) {
		_php3_stripslashes(arg2->value.str.val,&num_bytes);
	}

	if (issock){
		ret = SOCK_WRITEL(arg2->value.str.val,num_bytes,socketd);
	} else {
		ret = fwrite(arg2->value.str.val,1,num_bytes,fp);
	}
	RETURN_LONG(ret);
}	


void php3_rewind(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	int id,type;
	FILE *fp;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id = arg1->value.lval;	
	fp = php3_list_find(id,&type);
	if (!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	rewind(fp);
	RETURN_TRUE;
}

void php3_ftell(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	int id, type;
	long pos;
	FILE *fp;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id = arg1->value.lval;	
	fp = php3_list_find(id,&type);
	if (!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	pos = ftell(fp);
	RETURN_LONG(pos);
}

void php3_fseek(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1, *arg2;
	int ret,id,type;
	long pos;
	FILE *fp;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	convert_to_long(arg2);
	pos = arg2->value.lval;
	id = arg1->value.lval;
	fp = php3_list_find(id,&type);
	if (!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
/*fseek is flaky on windows, use setfilepointer, but we have to live with
	it until we use win32 api for all the file functions in 3.1 */
#if 0
	ret = SetFilePointer (fp, pos, NULL, FILE_BEGIN);
	if (ret == 0xFFFFFFFF){
		php3_error(E_WARNING,"Unable to move file postition: %s",php3_win_err());
		RETURN_FALSE;
	}
#else
 	ret = fseek(fp,pos,SEEK_SET);
#endif
	RETURN_LONG(ret);
}

void php3_mkdir(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1, *arg2;
	int ret,mode;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);
	convert_to_long(arg2);
	mode = arg2->value.lval;
	if (php3_ini.safe_mode &&(!_php3_checkuid(arg1->value.str.val,3))) {
		php3_error(E_WARNING,"SAFE MODE Restriction in effect.  Invalid owner of parent directory.");
		RETURN_FALSE;
	}
	ret = mkdir(arg1->value.str.val,mode);
	if (ret < 0) {
		php3_error(E_WARNING,"MkDir failed (%s)", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}	

void php3_rmdir(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	int ret;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);
	if (php3_ini.safe_mode &&(!_php3_checkuid(arg1->value.str.val,1))) {
		php3_error(E_WARNING,"SAFE MODE Restriction in effect.  Invalid owner of directory to be removed.");
		RETURN_FALSE;
	}
	ret = rmdir(arg1->value.str.val);
	if (ret < 0) {
		php3_error(E_WARNING,"RmDir failed (%s)", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}	

/*
 * Read a file and write the ouput to stdout
 */
void php3_readfile(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1, *arg2;
	char buf[8192];
	FILE *fp;
	int b, size;
	int use_include_path = 0;

	int issock=0, socketd=0;
	TLS_VARS;
	
	/* check args */
	switch (ARG_COUNT(ht)) {
	case 1:
		if (getParameters(ht,1,&arg1) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (getParameters(ht,2,&arg1,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long(arg2);
		use_include_path = arg2->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);	

	/*
	 * We need a better way of returning error messages from
	 * php3_fopen_wrapper().
	 */
	fp = php3_fopen_wrapper(arg1->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd);
	if (!fp && !socketd){
		if (issock != BAD_URL) {
			php3_strip_url_passwd(arg1->value.str.val);
			php3_error(E_WARNING,"ReadFile(\"%s\") - %s",arg1->value.str.val,strerror(errno));
		}
		RETURN_FALSE;
	}
	size= 0;
	if (php3_header()) {  /* force header if not already sent */
		while(issock?(b=SOCK_FGETS(buf,sizeof(buf),socketd)):(b = fread(buf, 1, sizeof(buf), fp)) > 0) {
			PHPWRITE(buf,b);
			size += b ;
		}
	}
	if (issock) {
#if WIN32|WINNT
		closesocket(socketd);
#else
		close(socketd);
#endif
	} else {
		fclose(fp);
	}
	RETURN_LONG(size);
}

/*
 * Return or change the umask.
 */
void php3_fileumask(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	int oldumask;
	int arg_count = ARG_COUNT(ht);
	TLS_VARS;
	
	oldumask = umask(077);

	if (arg_count == 0) {
		umask(oldumask);
	}
	else {
		if (arg_count > 1 || getParameters(ht, 1, &arg1) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long(arg1);
		umask(arg1->value.lval);
	}
	RETURN_LONG(oldumask);
}

/*
 * Read to EOF on a file descriptor and write the output to stdout.
 */
void php3_fpassthru(INTERNAL_FUNCTION_PARAMETERS) {
	pval *arg1;
	FILE *fp;
	char buf[8192];
	int id, size, b, type;
	int issock=0;
	int socketd=0, *sock;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	id = arg1->value.lval;
	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd=*sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	size = 0;
	if (php3_header()) { /* force headers if not already sent */
		while(issock?(b=SOCK_FGETS(buf,sizeof(buf),socketd)):(b = fread(buf, 1, sizeof(buf), fp)) > 0) {
			PHPWRITE(buf,b);
			size += b ;
		}
	}
/*
	if (issock) { 
#if WIN32|WINNT
		closesocket(socketd);
#else
		close(socketd);
#endif
	} else {
		fclose(fp);
	}
*/
	php3_list_delete(id);
	RETURN_LONG(size);
}


void php3_rename(INTERNAL_FUNCTION_PARAMETERS) {
	pval *OLD, *NEW;
	char *old, *new;
	int ret;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &OLD, &NEW) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string(OLD);
	convert_to_string(NEW);

	old = OLD->value.str.val;
	new = NEW->value.str.val;

	if (php3_ini.safe_mode &&(!_php3_checkuid(old,2))) {
		php3_error(E_WARNING,
					"SAFE MODE Restriction in effect.  "
					"Invalid owner of file to be renamed.");
		RETURN_FALSE;
	}
	ret = rename(old, new);

	if (ret == -1) {
		php3_error(E_WARNING,
					"Rename failed (%s)", strerror(errno));
		RETURN_FALSE;
	}

	RETVAL_TRUE;
}


void php3_file_copy(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *source, *target;
	char buffer[8192];
	int fd_s,fd_t,read_bytes;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &source, &target) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string(source);
	convert_to_string(target);

	if (php3_ini.safe_mode &&(!_php3_checkuid(source->value.str.val,2))) {
		php3_error(E_WARNING,
					"SAFE MODE Restriction in effect.  "
					"Invalid owner of file to be renamed.");
		RETURN_FALSE;
	}
	
#if WIN32|WINNT
	if ((fd_s=open(source->value.str.val,O_RDONLY|_O_BINARY))==-1) {
#else
	if ((fd_s=open(source->value.str.val,O_RDONLY))==-1) {
#endif
		php3_error(E_WARNING,"Unable to open '%s' for reading:  %s",source->value.str.val,strerror(errno));
		RETURN_FALSE;
	}
#if WIN32|WINNT
	if ((fd_t=open(target->value.str.val,_O_WRONLY|_O_CREAT|_O_TRUNC|_O_BINARY,_S_IREAD|_S_IWRITE))==-1){
#else
	if ((fd_t=creat(target->value.str.val,0777))==-1) {
#endif
		php3_error(E_WARNING,"Unable to create '%s':  %s", target->value.str.val,strerror(errno));
		close(fd_s);
		RETURN_FALSE;
	}

	while ((read_bytes=read(fd_s,buffer,8192))!=-1 && read_bytes!=0) {
		if (write(fd_t,buffer,read_bytes)==-1) {
			php3_error(E_WARNING,"Unable to write to '%s':  %s",target->value.str.val,strerror(errno));
			close(fd_s);
			close(fd_t);
			RETURN_FALSE;
		}
	}
	
	close(fd_s);
	close(fd_t);

	RETVAL_TRUE;
}


void php3_fread(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *arg1, *arg2;
	FILE *fp;
	int id, len, type;
	int issock=0;
	int *sock, socketd=0;
	TLS_VARS;
	
	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(arg1);
	convert_to_long(arg2);
	id = arg1->value.lval;
	len = arg2->value.lval;

	fp = php3_list_find(id,&type);
	if (type==GLOBAL(wsa_fp)){
		issock=1;
		sock = php3_list_find(id,&type);
		socketd=*sock;
	}
	if ((!fp || (type!=GLOBAL(le_fp) && type!=GLOBAL(le_pp))) && (!socketd || type!=GLOBAL(wsa_fp))) {
		php3_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}
	return_value->value.str.val = emalloc(sizeof(char) * (len + 1));
	/* needed because recv doesnt put a null at the end*/
	
	if (!issock) {
		return_value->value.str.len = fread(return_value->value.str.val, 1, len, fp);
		return_value->value.str.val[return_value->value.str.len] = 0;
	} else {
		return_value->value.str.len = _php3_sock_fread(return_value->value.str.val, len, socketd);
	}
	if (php3_ini.magic_quotes_runtime) {
		return_value->value.str.val = _php3_addslashes(return_value->value.str.val,return_value->value.str.len,&return_value->value.str.len,1);
	}
	return_value->type = IS_STRING;
}


/*
 * Local variables:
 * tab-width: 4
 * End:
 */
