/**
 * @file fs.h
 * @brief SSL service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// CaCertificateId
typedef enum {
    SslCaCertificateId_All                                                    =   -1,            ///< [3.0.0+] All

    SslCaCertificateId_NintendoCAG3                                           =    1,            ///< NintendoCAG3
    SslCaCertificateId_NintendoClass2CAG3                                     =    2,            ///< NintendoClass2CAG3

    SslCaCertificateId_AmazonRootCA1                                          = 1000,            ///< AmazonRootCA1
    SslCaCertificateId_StarfieldServicesRootCertificateAuthorityG2            = 1001,            ///< StarfieldServicesRootCertificateAuthorityG2
    SslCaCertificateId_AddTrustExternalCARoot                                 = 1002,            ///< AddTrustExternalCARoot
    SslCaCertificateId_COMODOCertificationAuthority                           = 1003,            ///< COMODOCertificationAuthority
    SslCaCertificateId_UTNDATACorpSGC                                         = 1004,            ///< UTNDATACorpSGC
    SslCaCertificateId_UTNUSERFirstHardware                                   = 1005,            ///< UTNUSERFirstHardware
    SslCaCertificateId_BaltimoreCyberTrustRoot                                = 1006,            ///< BaltimoreCyberTrustRoot
    SslCaCertificateId_CybertrustGlobalRoot                                   = 1007,            ///< CybertrustGlobalRoot
    SslCaCertificateId_VerizonGlobalRootCA                                    = 1008,            ///< VerizonGlobalRootCA
    SslCaCertificateId_DigiCertAssuredIDRootCA                                = 1009,            ///< DigiCertAssuredIDRootCA
    SslCaCertificateId_DigiCertAssuredIDRootG2                                = 1010,            ///< DigiCertAssuredIDRootG2
    SslCaCertificateId_DigiCertGlobalRootCA                                   = 1011,            ///< DigiCertGlobalRootCA
    SslCaCertificateId_DigiCertGlobalRootG2                                   = 1012,            ///< DigiCertGlobalRootG2
    SslCaCertificateId_DigiCertHighAssuranceEVRootCA                          = 1013,            ///< DigiCertHighAssuranceEVRootCA
    SslCaCertificateId_EntrustnetCertificationAuthority2048                   = 1014,            ///< EntrustnetCertificationAuthority2048
    SslCaCertificateId_EntrustRootCertificationAuthority                      = 1015,            ///< EntrustRootCertificationAuthority
    SslCaCertificateId_EntrustRootCertificationAuthorityG2                    = 1016,            ///< EntrustRootCertificationAuthorityG2
    SslCaCertificateId_GeoTrustGlobalCA2                                      = 1017,            ///< GeoTrustGlobalCA2 ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_GeoTrustGlobalCA                                       = 1018,            ///< GeoTrustGlobalCA ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_GeoTrustPrimaryCertificationAuthorityG3                = 1019,            ///< GeoTrustPrimaryCertificationAuthorityG3 ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_GeoTrustPrimaryCertificationAuthority                  = 1020,            ///< GeoTrustPrimaryCertificationAuthority ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_GlobalSignRootCA                                       = 1021,            ///< GlobalSignRootCA
    SslCaCertificateId_GlobalSignRootCAR2                                     = 1022,            ///< GlobalSignRootCAR2
    SslCaCertificateId_GlobalSignRootCAR3                                     = 1023,            ///< GlobalSignRootCAR3
    SslCaCertificateId_GoDaddyClass2CertificationAuthority                    = 1024,            ///< GoDaddyClass2CertificationAuthority
    SslCaCertificateId_GoDaddyRootCertificateAuthorityG2                      = 1025,            ///< GoDaddyRootCertificateAuthorityG2
    SslCaCertificateId_StarfieldClass2CertificationAuthority                  = 1026,            ///< StarfieldClass2CertificationAuthority
    SslCaCertificateId_StarfieldRootCertificateAuthorityG2                    = 1027,            ///< StarfieldRootCertificateAuthorityG2
    SslCaCertificateId_thawtePrimaryRootCAG3                                  = 1028,            ///< thawtePrimaryRootCAG3 ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_thawtePrimaryRootCA                                    = 1029,            ///< thawtePrimaryRootCA ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_VeriSignClass3PublicPrimaryCertificationAuthorityG3    = 1030,            ///< VeriSignClass3PublicPrimaryCertificationAuthorityG3 ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_VeriSignClass3PublicPrimaryCertificationAuthorityG5    = 1031,            ///< VeriSignClass3PublicPrimaryCertificationAuthorityG5 ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_VeriSignUniversalRootCertificationAuthority            = 1032,            ///< VeriSignUniversalRootCertificationAuthority ([8.0.0+] ::SslTrustedCertStatus is ::SslTrustedCertStatus_EnabledNotTrusted)
    SslCaCertificateId_DSTRootCAX3                                            = 1033,            ///< [6.0.0+] DSTRootCAX3
} SslCaCertificateId;

/// TrustedCertStatus
typedef enum {
    SslTrustedCertStatus_Invalid                                              =   -1,            ///< Invalid
    SslTrustedCertStatus_Removed                                              =    0,            ///< Removed
    SslTrustedCertStatus_EnabledTrusted                                       =    1,            ///< EnabledTrusted
    SslTrustedCertStatus_EnabledNotTrusted                                    =    2,            ///< EnabledNotTrusted
    SslTrustedCertStatus_Revoked                                              =    3,            ///< Revoked
} SslTrustedCertStatus;

/// FlushSessionCacheOptionType
typedef enum {
    SslFlushSessionCacheOptionType_SingleHost                                 =    0,            ///< SingleHost. Uses the input string.
    SslFlushSessionCacheOptionType_AllHosts                                   =    1,            ///< AllHosts. Doesn't use the input string.
} SslFlushSessionCacheOptionType;

/// DebugOptionType
typedef enum {
    SslDebugOptionType_AllowDisableVerifyOption                               =    0,            ///< AllowDisableVerifyOption
} SslDebugOptionType;

/// SslVersion
typedef enum {
    SslVersion_Auto                                                           =  0x1,            ///< Auto
    SslVersion_TlsV10                                                         =  0x8,            ///< TlsV10
    SslVersion_TlsV11                                                         = 0x10,            ///< TlsV11
    SslVersion_TlsV12                                                         = 0x20,            ///< TlsV12
} SslVersion;

/// CertificateFormat
typedef enum {
    SslCertificateFormat_Pem                                                  = 1,               ///< Pem
    SslCertificateFormat_Der                                                  = 2,               ///< Der
} SslCertificateFormat;

/// InternalPki
typedef enum {
    SslInternalPki_DeviceClientCertDefault                                    = 1,               ///< DeviceClientCertDefault
} SslInternalPki;

/// ContextOption
typedef enum {
    SslContextOption_CrlImportDateCheckEnable                                 = 1,               ///< CrlImportDateCheckEnable. The default value at the time of \ref sslCreateContext is value 1.
} SslContextOption;

/// VerifyOption. The default bitmask value at the time of \ref sslContextCreateConnection is ::SslVerifyOption_PeerCa | ::SslVerifyOption_HostName.
/// [5.0.0+] \ref sslConnectionSetVerifyOption: (::SslVerifyOption_PeerCa | ::SslVerifyOption_HostName) must be set, unless: ::SslOptionType_SkipDefaultVerify is set, or [9.0.0+] ::SslDebugOptionType_AllowDisableVerifyOption is set.
/// [6.0.0+] \ref sslConnectionSetVerifyOption: Following that, if ::SslVerifyOption_EvPolicyOid is set, then the following options must be set (besides the previously mentioned one): ::SslVerifyOption_PeerCa and ::SslVerifyOption_DateCheck.
typedef enum {
    SslVerifyOption_PeerCa                                                    = BIT(0),          ///< PeerCa
    SslVerifyOption_HostName                                                  = BIT(1),          ///< HostName
    SslVerifyOption_DateCheck                                                 = BIT(2),          ///< DateCheck
    SslVerifyOption_EvCertPartial                                             = BIT(3),          ///< EvCertPartial
    SslVerifyOption_EvPolicyOid                                               = BIT(4),          ///< [6.0.0+] EvPolicyOid
    SslVerifyOption_EvCertFingerprint                                         = BIT(5),          ///< [6.0.0+] EvCertFingerprint
} SslVerifyOption;

/// IoMode. The default value at the time of \ref sslContextCreateConnection is ::SslIoMode_Blocking.
typedef enum {
    SslIoMode_Blocking                                                        = 1,               ///< Blocking
    SslIoMode_NonBlocking                                                     = 2,               ///< NonBlocking
} SslIoMode;

/// PollEvent
typedef enum {
    SslPollEvent_Read                                                         = BIT(0),          ///< Read
    SslPollEvent_Write                                                        = BIT(1),          ///< Write
    SslPollEvent_Except                                                       = BIT(2),          ///< Except
} SslPollEvent;

/// SessionCacheMode
typedef enum {
    SslSessionCacheMode_None                                                  = 0,               ///< None
    SslSessionCacheMode_SessionId                                             = 1,               ///< SessionId
    SslSessionCacheMode_SessionTicket                                         = 2,               ///< SessionTicket
} SslSessionCacheMode;

/// RenegotiationMode
typedef enum {
    SslRenegotiationMode_None                                                 = 0,               ///< None
    SslRenegotiationMode_Secure                                               = 1,               ///< Secure
} SslRenegotiationMode;

/// OptionType. The default bool flags value for these at the time of \ref sslContextCreateConnection is cleared.
typedef enum {
    SslOptionType_DoNotCloseSocket                                            = 0,               ///< DoNotCloseSocket. See \ref sslConnectionSetSocketDescriptor. This is only available if \ref sslConnectionSetSocketDescriptor wasn't used yet.
    SslOptionType_GetServerCertChain                                          = 1,               ///< [3.0.0+] GetServerCertChain
    SslOptionType_SkipDefaultVerify                                           = 2,               ///< [5.0.0+] SkipDefaultVerify. Checked by \ref sslConnectionSetVerifyOption, see \ref SslVerifyOption.
    SslOptionType_EnableAlpn                                                  = 3,               ///< [9.0.0+] EnableAlpn. Only available with \ref sslConnectionSetOption. \ref sslConnectionSetSocketDescriptor should have been used prior to this - this will optionally use state setup by that, without throwing an error if that cmd wasn't used.
} SslOptionType;

/// AlpnProtoState
typedef enum {
    SslAlpnProtoState_NoSupport                                               = 0,               ///< NoSupport
    SslAlpnProtoState_Negotiated                                              = 1,               ///< Negotiated
    SslAlpnProtoState_NoOverlap                                               = 2,               ///< NoOverlap
    SslAlpnProtoState_Selected                                                = 3,               ///< Selected
    SslAlpnProtoState_EarlyValue                                              = 4,               ///< EarlyValue
} SslAlpnProtoState;

/// SslContext
typedef struct {
    Service s;                                  ///< ISslContext
} SslContext;

/// SslConnection
typedef struct {
    Service s;                                  ///< ISslConnection
} SslConnection;

/// BuiltInCertificateInfo
typedef struct {
    u32 cert_id;                                ///< \ref SslCaCertificateId
    u32 status;                                 ///< \ref SslTrustedCertStatus
    u64 cert_size;                              ///< CertificateSize
    u8 *cert_data;                              ///< CertificateData (converted from an offset to a ptr), in DER format.
} SslBuiltInCertificateInfo;

/// CipherInfo
typedef struct {
    char cipher[0x40];                          ///< Cipher string.
    char protocol_version[0x8];                 ///< Protocol version string.
} SslCipherInfo;

/// Initialize ssl. A default value of 0x3 can be used for num_sessions. This must be 0x1-0x4.
Result sslInitialize(u32 num_sessions);

/// Exit ssl.
void sslExit(void);

/// Gets the Service object for the actual ssl service session.
Service* sslGetServiceSession(void);

/**
 * @brief CreateContext
 * @param[out] c \ref SslContext
 * @param[in] ssl_version \ref SslVersion
 */
