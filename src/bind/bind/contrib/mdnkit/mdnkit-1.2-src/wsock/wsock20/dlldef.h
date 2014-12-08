/*
 * dlldef.h
 */

/*
 * Copyright (c) 2000 Japan Network Information Center.  All rights reserved.
 *  
 * By using this file, you agree to the terms and conditions set forth bellow.
 * 
 * 			LICENSE TERMS AND CONDITIONS 
 * 
 * The following License Terms and Conditions apply, unless a different
 * license is obtained from Japan Network Information Center ("JPNIC"),
 * a Japanese association, Fuundo Bldg., 1-2 Kanda Ogawamachi, Chiyoda-ku,
 * Tokyo, Japan.
 * 
 * 1. Use, Modification and Redistribution (including distribution of any
 *    modified or derived work) in source and/or binary forms is permitted
 *    under this License Terms and Conditions.
 * 
 * 2. Redistribution of source code must retain the copyright notices as they
 *    appear in each source code file, this License Terms and Conditions.
 * 
 * 3. Redistribution in binary form must reproduce the Copyright Notice,
 *    this License Terms and Conditions, in the documentation and/or other
 *    materials provided with the distribution.  For the purposes of binary
 *    distribution the "Copyright Notice" refers to the following language:
 *    "Copyright (c) Japan Network Information Center.  All rights reserved."
 * 
 * 4. Neither the name of JPNIC may be used to endorse or promote products
 *    derived from this Software without specific prior written approval of
 *    JPNIC.
 * 
 * 5. Disclaimer/Limitation of Liability: THIS SOFTWARE IS PROVIDED BY JPNIC
 *    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *    PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL JPNIC BE LIABLE
 *    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * 
 * 6. Indemnification by Licensee
 *    Any person or entities using and/or redistributing this Software under
 *    this License Terms and Conditions shall defend indemnify and hold
 *    harmless JPNIC from and against any and all judgements damages,
 *    expenses, settlement liabilities, cost and other liabilities of any
 *    kind as a result of use and redistribution of this Software or any
 *    claim, suite, action, litigation or proceeding by any third party
 *    arising out of or relates to this License Terms and Conditions.
 * 
 * 7. Governing Law, Jurisdiction and Venue
 *    This License Terms and Conditions shall be governed by and and
 *    construed in accordance with the law of Japan. Any person or entities
 *    using and/or redistributing this Software under this License Terms and
 *    Conditions hereby agrees and consent to the personal and exclusive
 *    jurisdiction and venue of Tokyo District Court of Japan.
 */

#ifndef _DLLDEF_H
#define _DLLDEF_H

#include "../common/jpnicmdn.h"

/*
 * Execution TRacing
 */
 
extern  int     procPid   ;
extern  UCHAR   procExe[] ;

#ifdef  DEBUG
#define TRACE   mdnPrintf("%04x ws20 : ",procPid) ; mdnPrintf
#define FATAL   mdnPrintf("%04x ws20 : ",procPid) ; mdnPrintf
#else
#define TRACE
#define FATAL   mdnPrintf("%04x ws20 : ",procPid) ; mdnPrintf
#endif

/*
 * Encoding Names and Options
 */

extern  UCHAR   paramLocal[]  ;
extern  UCHAR   paramServer[] ;
extern  UCHAR   paramNorm[]   ;
  
extern  BOOL    encodeHere    ;
extern  CONVPTR encodeCtx     ;

/*
 * entry points to wrap
 */

extern  int (WSAAPI *fpGethostname) (char FAR * name, int namelen) ;

extern  struct hostent FAR * (WSAAPI *fpGethostbyaddr) 
    (const char FAR * addr, int len, int type) ;
extern  struct hostent FAR * (WSAAPI *fpGethostbyname) 
    (const char FAR * name) ;

extern  HANDLE (WSAAPI *fpWSAAsyncGetHostByName)
    (HWND hWnd, u_int wMsg,
    const char FAR * name, char FAR * buf,int buflen) ;
extern  HANDLE (WSAAPI *fpWSAAsyncGetHostByAddr)
    (HWND hWnd, u_int wMsg,
    const char FAR * addr, int len, int type, char FAR * buf, int buflen) ;

extern  INT (WSAAPI *fpWSALookupServiceBeginA)
    (LPWSAQUERYSETA lpqsRestrictions,
     DWORD          dwControlFlags,
     LPHANDLE       lphLookup) ;
extern  INT (WSAAPI *fpWSALookupServiceBeginW)
    (LPWSAQUERYSETW lpqsRestrictions,
     DWORD          dwControlFlags,
     LPHANDLE       lphLookup) ;

extern  INT (WSAAPI *fpWSALookupServiceNextA)
    (HANDLE           hLookup,
     DWORD            dwControlFlags,
     LPDWORD          lpdwBufferLength,
     LPWSAQUERYSETA   lpqsResults) ;
extern  INT (WSAAPI *fpWSALookupServiceNextW)
    (HANDLE           hLookup,
     DWORD            dwControlFlags,
     LPDWORD          lpdwBufferLength,
     LPWSAQUERYSETW   lpqsResults) ;

extern  INT (WSAAPI *fpWSALookupServiceEnd) (HANDLE  hLookup) ;
     
/*
 * link above entries
 */
 
void    linkFree(void) ;
BOOL    linkLoad(void) ;

#endif  /* _DLLDEF_H */
