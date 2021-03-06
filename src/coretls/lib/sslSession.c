/*
 * Copyright (c) 2000-2001,2005-2008,2010-2012 Apple Inc. All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */


#include "tls_handshake_priv.h"
#include "sslSession.h"
#include "sslMemory.h"
#include "sslUtils.h"
#include "sslDebug.h"
#include "sslCipherSpecs.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>

typedef struct
{
    size_t                sessionIDLen;
    uint8_t               sessionID[32];
    tls_protocol_version  negProtocolVersion;    /* negotiated version */
    tls_protocol_version  reqProtocolVersion;    /* version requested by client */
    uint16_t              cipherSuite;
	uint16_t			  padding;          /* so remainder is word aligned */
    uint8_t               masterSecret[48];
    size_t                ticketLen;
    size_t                certCount;
    uint8_t               data[1];         /* Actually, variable length */
    /* data contain the session ticket (if any), followed by certCount certs */
} ResumableSession;

/* Special fixed sessionID when using a session ticket */
#define SESSION_TICKET_ID "SESSION-TICKET"

/*
 * Cook up a (private) resumable session blob, based on the
 * specified ctx, store it with ctx->peerID (client) or ctx->sessionID (server) as the key.
 */
int
SSLAddSessionData(const tls_handshake_t ctx)
{   int                 err;
	size_t              sessionDataLen;
	tls_buffer          sessionData;
	ResumableSession    *session;
	size_t              certCount;
	SSLCertificate      *cert;
	uint8_t             *certDest;

	/* If we don't have session ID or a ticket, we can't store a session */
	if (!ctx->sessionID.data && !ctx->sessionTicket.data)
		return errSSLSessionNotFound;


	sessionDataLen = offsetof(ResumableSession, data) + ctx->sessionTicket.length;
	cert = ctx->peerCert;
	certCount = 0;
	while (cert)
	{   ++certCount;
		sessionDataLen += 4 + cert->derCert.length;
		cert = cert->next;
	}

    if ((err = SSLAllocBuffer(&sessionData, sessionDataLen)))
        return err;
    
    session = (ResumableSession*)sessionData.data;

    if(ctx->sessionID.data==NULL) {
        session->sessionIDLen = strlen(SESSION_TICKET_ID);
        memcpy(session->sessionID, SESSION_TICKET_ID, session->sessionIDLen);
    } else {
        session->sessionIDLen = ctx->sessionID.length;
        memcpy(session->sessionID, ctx->sessionID.data, session->sessionIDLen);
    }
    session->negProtocolVersion = ctx->negProtocolVersion;
    session->reqProtocolVersion = ctx->clientReqProtocol;
    session->cipherSuite = ctx->selectedCipher;
    memcpy(session->masterSecret, ctx->masterSecret, 48);
    session->certCount = certCount;
    session->padding = 0;
    session->ticketLen = ctx->sessionTicket.length;

    memcpy(session->data, ctx->sessionTicket.data,ctx->sessionTicket.length);

    certDest = session->data + session->ticketLen;

	cert = ctx->peerCert;
	while (cert)
	{   certDest = SSLEncodeInt(certDest, cert->derCert.length, 4);
		memcpy(certDest, cert->derCert.data, cert->derCert.length);
		certDest += cert->derCert.length;
		cert = cert->next;
	}

    if(ctx->isServer)
        err = ctx->callbacks->save_session_data(ctx->callback_ctx, ctx->sessionID, sessionData);
    else
        err = ctx->callbacks->save_session_data(ctx->callback_ctx, ctx->peerID, sessionData);

    SSLFreeBuffer(&sessionData);

	return err;
}

int
SSLDeleteSessionData(const tls_handshake_t ctx)
{   int      err;

    if (ctx->sessionID.data == 0)
        return errSSLSessionNotFound;

    err = ctx->callbacks->delete_session_data(ctx->callback_ctx, ctx->sessionID);
    return err;
}


/*
 * Given a sessionData blob, obtain the associated sessionTicket (if Any).
 */
int
SSLRetrieveSessionTicket(
                     const tls_buffer sessionData,
                     tls_buffer *ticket)
{
    ResumableSession    *session;

    session = (ResumableSession*) sessionData.data;
    ticket->data = session->data;
    ticket->length = session->ticketLen;
    return errSSLSuccess;
}

/*
 * Given a sessionData blob, obtain the associated sessionID (NOT the key...).
 */
int
SSLRetrieveSessionID(
		const tls_buffer sessionData,
		tls_buffer *identifier)
{
    ResumableSession    *session;

    session = (ResumableSession*) sessionData.data;
    identifier->data = session->sessionID;
    identifier->length = session->sessionIDLen;
    return errSSLSuccess;
}

/*
 * Validate session data on the server side, after receiving ClientHello.
 * If this fails, the server will attempt a full handshake.
 */