Result sslCreateContext(SslContext *c, SslVersion ssl_version);

/**
 * @brief GetContextCount
 * @note Not used by official sw.
 * @param[out] out Output value.
 */
Result sslGetContextCount(u32 *out);

/**
 * @brief GetCertificates
 * @param[in] buffer Output buffer. The start of this buffer is an array of \ref SslBuiltInCertificateInfo, with the specified count. The cert data (SslBuiltInCertificateInfo::data) is located after this array.
 * @param[in] size Output buffer size, this should be the size from \ref sslGetCertificateBufSize.
 * @param[in] ca_cert_ids Input array of \ref SslCaCertificateId.
 * @param[in] count Size of the ca_cert_ids array in entries.
 * @param[out] total_out [3.0.0+] Total output entries. Will always match count on pre-3.0.0. This will differ from count when ::SslCaCertificateId_All is used.
 */
Result sslGetCertificates(void* buffer, u32 size, u32 *ca_cert_ids, u32 count, u32 *total_out);

/**
 * @brief GetCertificateBufSize
 * @param[in] ca_cert_ids Input array of \ref SslCaCertificateId.
 * @param[in] count Size of the ca_cert_ids array in entries.
 * @param[out] out Output size.
 */
Result sslGetCertificateBufSize(u32 *ca_cert_ids, u32 count, u32 *out);

