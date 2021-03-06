/****************************************************************************
 * Copyright (c) 2001 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Thomas E. Dickey 2001                                           *
 ****************************************************************************/

#include <curses.priv.h>

#include <ctype.h>

MODULE_ID("$Id: varargs.c,v 1.1.1.1 2001/11/29 20:40:58 jevans Exp $")

#ifdef TRACE

typedef enum {
    atUnknown = 0, atInteger, atFloat, atPoint, atString
} ARGTYPE;

#define VA_INT(type) ival = va_arg(ap, type)
#define VA_FLT(type) fval = va_arg(ap, type)
#define VA_PTR(type) pval = (void *)va_arg(ap, type)
#define VA_STR(type) sval = va_arg(ap, type)

/*
 * Returns a string that represents the parameter list of a printf-style call.
 */
NCURSES_EXPORT(char *)
_nc_varargs(const char *fmt, va_list ap)
{
    static char *result_buf;
    static size_t result_len;

    char buffer[BUFSIZ];

    if (fmt == 0 || *fmt == '\0')
	return "";
    if (result_len == 0)
	result_buf = typeMalloc(char, result_len = BUFSIZ);
    if (result_buf == 0)
	return "";
    *result_buf = '\0';

    while (*fmt != '\0') {
	if (*fmt == '%') {
	    char *pval = 0;	/* avoid const-cast */
	    const char *sval = "";
	    double fval = 0.0;
	    int done = FALSE;
	    int ival = 0;
	    int type = 0;
	    ARGTYPE used = atUnknown;

	    while (*++fmt != '\0' && !done) {

		if (*fmt == '*') {
		    VA_INT(int);
		    used = atInteger;
		    break;
		} else if (isalpha(UChar(*fmt))) {
		    done = TRUE;
		    switch (*fmt) {
		    case 'Z':	/* FALLTHRU */
		    case 'h':	/* FALLTHRU */
		    case 'l':	/* FALLTHRU */
			done = FALSE;
			type = *fmt;
			break;
		    case 'i':	/* FALLTHRU */
		    case 'd':	/* FALLTHRU */
		    case 'u':	/* FALLTHRU */
		    case 'x':	/* FALLTHRU */
		    case 'X':	/* FALLTHRU */
			if (type == 'l')
			    VA_INT(long);
			else if (type == 'Z')
			    VA_INT(size_t);
			else
			    VA_INT(int);
			used = atInteger;
			break;
		    case 'f':	/* FALLTHRU */
		    case 'e':	/* FALLTHRU */
		    case 'E':	/* FALLTHRU */
		    case 'g':	/* FALLTHRU */
		    case 'G':	/* FALLTHRU */
			VA_FLT(double);
			used = atFloat;
			break;
		    case 'c':
			VA_INT(int);
			used = atInteger;
			break;
		    case 's':
			VA_STR(const char *);
			used = atString;
			break;
		    case 'p':
			VA_PTR(void *);
			used = atPoint;
			break;
		    case 'n':
			VA_PTR(int *);
			used = atPoint;
			break;
		    default:
			break;
		    }
		} else if (*fmt == '%') {
		    done = TRUE;
		}
		if (used != atUnknown) {
		    const char *param = buffer;
		    switch (used) {
		    case atInteger:
			sprintf(buffer, "%d", ival);
			break;
		    case atFloat:
			sprintf(buffer, "%f", fval);
			break;
		    case atPoint:
			sprintf(buffer, "%p", pval);
			break;
		    case atString:
			param = _nc_visbuf2(1, sval);
			break;
		    default:
			strcpy(buffer, "?");
			break;
		    }
		    result_len += strlen(param) + 2;
		    result_buf = typeRealloc(char, result_len, result_buf);
		    sprintf(result_buf + strlen(result_buf), ",%s", param);
		    used = atUnknown;
		}
	    }
	} else {
	    fmt++;
	}
    }

    return (result_buf);
}
#else
empty_module(_nc_varargs)
#endif
