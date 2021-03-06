//
//  SOSAccountPriv.h
//  sec
//

#ifndef sec_SOSAccountPriv_h
#define sec_SOSAccountPriv_h

#include "SOSAccount.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRuntime.h>
#include <utilities/SecCFWrappers.h>
#include <utilities/SecCFError.h>
#include <utilities/SecAKSWrappers.h>


#include <Security/SecKeyPriv.h>

#include <utilities/der_plist.h>
#include <utilities/der_plist_internal.h>
#include <corecrypto/ccder.h>

#include <AssertMacros.h>
#include <assert.h>

#import <notify.h>

#include <Security/SecureObjectSync/SOSInternal.h>
#include <Security/SecureObjectSync/SOSCircle.h>
#include <Security/SecureObjectSync/SOSCircleV2.h>
#include <Security/SecureObjectSync/SOSRing.h>
#include <Security/SecureObjectSync/SOSRingUtils.h>
#include <Security/SecureObjectSync/SOSCloudCircle.h>
#include <securityd/SOSCloudCircleServer.h>
#include <Security/SecureObjectSync/SOSEngine.h>
#include <Security/SecureObjectSync/SOSPeer.h>
#include <Security/SecureObjectSync/SOSFullPeerInfo.h>
#include <Security/SecureObjectSync/SOSPeerInfo.h>
#include <Security/SecureObjectSync/SOSPeerInfoInternal.h>
#include <Security/SecureObjectSync/SOSUserKeygen.h>
#include <utilities/iCloudKeychainTrace.h>

#include <Security/SecItemPriv.h>


struct __OpaqueSOSAccount {
    CFRuntimeBase           _base;

    CFDictionaryRef         gestalt;

    CFDataRef               backup_key;

    SOSFullPeerInfoRef      my_identity;
    SOSCircleRef            trusted_circle;

    CFMutableDictionaryRef  trusted_rings;
    CFMutableDictionaryRef  backups;

    CFMutableSetRef         retirees;

    bool      user_public_trusted;
    CFDataRef user_key_parameters;
    SecKeyRef user_public;
    SecKeyRef previous_public;
    enum DepartureReason    departure_code;
    CFMutableDictionaryRef  expansion; // All CFTypes and Keys
    
    // Non-persistent data
    dispatch_queue_t        queue;
    CFMutableDictionaryRef  notification_cleanups;

    SOSDataSourceFactoryRef factory;
    SecKeyRef _user_private;
    CFDataRef _password_tmp;

    dispatch_source_t user_private_timer;
    int               lock_notification_token;
    
    SOSTransportKeyParameterRef key_transport;
    SOSTransportCircleRef       circle_transport;
    SOSTransportMessageRef      kvs_message_transport;
    SOSTransportMessageRef      ids_message_transport;
    
    //indicates if changes in circle, rings, or retirements need to be pushed
    bool                         circle_rings_retirements_need_attention;
    
    // Live Notification
    CFMutableArrayRef       change_blocks;
    CFMutableDictionaryRef  waitForInitialSync_blocks;
};
extern const CFStringRef kSOSEscrowRecord;

SOSAccountRef SOSAccountCreateBasic(CFAllocatorRef allocator,
                                    CFDictionaryRef gestalt,
                                    SOSDataSourceFactoryRef factory);

bool SOSAccountEnsureFactoryCircles(SOSAccountRef a);

void SOSAccountSetToNew(SOSAccountRef a);

bool SOSAccountIsMyPeerActive(SOSAccountRef account, CFErrorRef* error);

SOSTransportMessageRef SOSAccountGetMessageTransportFor(SOSAccountRef account, SOSPeerInfoRef peerInfo);

// MARK: In Sync checking

void SOSAccountEnsureSyncChecking(SOSAccountRef account);
void SOSAccountCancelSyncChecking(SOSAccountRef account);
bool SOSAccountCheckHasBeenInSync(SOSAccountRef account);

void SOSAccountCleanupNotificationForAllPeers(SOSAccountRef account);


// MARK: DER Stuff


size_t der_sizeof_data_or_null(CFDataRef data, CFErrorRef* error);

uint8_t* der_encode_data_or_null(CFDataRef data, CFErrorRef* error, const uint8_t* der, uint8_t* der_end);