/**
 * @brief FlushSessionCache
 * @note Only available on [5.0.0+].
 * @param[in] str Input string. Must be NULL with ::SslFlushSessionCacheOptionType_AllHosts.
 * @param[in] str_bufsize String buffer size, excluding NUL-terminator. Hence, this should be actual_bufsize-1. This must be 0 with ::SslFlushSessionCacheOptionType_AllHosts.
 * @param[in] type \ref SslFlushSessionCacheOptionType
 * @param[out] out Output value.
 */
Result sslFlushSessionCache(const char *str, size_t str_bufsize, SslFlushSessionCacheOptionType type, u32 *out);

/**
 * @brief SetDebugOption
 * @note Only available on [6.0.0+].
 * @note The official impl of this doesn't actually use the cmd.
 * @param[in] buffer Input buffer, must not be NULL. The u8 from here is copied to state.
 * @param[in] size Buffer size, must not be 0.
 * @param[in] type \ref SslDebugOptionType
 */
Result sslSetDebugOption(const void* buffer, size_t size, SslDebugOptionType type);

/**
 * @brief GetDebugOption
 * @note Only available on [6.0.0+].
 * @param[out] buffer Output buffer, must not be NULL. An u8 is written here loaded from state.
 * @param[in] size Buffer size, must not be 0.
 * @param[in] type \ref SslDebugOptionType
 */
