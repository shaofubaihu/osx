/*
 * Copyright (c) 2012-2014 Apple Inc. All Rights Reserved.
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


/*!
 @header SOSEngine.h - Manifest managent engine and decision making for
 object syncing protocol.
 */

#ifndef _SEC_SOSENGINE_H_
#define _SEC_SOSENGINE_H_

#include <Security/SecureObjectSync/SOSDataSource.h>
#include <Security/SecureObjectSync/SOSMessage.h>
#include <Security/SecureObjectSync/SOSPeer.h>
#include <dispatch/dispatch.h>

__BEGIN_DECLS

// TODO: Move this to SOSPeer.h?
typedef void (^SOSEnginePeerMessageSentBlock)(bool success);

// Return a new engine instance for a given data source.
SOSEngineRef SOSEngineCreate(SOSDataSourceRef dataSource, CFErrorRef *error);

// TODO: Nuke from orbit
SOSManifestRef SOSEngineCopyManifest(SOSEngineRef engine, CFErrorRef *error);

// Return a snapshot of the current manifest of the engines data source for the views that the given peer is in.
SOSManifestRef SOSEngineCopyLocalPeerManifest(SOSEngineRef engine, SOSPeerRef peer, CFErrorRef *error);
SOSManifestRef SOSEngineCopyLocalPeerManifest_locked(SOSEngineRef engine, SOSPeerRef peer, CFErrorRef *error);

// Apply changes to all views manifests, and update all peers accordingly
bool SOSEngineUpdateChanges(SOSEngineRef engine, SOSDataSourceTransactionSource source, CFArrayRef changes, CFErrorRef *error);

// Store manifest indexed by it's own digest.  Can be retrieved with SOSEngineGetManifestForDigest()
void SOSEngineAddManifest(SOSEngineRef engine, SOSManifestRef manifest);

// Retrive a digest stored with SOSEngineAddManifest()
SOSManifestRef SOSEngineGetManifestForDigest(SOSEngineRef engine, CFDataRef digest);

// Return the digest for a patched manifest (which is stored in the cache already).
CFDataRef SOSEnginePatchRecordAndCopyDigest(SOSEngineRef engine, SOSManifestRef base, SOSManifestRef removals, SOSManifestRef additions, CFErrorRef *error);

// Copy a manifest for a key persisted in a persisted dictionary
SOSManifestRef SOSEngineCopyPersistedManifest(SOSEngineRef engine, CFDictionaryRef persisted, CFStringRef key);

// Copy a manifest for a key persisted in a persisted dictionary
CFMutableArrayRef SOSEngineCopyPersistedManifestArray(SOSEngineRef engine, CFDictionaryRef persisted, CFStringRef key, CFErrorRef *error);

void SOSEngineClearCache(SOSEngineRef engine);

// Dispose of an engine when it's no longer needed.
void SOSEngineDispose(SOSEngineRef engine);

// Handle incoming message from a remote peer.
bool SOSEngineHandleMessage(SOSEngineRef engine, CFStringRef peerID,
                            CFDataRef message, CFErrorRef *error);

// Change the set of peers we know about.  trustedPeers and untrustedPeers are arrays of SOSPeerMetaRef
// trustedPeers is an array of SOSPeerMetaRef (peer SOSPeer.h), untrustedpeers is redundant as the engine
// treats a trustedPeer with no views and no publicKey the same as an untrustedPeer.
// TODO: Fix the documentation above this line.
void SOSEngineCircleChanged(SOSEngineRef engine, CFStringRef myPeerID, CFArrayRef trustedPeers, CFArrayRef untrustedPeers);

// Iterate over all peers.
void SOSEngineForEachPeer(SOSEngineRef engine, void (^with)(SOSPeerRef peer));

// TODO: Move SOSTransportMessageIDSRef declarations somewhere we can get to them here.
//void SOSEngineSyncWithPeers(SOSEngineRef engine, SOSTransportMessageIDSRef ids, SOSTransportMessageKVSRef kvs, CFErrorRef *error);
bool SOSEngineSyncWithPeers(SOSEngineRef engine, CFTypeRef ids, CFTypeRef kvs, CFErrorRef *error);

// Don't call this unless you know what you are doing.  If you do then still don't call it.
bool SOSEngineHandleMessage_locked(SOSEngineRef engine, CFStringRef peerID, SOSMessageRef message,
                                   SOSTransactionRef txn, bool *commit, bool *somethingChanged, CFErrorRef *error);

CFDataRef SOSEngineCreateMessage_locked(SOSEngineRef engine, SOSPeerRef peer,
                                        CFErrorRef *error, SOSEnginePeerMessageSentBlock *sent);

// Return a SOSPeerRef for a given peer_id.
SOSPeerRef SOSEngineCopyPeerWithID(SOSEngineRef engine, CFStringRef peer_id, CFErrorRef *error);

// Operate on a peer with a given peer_id under the engine lock
bool SOSEngineForPeerID(SOSEngineRef engine, CFStringRef peer_id, CFErrorRef *error, void (^forPeer)(SOSPeerRef peer));

// Modify a peer inside a transaction under then engine lock and optionally force an engine state save when done.
bool SOSEngineWithPeerID(SOSEngineRef engine, CFStringRef peer_id, CFErrorRef *error, void (^with)(SOSPeerRef peer, SOSDataSourceRef dataSource, SOSTransactionRef txn, bool *forceSaveState));

// Return a message to be sent for the current state.  Returns NULL on errors,
// return a zero length CFDataRef if there is nothing to send.
// If *ProposedManifest is set the caller is responsible for updating their
// proposed manifest upon successful transmission of the message.
CFDataRef SOSEngineCreateMessageToSyncToPeer(SOSEngineRef engine, CFStringRef peerID, SOSEnginePeerMessageSentBlock *sentBlock, CFErrorRef *error);

CFStringRef SOSEngineGetMyID(SOSEngineRef engine);
bool SOSEnginePeerDidConnect(SOSEngineRef engine, CFStringRef peerID, CFErrorRef *error);
bool SOSEngineSetPeerConfirmedManifest(SOSEngineRef engine, CFStringRef backupName,
                                       CFDataRef keybagDigest, CFDataRef manifestData, CFErrorRef *error);
CFArrayRef SOSEngineCopyBackupPeerNames(SOSEngineRef engine, CFErrorRef *error);

void logRawMessage(CFDataRef message, bool sending, uint64_t seqno);

// TODO: TEMPORARY: Get the list of IDs for cleanup, this shouldn't be used instead transport should iterate KVS.
CFArrayRef SOSEngineGetPeerIDs(SOSEngineRef engine);

CFArrayRef SOSEngineCopyPeerConfirmedDigests(SOSEngineRef engine, CFErrorRef *error);

// Private do not use!
SOSDataSourceRef SOSEngineGetDataSource(SOSEngineRef engine);

// MARK: Sync completion notification registration

void SOSEngineSetSyncCompleteListener(SOSEngineRef engine, CFStringRef peerID, dispatch_block_t notify_block);
void SOSEngineSetSyncCompleteListenerQueue(SOSEngineRef engine, dispatch_queue_t notify_queue);


__END_DECLS

#endif /* !_SEC_SOSENGINE_H_ */