const uint8_t* der_decode_data_or_null(CFAllocatorRef allocator, CFDataRef* data,
                                       CFErrorRef* error,
                                       const uint8_t* der, const uint8_t* der_end);

size_t der_sizeof_fullpeer_or_null(SOSFullPeerInfoRef data, CFErrorRef* error);

uint8_t* der_encode_fullpeer_or_null(SOSFullPeerInfoRef data, CFErrorRef* error, const uint8_t* der, uint8_t* der_end);

const uint8_t* der_decode_fullpeer_or_null(CFAllocatorRef allocator, SOSFullPeerInfoRef* data,
                                       CFErrorRef* error,
                                       const uint8_t* der, const uint8_t* der_end);


size_t der_sizeof_public_bytes(SecKeyRef publicKey, CFErrorRef* error);

uint8_t* der_encode_public_bytes(SecKeyRef publicKey, CFErrorRef* error, const uint8_t* der, uint8_t* der_end);

const uint8_t* der_decode_public_bytes(CFAllocatorRef allocator, CFIndex algorithmID, SecKeyRef* publicKey, CFErrorRef* error, const uint8_t* der, const uint8_t* der_end);


// Persistence

SOSAccountRef SOSAccountCreateFromDER(CFAllocatorRef allocator,
                                      SOSDataSourceFactoryRef factory,
                                      CFErrorRef* error,
                                      const uint8_t** der_p, const uint8_t *der_end);

SOSAccountRef SOSAccountCreateFromData(CFAllocatorRef allocator, CFDataRef circleData,
                                       SOSDataSourceFactoryRef factory,
                                       CFErrorRef* error);

size_t SOSAccountGetDEREncodedSize(SOSAccountRef account, CFErrorRef *error);

uint8_t* SOSAccountEncodeToDER(SOSAccountRef account, CFErrorRef* error, const uint8_t* der, uint8_t* der_end);

CFDataRef SOSAccountCopyEncodedData(SOSAccountRef account, CFAllocatorRef allocator, CFErrorRef *error);

// Update

bool SOSAccountHandleCircleMessage(SOSAccountRef account,
                                   CFStringRef circleName, CFDataRef encodedCircleMessage, CFErrorRef *error);

CF_RETURNS_RETAINED
CFDictionaryRef SOSAccountHandleRetirementMessages(SOSAccountRef account, CFDictionaryRef circle_retirement_messages, CFErrorRef *error);


bool SOSAccountHandleUpdateCircle(SOSAccountRef account,
                                  SOSCircleRef prospective_circle,
                                  bool writeUpdate,
                                  CFErrorRef *error);

void SOSAccountNotifyEngines(SOSAccountRef account);

bool SOSAccountSyncingV0(SOSAccountRef account);

// My Peer
bool SOSAccountHasFullPeerInfo(SOSAccountRef account, CFErrorRef* error);
SOSPeerInfoRef SOSAccountGetMyPeerInfo(SOSAccountRef account);
SOSFullPeerInfoRef SOSAccountGetMyFullPeerInfo(SOSAccountRef account);
CFStringRef SOSAccountGetMyPeerID(SOSAccountRef a);
bool SOSAccountIsMyPeerInBackupAndCurrentInView(SOSAccountRef account, CFStringRef viewname);
bool SOSAccountUpdateOurPeerInBackup(SOSAccountRef account, SOSRingRef oldRing, CFErrorRef *error);
bool SOSAccountIsPeerInBackupAndCurrentInView(SOSAccountRef account, SOSPeerInfoRef testPeer, CFStringRef viewname);
bool SOSDeleteV0Keybag(CFErrorRef *error);
void SOSAccountForEachBackupView(SOSAccountRef account,  void (^operation)(const void *value));
bool SOSAccountUpdatePeerInfo(SOSAccountRef account, CFStringRef updateDescription, CFErrorRef *error, bool (^update)(SOSFullPeerInfoRef fpi, CFErrorRef *error));

// Currently permitted backup rings.
void SOSAccountForEachBackupRingName(SOSAccountRef account, void (^operation)(CFStringRef value));