Result sslGetDebugOption(void* buffer, size_t size, SslDebugOptionType type);

///@name ISslContext
///@{

/**
 * @brief Closes a Context object.
 * @param c \ref SslContext
 */
void sslContextClose(SslContext *c);

/**
 * @brief SetOption
 * @note Prior to 4.x this is stubbed.
 * @param c \ref SslContext
 * @param[in] option \ref SslContextOption
 * @param[in] value Value to set. With ::SslContextOption_CrlImportDateCheckEnable, this must be 0 or 1.
 */
Result sslContextSetOption(SslContext *c, SslContextOption option, s32 value);

/**
 * @brief GetOption
 * @note Prior to 4.x this is stubbed.
 * @param c \ref SslContext
 * @param[in] option \ref SslContextOption
 * @param[out] out Output value.
 */
Result sslContextGetOption(SslContext *c, SslContextOption option, s32 *out);

/**
 * @brief CreateConnection
 * @param c \ref SslContext
 * @param[out] conn Output \ref SslConnection.
 */
Result sslContextCreateConnection(SslContext *c, SslConnection *conn);

/**
 * @brief GetConnectionCount
 * @note Not exposed by official sw.
 * @param c \ref SslContext
 * @param[out] out Output value.
 */
Result sslContextGetConnectionCount(SslContext *c, u32 *out);

/**
 * @brief ImportServerPki
 * @param c \ref SslContext
 * @param[in] buffer Input buffer, must not be NULL.
 * @param[in] size Input buffer size.
 * @param[in] format \ref SslCertificateFormat
 * @param[out] id Output Id.
 */
Result sslContextImportServerPki(SslContext *c, const void* buffer, u32 size, SslCertificateFormat format, u64 *id);

/**
 * @brief ImportClientPki
 * @param c \ref SslContext
 * @param[in] buf0 First input buffer, must not be NULL.
 * @param[in] size0 First input buffer size.
 * @param[in] buf1 Second input buffer, this can only be NULL if size1 is 0.
 * @param[in] size1 Second input buffer size, this can only be 0 if buf1 is NULL.
 * @param[out] id Output Id.
 */
Result sslContextImportClientPki(SslContext *c, const void* buf0, u32 size0, const void* buf1, u32 size1, u64 *id);

/**
 * @brief Remove the specified *Pki, or on [3.0.0+] Crl.
 * @param c \ref SslContext
 * @param[in] id Id
 */
Result sslContextRemovePki(SslContext *c, u64 id);

/**
 * @brief RegisterInternalPki
 * @param c \ref SslContext
 * @param[in] internal_pki \ref SslInternalPki
 * @param[out] id Output Id.
 */
Result sslContextRegisterInternalPki(SslContext *c, SslInternalPki internal_pki, u64 *id);

