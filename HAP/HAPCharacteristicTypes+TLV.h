// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#ifndef HAP_CHARACTERISTIC_TYPES_TLV_H
#define HAP_CHARACTERISTIC_TYPES_TLV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+Internal.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Streaming Status Body.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-17
 */
HAP_ENUM_BEGIN(uint8_t, HAPStreamingStatusType) {
    /** Streaming Status */
    kHAPStreamingStatusType_Value = 0x01,

} HAP_ENUM_END(uint8_t, HAPStreamingStatusType);

/**
 * Streaming Status TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-17 Status
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_StreamingStatus) {
    /** Available */
    kHAPTLVValue_StreamingStatus_Available = 0,

    /** In Use */
    kHAPTLVValue_StreamingStatus_InUse = 1,

    /** Unavailable */
    kHAPTLVValue_StreamingStatus_Unavailable = 2,

} HAP_ENUM_END(uint8_t, HAPTLVValue_StreamingStatus);


/**
 * Selected RTP Stream Configuration.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-7
 */
HAP_ENUM_BEGIN(uint8_t, HAPSelectedRTPConfigurationType) {
    /** Session Control */
    kHAPSelectedRTPConfigurationType_SessionControl = 0x01,

    /** Selected Video Parameters */
    kHAPSelectedRTPConfigurationType_SelectedVideoParams = 0x02,

    /** Selected Audio Parameters */
    kHAPSelectedRTPConfigurationType_SelectedAudioParams = 0x03,

} HAP_ENUM_END(uint8_t, HAPSelectedRTPConfigurationType);

/**
 * Session Control.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-8
 */
HAP_ENUM_BEGIN(uint8_t, HAPSessionControlType) {
    /** Session ID */
    kHAPSessionControlType_SessionID = 0x01,

    /** Command */
    kHAPSessionControlType_Command = 0x02,

} HAP_ENUM_END(uint8_t, HAPSessionControlType);

/**
 * Session Control Command TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-8 Command
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_SessionControl) {
    /** End streaming session */
    kHAPTLVValue_SessionControl_End = 0,

    /** Start streaming session */
    kHAPTLVValue_SessionControl_Start = 1,

    /** Suspend streaming session */
    kHAPTLVValue_SessionControl_Suspend = 2,

    /** Resume streaming session */
    kHAPTLVValue_SessionControl_Resume = 3,

    /** Reconfigure streaming sessions */
    kHAPTLVValue_SessionControl_Reconfigure = 4,

} HAP_ENUM_END(uint8_t, HAPTLVValue_SessionControl);

/**
 * Selected Video Parameters.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-9
 */
HAP_ENUM_BEGIN(uint8_t, HAPSelectedVideoParametersType) {
    /** Selected Video Codec type */
    kHAPSelectedVideoParametersType_CodecType = 0x01,

    /** Selected Video Codec parameters */
    kHAPHAPSelectedVideoParametersType_CodecParameters = 0x02,

    /** Selected Video attributes */
    kHAPHAPSelectedVideoParametersType_Attributes = 0x03,

    /** Selected Video RTP parameters */
    kHAPHAPSelectedVideoParametersType_RTPParameters = 0x04,

} HAP_ENUM_END(uint8_t, HAPSelectedVideoParametersType);

/**
 * Selected Video RTP Parameters.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-10
 */
HAP_ENUM_BEGIN(uint8_t, HAPSelectedVideoRTPParametersType) {
    /** Payload type */
    kHAPSelectedVideoRTPParametersType_PayloadType = 0x01,

    /** Synchronization Source for Video */
    kHAPSelectedVideoRTPParametersType_SSRC = 0x02,

    /** Maximum Bitrate for Video */
    kHAPSelectedVideoRTPParametersType_MaxBitrate = 0x03,

    /** Minimum RTCP interval */
    kHAPSelectedVideoRTPParametersType_MinRTCPInterval = 0x04,

    /** Max MTU */
    kHAPSelectedVideoRTPParametersType_MaxMTU = 0x05,

} HAP_ENUM_END(uint8_t, HAPSelectedVideoRTPParametersType);