// My Circle
bool SOSAccountHasCircle(SOSAccountRef account, CFErrorRef* error);
SOSCircleRef SOSAccountGetCircle(SOSAccountRef a, CFErrorRef *error);
SOSCircleRef SOSAccountEnsureCircle(SOSAccountRef a, CFStringRef name, CFErrorRef *error);

bool SOSAccountUpdateCircleFromRemote(SOSAccountRef account, SOSCircleRef newCircle, CFErrorRef *error);
bool SOSAccountUpdateCircle(SOSAccountRef account, SOSCircleRef newCircle, CFErrorRef *error);
bool SOSAccountModifyCircle(SOSAccountRef account,
                            CFErrorRef* error,
                            bool (^action)(SOSCircleRef circle));

void AppendCircleKeyName(CFMutableArrayRef array, CFStringRef name);

CFStringRef SOSInterestListCopyDescription(CFArrayRef interests);


// FullPeerInfos - including Cloud Identity
SOSFullPeerInfoRef CopyCloudKeychainIdentity(SOSPeerInfoRef cloudPeer, CFErrorRef *error);

SecKeyRef GeneratePermanentFullECKey(int keySize, CFStringRef name, CFErrorRef* error);

bool SOSAccountEnsureFullPeerAvailable(SOSAccountRef account, CFErrorRef * error);

bool SOSAccountIsAccountIdentity(SOSAccountRef account, SOSPeerInfoRef peer_info, CFErrorRef *error);
bool SOSAccountFullPeerInfoVerify(SOSAccountRef account, SecKeyRef privKey, CFErrorRef *error);
SOSPeerInfoRef GenerateNewCloudIdentityPeerInfo(CFErrorRef *error);

// Credentials
bool SOSAccountHasPublicKey(SOSAccountRef account, CFErrorRef* error);
void SOSAccountSetPreviousPublic(SOSAccountRef account);
bool SOSAccountPublishCloudParameters(SOSAccountRef account, CFErrorRef* error);
bool SOSAccountRetrieveCloudParameters(SOSAccountRef account, SecKeyRef *newKey,
                                       CFDataRef derparms,
                                       CFDataRef *newParameters, CFErrorRef* error);

//DSID
bool SOSAccountUpdateDSID(SOSAccountRef account, CFStringRef dsid);

//
// Key extraction
//

SecKeyRef SOSAccountCopyDeviceKey(SOSAccountRef account, CFErrorRef *error);
SecKeyRef SOSAccountCopyPublicKeyForPeer(SOSAccountRef account, CFStringRef peer_id, CFErrorRef *error);

// Testing
void SOSAccountSetLastDepartureReason(SOSAccountRef account, enum DepartureReason reason);
void SOSAccountSetUserPublicTrustedForTesting(SOSAccountRef account);

static inline void CFArrayAppendValueIfNot(CFMutableArrayRef array, CFTypeRef value, CFTypeRef excludedValue)
{
    if (!CFEqualSafe(value, excludedValue))
        CFArrayAppendValue(array, value);
}

static inline CFMutableDictionaryRef CFDictionaryEnsureCFDictionaryAndGetCurrentValue(CFMutableDictionaryRef dict, CFTypeRef key)
{
    CFMutableDictionaryRef result = (CFMutableDictionaryRef) CFDictionaryGetValue(dict, key);

    if (!isDictionary(result)) {
        result = CFDictionaryCreateMutableForCFTypes(kCFAllocatorDefault);
        CFDictionarySetValue(dict, key, result);
        CFReleaseSafe(result);
    }

    return result;
}

static inline CFMutableArrayRef CFDictionaryEnsureCFArrayAndGetCurrentValue(CFMutableDictionaryRef dict, CFTypeRef key)
{
    CFMutableArrayRef result = (CFMutableArrayRef) CFDictionaryGetValue(dict, key);

    if (!isArray(result)) {
        result = CFArrayCreateMutableForCFTypes(kCFAllocatorDefault);
        CFDictionarySetValue(dict, key, result);
        CFReleaseSafe(result);
    }

    return result;
}