/**
 * @brief AddPolicyOid
 * @param c \ref SslContext
 * @param[in] str Input string.
 * @param[in] str_bufsize String buffer size, excluding NUL-terminator (must not match the string length). Hence, this should be actual_bufsize-1. This must not be >0xff.
 */
Result sslContextAddPolicyOid(SslContext *c, const char* str, u32 str_bufsize);

/**
 * @brief ImportCrl
 * @note Only available on [3.0.0+].
 * @param c \ref SslContext
 * @param[in] buffer Input buffer, must not be NULL.
 * @param[in] size Input buffer size.
 * @param[out] id Output Id.
 */
Result sslContextImportCrl(SslContext *c, const void* buffer, u32 size, u64 *id);

///@}

///@name ISslConnection
///@{

/**
 * @brief Closes a Connection object.
 * @param c \ref SslConnection
 */
void sslConnectionClose(SslConnection *c);

/**
 * @brief SetSocketDescriptor. Do not use directly, use \ref socketSslConnectionSetSocketDescriptor instead.
 * @note An error is thrown if this was used previously.
 * @param c \ref SslConnection
 * @param[in] sockfd sockfd
 * @param[out] out_sockfd sockfd. Prior to using \ref sslConnectionClose, this must be closed if it's not negative (it will be -1 if ::SslOptionType_DoNotCloseSocket is set).
 */
Result sslConnectionSetSocketDescriptor(SslConnection *c, int sockfd, int *out_sockfd);

/**
 * @brief SetHostName
 * @param c \ref SslConnection
 * @param[in] str Input string.
 * @param[in] str_bufsize String buffer size. This must not be >0xff.
 */
Result sslConnectionSetHostName(SslConnection *c, const char* str, u32 str_bufsize);

/**
 * @brief SetVerifyOption
 * @param c \ref SslConnection
 * @param[in] verify_option Input bitmask of \ref SslVerifyOption.
 */
Result sslConnectionSetVerifyOption(SslConnection *c, u32 verify_option);

/**
 * @brief SetIoMode
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[in] mode \ref SslIoMode
 */
Result sslConnectionSetIoMode(SslConnection *c, SslIoMode mode);

/**
 * @brief GetSocketDescriptor. Do not use directly, use \ref socketSslConnectionGetSocketDescriptor instead.
 * @note This gets the input sockfd which was previously saved in state by \ref sslConnectionSetSocketDescriptor.
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] sockfd Output sockfd.
 */
Result sslConnectionGetSocketDescriptor(SslConnection *c, int *sockfd);

/**
 * @brief GetHostName
 * @param c \ref SslConnection
 * @param[out] str Output string buffer.
 * @param[in] str_bufsize String buffer size, must be large enough for the entire output string.
 * @param[out] out Output string length.
 */
Result sslConnectionGetHostName(SslConnection *c, char* str, u32 str_bufsize, u32 *out);

/**
 * @brief GetVerifyOption
 * @param c \ref SslConnection
 * @param[out] out Output bitmask of \ref SslVerifyOption.
 */
Result sslConnectionGetVerifyOption(SslConnection *c, u32 *out);

/**
 * @brief GetIoMode
 * @param c \ref SslConnection
 * @param[out] out \ref SslIoMode
 */
Result sslConnectionGetIoMode(SslConnection *c, SslIoMode *out);

/**
 * @brief DoHandshake
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @note \ref sslConnectionSetHostName must have been used previously with a non-empty string when ::SslVerifyOption_HostName is set.
 * @note The DoHandshakeGetServerCert cmd is only used if both server_certbuf/server_certbuf_size are set, otherwise the DoHandshake cmd is used (in which case out0/out1 will be left at value 0).
 * @param c \ref SslConnection
 * @param[out] out0 Optional first output value, can be NULL.
 * @param[out] out1 Optional second output value, can be NULL.
 * @param[out] server_certbuf Optional output server cert buffer, can be NULL.
 * @param[in] server_certbuf_size Optional output server cert buffer size, can be 0.
 */
Result sslConnectionDoHandshake(SslConnection *c, u32 *out0, u32 *out1, void* server_certbuf, u32 server_certbuf_size);

/**
 * @brief Read
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] buffer Output buffer, must not be NULL.
 * @param[in] size Output buffer size, must not be 0.
 * @param[out] out Output value.
 */
Result sslConnectionRead(SslConnection *c, void* buffer, u32 size, u32 *out);