/**
 * Selected Audio Parameters.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-11
 */
HAP_ENUM_BEGIN(uint8_t, HAPSelectedAudioParametersType) {
    /** Selected Audio Codec type */
    kHAPSelectedAudioParametersType_CodecType = 0x01,

    /** Selected Audio Codec parameters */
    kHAPSelectedAudioParametersType_CodecParameters = 0x02,

    /** Selected Audio RTP parameters */
    kHAPSelectedAudioParametersType_RTPParameters = 0x03,

    /** Comfort Noise */
    kHAPSelectedAudioParametersType_ComfortNoise = 0x04,

} HAP_ENUM_END(uint8_t, HAPSelectedAudioParametersType);



/**
 * Selected Audio RTP Parameters.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-12
 */
HAP_ENUM_BEGIN(uint8_t, HAPSelectedAudioRTPParametersType) {
    /** Payload type */
    kHAPSelectedAudioRTPParametersType_PayloadType = 0x01,

    /** Synchronization Source for Video */
    kHAPSelectedAudioRTPParametersType_SSRC = 0x02,

    /** Maximum Bitrate for Video */
    kHAPSelectedAudioRTPParametersType_MaxBitrate = 0x03,

    /** Minimum RTCP interval */
    kHAPSelectedAudioRTPParametersType_MinRTCPInterval = 0x04,

    /** Comfort Noise Payload Type */
    kHAPSelectedAudioRTPParametersType_ComfortNoisePayloadType = 0x06,

} HAP_ENUM_END(uint8_t, HAPSelectedAudioRTPParametersType);

/**
 * Setup Endpoints Read/Write
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-13 + Table 9-16
 */
HAP_ENUM_BEGIN(uint8_t, HAPSetupEndpointsType) {
    /** Session ID */
    kHAPSetupEndpointsType_SessionID = 0x01,

    kHAPSetupEndpointsType_Status = 0x02,

    /** Selected Controller/Accessory Address */
    kHAPSetupEndpointsType_Address = 0x03,

    /** SRTP Parameters for Video */
    kHAPSetupEndpointsType_SRTPVideoParams = 0x04,

    /** SRTP Parameters for Audio */
    kHAPSetupEndpointsType_SRTPAudioParams = 0x05,

    kHAPSetupEndpointsType_VideoSSRC = 0x06,

    kHAPSetupEndpointsType_AudioSSRC = 0x07,

} HAP_ENUM_END(uint8_t, HAPSetupEndpointsType);

/**
 * Controller Address.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-14
 */
HAP_ENUM_BEGIN(uint8_t, HAPControllerAddressType) {
    /** IP address version */
    kHAPControllerAddressType_IPVersion = 0x01,

    /** IP address */
    kHAPControllerAddressType_IPAddress = 0x02,

    /** Video RTP Port */
    kHAPControllerAddressType_VideoRTPPort = 0x03,

    /** Audio RTP Port */
    kHAPControllerAddressType_AudioRTPPort = 0x04,

} HAP_ENUM_END(uint8_t, HAPControllerAddressType);

/**
 * IP address version TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-14 IP address version
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_IPAddressVersion) {
    /** IPv4 */
    kHAPTLVValue_IPAddressVersion_IPV4 = 0,

    /** IPv6 */
    kHAPTLVValue_IPAddressVersion_IPV6 = 1,

} HAP_ENUM_END(uint8_t, HAPTLVValue_IPAddressVersion);

/**
 * SRTP Crypto Params.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-15
 */
HAP_ENUM_BEGIN(uint8_t, HAPSRTPCryptoParamsType) {
    /** SRTP Crypto Suite */
    kHAPHAPSRTPCryptoParamsType_CryptoSuite = 0x01,

    /** SRTP Master Key */
    kHAPHAPSRTPCryptoParamsType_MasterKey = 0x02,

    /** SRTP Master Salt */
    kHAPHAPSRTPCryptoParamsType_MasterSalt = 0x03,

} HAP_ENUM_END(uint8_t, HAPSRTPCryptoParamsType);