void SOSAccountPurgeIdentity(SOSAccountRef account);
bool sosAccountLeaveCircle(SOSAccountRef account, SOSCircleRef circle, CFErrorRef* error);
bool sosAccountLeaveRing(SOSAccountRef account, SOSRingRef ring, CFErrorRef* error);
CFMutableDictionaryRef SOSAccountGetRings(SOSAccountRef a, CFErrorRef *error);
CFMutableDictionaryRef SOSAccountGetBackups(SOSAccountRef a, CFErrorRef *error);
bool SOSAccountUpdateBackUp(SOSAccountRef account, CFStringRef viewname, CFErrorRef *error);
bool SOSAccountEnsureBackupStarts(SOSAccountRef account);

bool SOSAccountEnsurePeerRegistration(SOSAccountRef account, CFErrorRef *error);

extern CFIndex whichTransportType;
extern const CFStringRef kSOSDSIDKey;
extern const CFStringRef SOSTransportMessageTypeIDS;
extern const CFStringRef SOSTransportMessageTypeKVS;

extern const CFStringRef kSOSUnsyncedViewsKey;

typedef enum{
    kSOSTransportNone = 0,
    kSOSTransportIDS = 1,
    kSOSTransportKVS = 2,
    kSOSTransportFuture = 3,
    kSOSTransportPresent = 4
}TransportType;

SOSPeerInfoRef SOSAccountCopyPeerWithID(SOSAccountRef account, CFStringRef peerid, CFErrorRef *error);

bool SOSAccountSetValue(SOSAccountRef account, const void *key, const void *value, CFErrorRef *error);
bool SOSAccountClearValue(SOSAccountRef account, const void *key, CFErrorRef *error);
const void *SOSAccountGetValue(SOSAccountRef account, const void *key, CFErrorRef *error);

bool SOSAccountAddEscrowToPeerInfo(SOSAccountRef account, SOSFullPeerInfoRef myPeer, CFErrorRef *error);
bool SOSAccountAddEscrowRecords(SOSAccountRef account, CFStringRef dsid, CFDictionaryRef record, CFErrorRef *error);
bool SOSAccountCheckForRings(SOSAccountRef a, CFErrorRef *error);
bool SOSAccountHandleUpdateRing(SOSAccountRef account, SOSRingRef prospective_ring, bool writeUpdate, CFErrorRef *error);
SOSRingRef SOSAccountGetRing(SOSAccountRef a, CFStringRef ringName, CFErrorRef *error);
SOSRingRef SOSAccountRingCreateForName(SOSAccountRef a, CFStringRef ringName, CFErrorRef *error);
bool SOSAccountEnsureRings(SOSAccountRef a, CFErrorRef *error);
bool SOSAccountUpdateRingFromRemote(SOSAccountRef account, SOSRingRef newRing, CFErrorRef *error);
bool SOSAccountUpdateRing(SOSAccountRef account, SOSRingRef newRing, CFErrorRef *error);
bool SOSAccountModifyRing(SOSAccountRef account, CFStringRef ringName,
                          CFErrorRef* error,
                          bool (^action)(SOSRingRef ring));
CFDataRef SOSAccountRingGetPayload(SOSAccountRef account, CFStringRef ringName, CFErrorRef *error);
SOSRingRef SOSAccountRingCopyWithPayload(SOSAccountRef account, CFStringRef ringName, CFDataRef payload, CFErrorRef *error);
bool SOSAccountRemoveBackupPeers(SOSAccountRef account, CFArrayRef peerIDs, CFErrorRef *error);
bool SOSAccountResetRing(SOSAccountRef account, CFStringRef ringName, CFErrorRef *error);
bool SOSAccountResetAllRings(SOSAccountRef account, CFErrorRef *error);
bool SOSAccountCheckPeerAvailability(SOSAccountRef account, CFErrorRef *error);

//
// MARK: Backup translation functions
//

CFStringRef SOSBackupCopyRingNameForView(CFStringRef viewName);

//
// Security tool test/debug functions
//

CFDataRef SOSAccountCopyAccountStateFromKeychain(CFErrorRef *error);
bool SOSAccountDeleteAccountStateFromKeychain(CFErrorRef *error);
CFDataRef SOSAccountCopyEngineStateFromKeychain(CFErrorRef *error);
bool SOSAccountDeleteEngineStateFromKeychain(CFErrorRef *error);


#endif