/**
 * @brief Write
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[in] buffer Input buffer, must not be NULL.
 * @param[in] size Input buffer size, must not be 0.
 * @param[out] out Output value.
 */
Result sslConnectionWrite(SslConnection *c, const void* buffer, u32 size, u32 *out);

/**
 * @brief Pending
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] out Output value.
 */
Result sslConnectionPending(SslConnection *c, s32 *out);

/**
 * @brief Peek
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] buffer Output buffer, must not be NULL.
 * @param[in] size Output buffer size, must not be 0.
 * @param[out] out Output value.
 */
Result sslConnectionPeek(SslConnection *c, void* buffer, u32 size, u32 *out);

/**
 * @brief Poll
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[in] in_pollevent Input bitmask of \ref SslPollEvent.
 * @param[out] out_pollevent Output bitmask of \ref SslPollEvent.
 * @param[in] timeout Timeout in milliseconds.
 */
Result sslConnectionPoll(SslConnection *c, u32 in_pollevent, u32 *out_pollevent, u32 timeout);

/**
 * @brief GetVerifyCertError
 * @note The value in state is cleared after loading it.
 * @param c \ref SslConnection
 */
Result sslConnectionGetVerifyCertError(SslConnection *c);

/**
 * @brief GetNeededServerCertBufferSize
 * @param c \ref SslConnection
 * @param[out] out Output value.
 */
Result sslConnectionGetNeededServerCertBufferSize(SslConnection *c, u32 *out);

/**
 * @brief SetSessionCacheMode
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[in] mode \ref SslSessionCacheMode
 */
Result sslConnectionSetSessionCacheMode(SslConnection *c, SslSessionCacheMode mode);

/**
 * @brief GetSessionCacheMode
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] out \ref SslSessionCacheMode
 */
Result sslConnectionGetSessionCacheMode(SslConnection *c, SslSessionCacheMode *out);

/**
 * @brief GetSessionCacheMode
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 */
Result sslConnectionFlushSessionCache(SslConnection *c);

/**
 * @brief SetRenegotiationMode
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[in] mode \ref SslRenegotiationMode
 */
Result sslConnectionSetRenegotiationMode(SslConnection *c, SslRenegotiationMode mode);

/**
 * @brief GetRenegotiationMode
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] out \ref SslRenegotiationMode
 */
Result sslConnectionGetRenegotiationMode(SslConnection *c, SslRenegotiationMode *out);

/**
 * @brief SetOption
 * @param c \ref SslConnection
 * @param[in] option \ref SslOptionType
 * @param[in] flag Input flag value.
 */
Result sslConnectionSetOption(SslConnection *c, SslOptionType option, bool flag);

/**
 * @brief GetOption
 * @param c \ref SslConnection
 * @param[in] option \ref SslOptionType
 * @param[out] out Output flag value.
 */
Result sslConnectionGetOption(SslConnection *c, SslOptionType option, bool *out);

/**
 * @brief GetVerifyCertErrors
 * @note An error is thrown when the cmd is successful, if the two output u32s match.
 * @param[out] out0 First output value, must not be NULL.
 * @param[out] out1 Second output value.
 * @param[out] errors Output array of Result, must not be NULL.
 * @param[in] count Size of the errors array in entries.
 */
Result sslConnectionGetVerifyCertErrors(SslConnection *c, u32 *out0, u32 *out1, Result *errors, u32 count);

/**
 * @brief GetCipherInfo
 * @note Only available on [4.0.0+].
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] out \ref SslCipherInfo
 */
Result sslConnectionGetCipherInfo(SslConnection *c, SslCipherInfo *out);

/**
 * @brief SetNextAlpnProto
 * @note Only available on [9.0.0+].
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[in] buffer Input buffer, must not be NULL.
 * @param[in] size Input buffer size, must not be 0. Must be at least 0x2.
 */
Result sslConnectionSetNextAlpnProto(SslConnection *c, const u8* buffer, u32 size);

/**
 * @brief GetNextAlpnProto
 * @note Only available on [9.0.0+].
 * @note \ref sslConnectionSetSocketDescriptor must have been used prior to this successfully.
 * @param c \ref SslConnection
 * @param[out] state \ref SslAlpnProtoState
 * @param[out] out Output value.
 * @param[out] buffer Output buffer, must not be NULL.
 * @param[in] size Output buffer size, must not be 0.
 */
Result sslConnectionGetNextAlpnProto(SslConnection *c, SslAlpnProtoState *state, u32 *out, u8 *buffer, u32 size);

///@}