int SSLServerValidateSessionData(const tls_buffer sessionData, tls_handshake_t ctx)
{
    ResumableSession *session = (ResumableSession *)sessionData.data;

    /*
       Currently, the session cache is looked up by sessionID on the server side,
       so these two checks are mostly redundant and act as sanity check, in case
       the session cache callbacks are badly implemented.
       It would also be possible to lookup by more than sessionID, eg: you could
       lookup by sessionID + protocol version, to handle session resumption
       with in various version fallback cases.
     */

    require(session->sessionIDLen == ctx->proposedSessionID.length, out);
    require(memcmp(session->sessionID, ctx->proposedSessionID.data, ctx->proposedSessionID.length) == 0, out);

    /*
        If for some reason a session was cached with a different protocol version,
        then the server will fallback to a full handshake. We could accept to resume the session with
        the cached version, but we prefer to negotiate the best possible version instead.
     */
    require(session->negProtocolVersion == ctx->negProtocolVersion, out);

    /*
        We also check that the session cipherSuite is in the list of enabled ciphersuites,
        and also in the list of ciphersuites requested by the client.
     */

    require(cipherSuiteInSet(session->cipherSuite, ctx->enabledCipherSuites, ctx->numEnabledCipherSuites), out);
    require(cipherSuiteInSet(session->cipherSuite, ctx->requestedCipherSuites, ctx->numRequestedCipherSuites), out);

    ctx->selectedCipher = session->cipherSuite;
    InitCipherSpecParams(ctx);

    return 0;

out:
    return errSSLSessionNotFound;
}

/*
 * Validate session data on the client side, before sending the ClientHello.
 * If this fails, the client will no attempt resumption.
 */
int SSLClientValidateSessionDataBefore(const tls_buffer sessionData, tls_handshake_t ctx)
{
    ResumableSession *session = (ResumableSession *)sessionData.data;

    /*
        If the current requested version is higher than the session one,
        we do not re-use this session (see: rdar://23329369)
     */
    require(ctx->maxProtocolVersion <= session->reqProtocolVersion, out);

    /*
        Make sure that the session version is within our enabled versions.
     */
    require(session->negProtocolVersion <= ctx->maxProtocolVersion, out);
    require(session->negProtocolVersion >= ctx->minProtocolVersion, out);

    /*
        Make sure that the session ciphersuite is within our enabled ciphers
     */
    require(cipherSuiteInSet(session->cipherSuite, ctx->enabledCipherSuites, ctx->numEnabledCipherSuites), out);

    return 0;
out:
    return errSSLSessionNotFound;
}

/*
 * Validate session data on the client side, after receiving the ServerHello.
 * If this fails, the client will abort the connection.
 */
int SSLClientValidateSessionDataAfter(const tls_buffer sessionData, tls_handshake_t ctx)
{
    ResumableSession *session = (ResumableSession *)sessionData.data;

    /* Make sure that the session version and server version match. */
    require(session->negProtocolVersion == ctx->negProtocolVersion, out);
    /* Make sure that the session ciphersuite and server ciphersuite match. */
    require(session->cipherSuite == ctx->selectedCipher, out);

    return 0;

out:
    return errSSLProtocol;
}


/*
 * Retrieve session state from specified sessionData blob, install into
 * ctx. Presumably, ctx->sessionID and
 * ctx->negProtocolVersion are already init'd (from the above two functions).
 */
int
SSLInstallSessionFromData(const tls_buffer sessionData, tls_handshake_t ctx)
{   int            err;
    ResumableSession    *session;
    uint8_t             *storedCertProgress;
    SSLCertificate		*cert;
    SSLCertificate      *lastCert = NULL;
    size_t              certCount;
    size_t              certLen;

    session = (ResumableSession*)sessionData.data;

    /*
     * selectedCipher and negProtocolVersion should already be validated.
     */
    assert(ctx->selectedCipher == session->cipherSuite);
    assert(ctx->negProtocolVersion == session->negProtocolVersion);

    memcpy(ctx->masterSecret, session->masterSecret, 48);

    storedCertProgress = session->data + session->ticketLen;
    certCount = session->certCount;

    while (certCount--)
    {
		cert = (SSLCertificate *)sslMalloc(sizeof(SSLCertificate));
		if(cert == NULL) {
			return errSSLAllocate;
		}
        cert->next = 0;
        certLen = SSLDecodeInt(storedCertProgress, 4);
        storedCertProgress += 4;
        if ((err = SSLAllocBuffer(&cert->derCert, certLen)))
        {   
			sslFree(cert);
            return err;
        }
        memcpy(cert->derCert.data, storedCertProgress, certLen);
        storedCertProgress += certLen;
        if (lastCert == 0) {
            SSLFreeCertificates(ctx->peerCert);
            ctx->peerCert = cert;
        }
        else
            lastCert->next = cert;
        lastCert = cert;
    }

    return errSSLSuccess;
}