/**
 * SRTP Crypto Suite TLV Values
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-15 SRTP Crypto Suite
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_SRTPCryptoSuite) {
    /** AES_CM_128_HMAC_SHA1_80 */
    kHAPTLVValue_SRTPCryptoSuite_AES128 = 0,

    /** AES_256_CM_HMAC_SHA1_80 */
    kHAPTLVValue_SRTPCryptoSuite_AES256 = 1,

    kHAPTLVValue_SRTPCryptoSuite_Disabled = 2,

} HAP_ENUM_END(uint8_t, HAPTLVValue_SRTPCryptoSuite);

/**
 * Setup Endpoints Read Response Status TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-16 Status
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_SetupEndpointsStatus) {
    /** Available */
    kHAPTLVValue_SetupEndpointsStatus_Success = 0,

    /** In Use */
    kHAPTLVValue_SetupEndpointsStatus_Busy = 1,

    /** Unavailable */
    kHAPTLVValue_SetupEndpointsStatus_Error = 2,

} HAP_ENUM_END(uint8_t, HAPTLVValue_SetupEndpoints);

/**
 * Supported Audio Stream Configuration.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-18
 */
HAP_ENUM_BEGIN(uint8_t, HAPSupportedAudioConfigurationType) {
    /** Audio Codec Configuration */
    kHAPSupportedAudioConfigurationType_AudioCodecConfiguration = 0x01,

    /** Comfort Noise Support */
    kHAPSupportedAudioConfigurationType_ComfortNoiseSupport = 0x02,

} HAP_ENUM_END(uint8_t, HAPSupportedAudioConfigurationType);

/**
 * Audio Codec Configuraton.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-19
 */
HAP_ENUM_BEGIN(uint8_t, HAPSupportedAudioCodecConfigurationType) {
    /** Codec type */
    kHAPSupportedAudioCodecConfigurationType_Codec = 0x01,

    /** Audio Codec Parameters */
    kHAPSupportedAudioCodecConfigurationType_CodecParameters = 0x02,

} HAP_ENUM_END(uint8_t, HAPSupportedAudioCodecConfigurationType);

/**
 * Audio Codec Type TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-19 Audio Codec Type
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_AudioCodecType) {
    /** AAC-ELD */
    kHAPTLVValue_AudioCodecType_AACELD = 2,
    /** Opus */
    kHAPTLVValue_AudioCodecType_Opus = 3,
    /** AMR */
    kHAPTLVValue_AudioCodecType_AMR = 5,
    /** AMR-WB */
    kHAPTLVValue_AudioCodecType_AMRWB = 6,

} HAP_ENUM_END(uint8_t, HAPTLVValue_AudioCodecType);

/**
 * Audio Codec Parameters.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-21
 */
HAP_ENUM_BEGIN(uint8_t, HAPAudioCodecParametersType) {
    /** Audio Channels */
    kHAPAudioCodecParametersType_Channels = 0x01,

    /** Bit-rate */
    kHAPAudioCodecParametersType_Bitrate = 0x02,

    /** Sample rate */
    kHAPAudioCodecParametersType_SampleRate = 0x03,

    /** RTP time */
    kHAPAudioCodecParametersType_RTPTime = 0x04,

} HAP_ENUM_END(uint8_t, HAPAudioCodecParametersType);

/**
 * Audio Codec Bit-rate TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-21 Bit-rate
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_AudioCodecBitrate) {
    /** Variable bit-rate */
    kHAPTLVValue_AudioCodecBitrate_Variable = 0,

    /** Constant bit-rate */
    kHAPTLVValue_AudioCodecBitrate_Constant = 1,

} HAP_ENUM_END(uint8_t, HAPTLVValue_AudioCodecBitrate);

/**
 * Audio Codec Sample rate TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-21 Sample rate
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_AudioCodecSampleRate) {
    /** 8KHz */
    kHAPTLVValue_AudioCodecSampleRate_8KHZ = 0,

    /** 16KHz */
    kHAPTLVValue_AudioCodecSampleRate_16KHZ = 1,

    /** 24KHz */
    kHAPTLVValue_AudioCodecSampleRate_24KHZ = 2,

} HAP_ENUM_END(uint8_t, HAPTLVValue_AudioCodecSampleRate);

/**
 * Supported RTP Configuration.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-24
 */
HAP_ENUM_BEGIN(uint8_t, HAPSupportedRTPConfigurationType) {
    /** SRTP Crypto Suite */
    kHAPSupportedRTPConfigurationType_CryptoSuite = 0x02,

} HAP_ENUM_END(uint8_t, HAPSupportedRTPConfigurationType);

/**
 * Supported Video Stream Configuration.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-25
 */
HAP_ENUM_BEGIN(uint8_t, HAPSupportedVideoConfigurationType) {
    /** Video Codec Configuration */
    kHAPSupportedVideoConfigurationType_VideoCodecConfiguration = 0x01,

} HAP_ENUM_END(uint8_t, HAPSupportedVideoConfigurationType);

/**
 * Video Codec Configuration.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-26
 */
HAP_ENUM_BEGIN(uint8_t, HAPVideoCodecConfigurationType) {
    /** Codec Type */
    kHAPVideoCodecConfigurationType_CodecType = 0x01,

    /** Video Codec Parameters */
    kHAPVideoCodecConfigurationType_CodecParameters = 0x02,

    /** Video Attributes */
    kHAPVideoCodecConfigurationType_Attributes = 0x03,

} HAP_ENUM_END(uint8_t, HAPVideoCodecConfigurationType);

/**
 * Video Codec Type TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-26 Video Codec Type
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_VideoCodecType) {
    /** H.264 */
    kHAPTLVValue_VideoCodecType_H264 = 0,

} HAP_ENUM_END(uint8_t, HAPTLVValue_VideoCodecType);

/**
 * Video Codec Parameters.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-27
 */
HAP_ENUM_BEGIN(uint8_t, HAPVideoCodecParametersType) {
    /** ProfileID */
    kHAPVideoCodecParametersType_ProfileID = 0x01,

    /** Level */
    kHAPVideoCodecParametersType_Level = 0x02,

    /** Packetization mode */
    kHAPVideoCodecParametersType_PacketizationMode = 0x03,

    /** CVO Enabled */
    kHAPVideoCodecParametersType_CVOEnabled = 0x04,

    /** CVO ID */
    kHAPVideoCodecParametersType_CVOID = 0x05,

} HAP_ENUM_END(uint8_t, HAPVideoCodecParametersType);

/**
 * Video Codec ProfileID TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-27 ProfileID
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_VideoCodecProfileID) {
    /** Constrained Baseline Profile */
    kHAPTLVValue_VideoCodecProfileID_Baseline = 0,

    /** Main Profile */
    kHAPTLVValue_VideoCodecProfileID_Main = 1,

    /** High Profile */
    kHAPTLVValue_VideoCodecProfileID_High = 2,

} HAP_ENUM_END(uint8_t, HAPTLVValue_VideoCodecProfileID);

/**
 * Video Codec Level TLV Values.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-27 Level
 */
HAP_ENUM_BEGIN(uint8_t, HAPTLVValue_VideoCodecLevel) {
    /** Level 3.1 */
    kHAPTLVValue_VideoCodecLevel_31 = 0,

    /** Level 3.2 */
    kHAPTLVValue_VideoCodecLevel_32 = 1,

    /** Level 4 */
    kHAPTLVValue_VideoCodecLevel_4 = 2,

} HAP_ENUM_END(uint8_t, HAPTLVValue_VideoCodecLevel);


/**
 * Video Attributes.
 *
 * @see HomeKit Accessory Protocol Specification R2
 *      Table 9-28
 */
HAP_ENUM_BEGIN(uint8_t, HAPVideoAttributesType) {
    /** Image width */
    kHAPVideoAttributesType_ImageWidth = 0x01,

    /** Image height */
    kHAPVideoAttributesType_ImageHeight = 0x02,

    /** Frame rate */
    kHAPVideoAttributesType_FrameRate = 0x03,

} HAP_ENUM_END(uint8_t, HAPVideoAttributesType);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
