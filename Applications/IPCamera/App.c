// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// An example that implements the light bulb HomeKit profile. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
//
// This implementation is platform-independent.
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration and its internal state.
//
//   2. Helper functions to load and save the state of the accessory.
//
//   3. The definitions for the HomeKit attribute database.
//
//   4. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   5. The initialization of the accessory state.
//
//   6. Callbacks that notify the server in case their associated value has changed.
#include <string.h>
#include <gst/gst.h>
#include <openssl/rand.h>

#include "HAP.h"
#include "App.h"
#include "Stream.h"
#include "DB.h"
#include "TLVHelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Domain used in the key value store for application data.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreDomain_Configuration ((HAPPlatformKeyValueStoreDomain) 0x00)

/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreDomain) 0x00)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NO_ACTIVE_CONNECTION 0
#define ENDPOINTS_EXCHANGED 1
#define STREAMING 2

const uint8_t sessionIDNULL [16] = {0};
const char ipAddressNULL [16] = {0};

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        HAPSessionRef* _Nullable active_stream_session;
        uint8_t stream_session_state;

        HAPSessionRef* _Nullable new_stream_session;
        //=-=-=-=-=-=-=-=-CAMERA RTP STREAM MANAGEMENT-=-=-=-=-=-=-=-=
        //STREAMING STATUS - Section 9.101
        // uint8_t streaming_status; // Type: 1, Length: 1,

        SelectedRTPStream selected_rtp_config;

        uint8_t endpoints_session_id[16];
        Endpoints controller_endpoints;
        Endpoints accessory_endpoints;

        SupportedAudioConfig supported_audio_config;

        SupportedRTPConfig supported_rtp_config;

        SupportedVideoConfig supported_video_config;

        //=-=-=-=-=-=-=-=-=MICROPHONE-=-=-=-=-=-=-=-=
        bool mute;

    } state;

    HAPAccessoryServerRef* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static GstElement* _Nullable pipeline = NULL;

static AccessoryConfiguration accessoryConfiguration = {
    .state = {
        .mute = false,
        .active_stream_session = NULL,
        .stream_session_state = NO_ACTIVE_CONNECTION,
    }
};


//----------------------------------------------------------------------------------------------------------------------

/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(void) {

    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.state) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
    }

    accessoryConfiguration.state.supported_audio_config = audioConfig;
    accessoryConfiguration.state.supported_rtp_config = rtpConfig;
    accessoryConfiguration.state.supported_video_config = videoConfig;
    accessoryConfiguration.state.accessory_endpoints = globalAccessoryEndpoints;

    // Set session id to 0
    memcpy(accessoryConfiguration.state.endpoints_session_id, sessionIDNULL, sizeof(sessionIDNULL));
    memcpy(accessoryConfiguration.state.selected_rtp_config.session_id, sessionIDNULL, sizeof(sessionIDNULL));
    memcpy(accessoryConfiguration.state.controller_endpoints.address.ip_address, ipAddressNULL, sizeof(ipAddressNULL));
}

/**
 * Save the accessory state to persistent memory.
 */
static void SaveAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the service.
 *
 * Note: Not constant to enable BCT Manual Name Change.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_IPCameras,
                                  .name = "William Camera",
                                  .manufacturer = "William",
                                  .model = "Camera1,1",
                                  .serialNumber = "0123456789AB",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &rtpStreamManagementService,
                                                                            &microphoneService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessory } };

//----------------------------------------------------------------------------------------------------------------------

// Only called when status is success
static HAPError BuildSetupEndpointsRead(HAPTLVWriterRef* responseWriter) {
    HAPTLVWriterRef subwriter;

    APPEND_TLV_WRITER_ARR(responseWriter, kHAPSetupEndpointsType_SessionID,accessoryConfiguration.state.endpoints_session_id);

    const uint8_t success = kHAPTLVValue_SetupEndpointsStatus_Success;

    APPEND_TLV_WRITER_VAL(responseWriter, kHAPSetupEndpointsType_Status, success);

    Endpoints *endpoints = &accessoryConfiguration.state.accessory_endpoints;

    const typeof(accessoryConfiguration.state.accessory_endpoints.address) *accessoryAddress =
        &accessoryConfiguration.state.accessory_endpoints.address;
    CreateNestedTLVWriter(responseWriter, &subwriter);
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPControllerAddressType_IPVersion, accessoryAddress->ip_version);

    HAPError err = AppendTLVWriterBuffer(&subwriter, kHAPControllerAddressType_IPAddress,
        endpoints->address.ip_address, strlen(accessoryAddress->ip_address));
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPControllerAddressType_VideoRTPPort, accessoryAddress->video_rtp_port);
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPControllerAddressType_AudioRTPPort, accessoryAddress->audio_rtp_port);

    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter, kHAPSetupEndpointsType_Address);

    // --- SRTP Parameters for Video
    CreateNestedTLVWriter(responseWriter, &subwriter);
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPHAPSRTPCryptoParamsType_CryptoSuite, endpoints->crypto_suite);
    if (RAND_bytes((unsigned char*)&endpoints->video_crypto_params.master_key,
        sizeof(endpoints->video_crypto_params.master_key)) != 1) {
        HAPLogError(&kHAPLog_Default, "%s: Out of entropy when writing: video key", __func__);
        return kHAPError_OutOfResources;
    }
    if (RAND_bytes((unsigned char*)&endpoints->video_crypto_params.master_salt,
        sizeof(endpoints->video_crypto_params.master_salt)) != 1) {
        HAPLogError(&kHAPLog_Default, "%s: Out of entropy when writing: video salt", __func__);
        return kHAPError_OutOfResources;
        }
    APPEND_TLV_WRITER_ARR(&subwriter, kHAPHAPSRTPCryptoParamsType_MasterKey, endpoints->video_crypto_params.master_key);
    APPEND_TLV_WRITER_ARR(&subwriter, kHAPHAPSRTPCryptoParamsType_MasterSalt,
        endpoints->video_crypto_params.master_salt);
    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter, kHAPSetupEndpointsType_SRTPVideoParams);

    // --- SRTP Parameters for Audio
    CreateNestedTLVWriter(responseWriter, &subwriter);
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPHAPSRTPCryptoParamsType_CryptoSuite, endpoints->crypto_suite);
    if (RAND_bytes((unsigned char*)&endpoints->audio_crypto_params.master_key,
        sizeof(endpoints->audio_crypto_params.master_key)) != 1) {
        HAPLogError(&kHAPLog_Default, "%s: Out of entropy when writing: audio key", __func__);
        return kHAPError_OutOfResources;
        }
    if (RAND_bytes((unsigned char*)&endpoints->audio_crypto_params.master_salt,
        sizeof(endpoints->audio_crypto_params.master_salt)) != 1) {
        HAPLogError(&kHAPLog_Default, "%s: Out of entropy when writing: audio salt", __func__);
        return kHAPError_OutOfResources;
        }
    APPEND_TLV_WRITER_ARR(&subwriter, kHAPHAPSRTPCryptoParamsType_MasterKey, endpoints->audio_crypto_params.master_key);
    APPEND_TLV_WRITER_ARR(&subwriter, kHAPHAPSRTPCryptoParamsType_MasterSalt,
        endpoints->audio_crypto_params.master_salt);
    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter, kHAPSetupEndpointsType_SRTPAudioParams);

    if (RAND_bytes((unsigned char*)&endpoints->video_ssrc, sizeof(endpoints->video_ssrc)) != 1) {
        HAPLogError(&kHAPLog_Default, "%s: Out of entropy when writing: video ssrc", __func__);
        return kHAPError_OutOfResources;
    }
    if (RAND_bytes((unsigned char*)&endpoints->audio_ssrc, sizeof(endpoints->audio_ssrc)) != 1) {
        HAPLogError(&kHAPLog_Default, "%s: Out of entropy when writing: audio ssrc", __func__);
        return kHAPError_OutOfResources;
    }

    APPEND_TLV_WRITER_VAL(responseWriter, kHAPSetupEndpointsType_VideoSSRC, endpoints->video_ssrc);
    APPEND_TLV_WRITER_VAL(responseWriter, kHAPSetupEndpointsType_AudioSSRC, endpoints->audio_ssrc);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneMuteRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.mute;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneMuteWrite(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.mute != value) {
        accessoryConfiguration.state.mute = value;

        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleStreamingStatusRead(
                HAPAccessoryServerRef* server HAP_UNUSED,
                const HAPTLV8CharacteristicReadRequest* request,
                HAPTLVWriterRef* responseWriter,
                void* _Nullable context HAP_UNUSED) {

    uint8_t status;

    if (accessoryConfiguration.state.stream_session_state == NO_ACTIVE_CONNECTION) {
        status = kHAPTLVValue_StreamingStatus_Available;
        HAPLogInfo(&kHAPLog_Default, "%s: Available", __func__);
    }
    else if (request->session == accessoryConfiguration.state.active_stream_session) {
        status = kHAPTLVValue_StreamingStatus_InUse;
        HAPLogInfo(&kHAPLog_Default, "%s: In Use", __func__);
    }
    else {
        status = kHAPTLVValue_StreamingStatus_Unavailable;
        HAPLogInfo(&kHAPLog_Default, "%s: Unavailable", __func__);
    }

    APPEND_TLV_WRITER_VAL(responseWriter, kHAPStreamingStatusType_Value, status);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSelectedRTPStreamConfigRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriterRef* responseWriter,
        void* _Nullable context HAP_UNUSED) {

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    //
    // if (!accessoryConfiguration.state.session_active ||
    //     accessoryConfiguration.state.active_stream_session != request->session) {
    //     return kHAPError_InvalidState;
    // }

    // Session is not the active stream session or stream is not active
    if (accessoryConfiguration.state.stream_session_state == NO_ACTIVE_CONNECTION) {
        return kHAPError_None;
    }
    if (accessoryConfiguration.state.active_stream_session != request->session ||
        accessoryConfiguration.state.stream_session_state != STREAMING) {
        HAPLogError(&kHAPLog_Default, "%s: Returned invalid state - not in streaming state", __func__);
        return kHAPError_InvalidState;
    }

    // Create subwriter to write nested TLVs
    HAPTLVWriterRef subwriter;
    HAPTLVWriterRef subsubwriter;

    CreateNestedTLVWriter(responseWriter, &subwriter);
    // Session Control
    // const auto *session_control = &accessoryConfiguration.state.selected_rtp_config.session_control;
    const SelectedRTPStream *selectedRTPStream = &accessoryConfiguration.state.selected_rtp_config;

    APPEND_TLV_WRITER_ARR(&subwriter, kHAPSessionControlType_SessionID, selectedRTPStream->session_id);
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPSessionControlType_Command, selectedRTPStream->command);
    // Append entire sub TLV to outer TLV
    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter, kHAPSelectedRTPConfigurationType_SessionControl);

    // --- Selected Video Parameters
    CreateNestedTLVWriter(responseWriter, &subwriter);
    const typeof(selectedRTPStream->selected_video_parameters) *videoParams =
        &selectedRTPStream->selected_video_parameters;
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPSelectedVideoParametersType_CodecType, videoParams->codec_type);

    // --- --- Selected Video Codec Parameters
    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    const typeof(videoParams->codec_parameters) *vidCodecParams = &videoParams->codec_parameters;
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_ProfileID, vidCodecParams->profile_id);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_Level, vidCodecParams->level);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_PacketizationMode,
        vidCodecParams->packetization_mode);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_CVOEnabled, vidCodecParams->cvo_enabled);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_CVOID, vidCodecParams->cvo_id);

    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPHAPSelectedVideoParametersType_CodecParameters);

    // --- --- Selected Video Attributes
    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    const typeof(selectedRTPStream->selected_video_parameters.attributes) *attributes =
        &selectedRTPStream->selected_video_parameters.attributes;
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoAttributesType_ImageWidth, attributes->image_width);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoAttributesType_ImageHeight, attributes->image_height);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoAttributesType_FrameRate, attributes->frame_rate);

    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPHAPSelectedVideoParametersType_Attributes);

    // --- --- Selected Video RTP Parameters
    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    const typeof(selectedRTPStream->selected_video_parameters.rtp_parameters) *vidRTPParams =
        &selectedRTPStream->selected_video_parameters.rtp_parameters;
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedVideoRTPParametersType_PayloadType, vidRTPParams->payload_type);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedVideoRTPParametersType_SSRC, vidRTPParams->sync_source);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedVideoRTPParametersType_MaxBitrate, vidRTPParams->max_bitrate);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedVideoRTPParametersType_MinRTCPInterval,
        vidRTPParams->min_rtcp_interval);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedVideoRTPParametersType_MaxMTU, vidRTPParams->max_mtu);

    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPHAPSelectedVideoParametersType_RTPParameters);

    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter, kHAPSelectedRTPConfigurationType_SelectedVideoParams);

    // --- Selected Audio Parameters
    CreateNestedTLVWriter(responseWriter, &subwriter);
    const typeof(selectedRTPStream->selected_audio_parameters) * audioParams =
        &selectedRTPStream->selected_audio_parameters;
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPSelectedAudioParametersType_CodecType, audioParams->codec_type);

    // --- --- Selected Audio Codec Parameters
    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    const typeof(audioParams->codec_parameters) * audCodecParams = &audioParams->codec_parameters;
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_Channels, audCodecParams->channels);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_Bitrate, audCodecParams->bitrate);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_SampleRate, audCodecParams->samplerate);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_RTPTime, audCodecParams->rtp_time);

    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPSelectedAudioParametersType_CodecParameters);

    // --- --- Selected Audio RTP Parameters
    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    const typeof(audioParams->rtp_parameters) * audRTPParams = &audioParams->rtp_parameters;
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedAudioRTPParametersType_PayloadType, audRTPParams->payload_type);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedAudioRTPParametersType_SSRC, audRTPParams->sync_source);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedAudioRTPParametersType_MaxBitrate, audRTPParams->max_bitrate);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedAudioRTPParametersType_MinRTCPInterval,
        audRTPParams->min_rtcp_interval);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPSelectedAudioRTPParametersType_ComfortNoisePayloadType,
        audRTPParams->comfort_noise_payload_type);

    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPSelectedAudioParametersType_RTPParameters);

    APPEND_TLV_WRITER_VAL(&subwriter, kHAPSelectedAudioParametersType_ComfortNoise, audioParams->comfort_noise);

    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter, kHAPSelectedRTPConfigurationType_SelectedAudioParams);

    return kHAPError_None;
}

HAPError DecodeSelectedRTPStreamConfigTLV(HAPTLVReaderRef* requestReader) {
    HAPTLVReaderRef subreader;
    HAPTLVReaderRef subsubreader;

    HAPTLV selectedVideoParamsTLV, selectedAudioParamsTLV;

    selectedVideoParamsTLV.type = kHAPSelectedRTPConfigurationType_SelectedVideoParams;
    selectedAudioParamsTLV.type = kHAPSelectedRTPConfigurationType_SelectedAudioParams;

    HAPError err = HAPTLVReaderGetAll(requestReader, (HAPTLV * const[]) {&selectedVideoParamsTLV,
        &selectedAudioParamsTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read selected rtp config tlv");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // if (memcmp(sessionIDTLV.value.bytes, accessoryConfiguration.state.selected_rtp_config.session_id,
    //     sessionIDTLV.value.numBytes) != 0) {
    //     HAPLogError(&kHAPLog_Default, "Session IDs do not match");
    //     HAPLogError(&kHAPLog_Default, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
    //         accessoryConfiguration.state.session_id[0], accessoryConfiguration.state.session_id[1], accessoryConfiguration.state.session_id[2], accessoryConfiguration.state.session_id[3], accessoryConfiguration.state.session_id[4], accessoryConfiguration.state.session_id[5], accessoryConfiguration.state.session_id[6], accessoryConfiguration.state.session_id[7], accessoryConfiguration.state.session_id[8], accessoryConfiguration.state.session_id[9], accessoryConfiguration.state.session_id[10], accessoryConfiguration.state.session_id[11], accessoryConfiguration.state.session_id[12], accessoryConfiguration.state.session_id[13], accessoryConfiguration.state.session_id[14], accessoryConfiguration.state.session_id[15]);
    //     memcpy(accessoryConfiguration.state.session_id, sessionIDTLV.value.bytes, sessionIDTLV.value.numBytes);
    //     HAPLogError(&kHAPLog_Default, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
    //         accessoryConfiguration.state.session_id[0], accessoryConfiguration.state.session_id[1], accessoryConfiguration.state.session_id[2], accessoryConfiguration.state.session_id[3], accessoryConfiguration.state.session_id[4], accessoryConfiguration.state.session_id[5], accessoryConfiguration.state.session_id[6], accessoryConfiguration.state.session_id[7], accessoryConfiguration.state.session_id[8], accessoryConfiguration.state.session_id[9], accessoryConfiguration.state.session_id[10], accessoryConfiguration.state.session_id[11], accessoryConfiguration.state.session_id[12], accessoryConfiguration.state.session_id[13], accessoryConfiguration.state.session_id[14], accessoryConfiguration.state.session_id[15]);
    //
    //     return kHAPError_InvalidState;
    // }

    HAPTLVReaderCreate(&subreader, (void*)selectedVideoParamsTLV.value.bytes, selectedVideoParamsTLV.value.numBytes);

    HAPTLV videoCodecTLV, videoCodecParamsTLV, videoAttributesTLV, videoRTPParamsTLV;

    videoCodecTLV.type = kHAPSelectedVideoParametersType_CodecType;
    videoCodecParamsTLV.type = kHAPHAPSelectedVideoParametersType_CodecParameters;
    videoAttributesTLV.type = kHAPHAPSelectedVideoParametersType_Attributes;
    videoRTPParamsTLV.type = kHAPHAPSelectedVideoParametersType_RTPParameters;

    typeof(accessoryConfiguration.state.selected_rtp_config.selected_video_parameters) * videoParams =
        &accessoryConfiguration.state.selected_rtp_config.selected_video_parameters;

    err = HAPTLVReaderGetAll(&subreader, (HAPTLV * const[]){&videoCodecTLV, &videoCodecParamsTLV, &videoAttributesTLV,
        &videoRTPParamsTLV, NULL});

    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read selected video params tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }

    // Only set on the start up of stream
    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start) {

        if (videoCodecTLV.value.numBytes != sizeof(videoParams->codec_type)) {
            HAPLogError(&kHAPLog_Default, "Codec type is not the right size %zu, %zu", videoCodecTLV.value.numBytes, sizeof(videoParams->codec_type));
            return kHAPError_InvalidData;
        }

        HAPTLVReaderCreate(&subsubreader, (void*)videoCodecParamsTLV.value.bytes, videoCodecParamsTLV.value.numBytes);

        HAPTLV profileIDTLV, levelTLV, packetModeTLV;//, cvoEnabledTLV, cvoIDTLV;

        profileIDTLV.type = kHAPVideoCodecParametersType_ProfileID;
        levelTLV.type = kHAPVideoCodecParametersType_Level;
        packetModeTLV.type = kHAPVideoCodecParametersType_PacketizationMode;
        // cvoEnabledTLV.type = kHAPVideoCodecParametersType_CVOEnabled;
        // cvoIDTLV.type = kHAPVideoCodecParametersType_CVOID;
        typeof(videoParams->codec_parameters) * videoCodecParams = &videoParams->codec_parameters;

        err = HAPTLVReaderGetAll(&subsubreader, (HAPTLV * const[]){&profileIDTLV, &levelTLV, &packetModeTLV, NULL});

        if (err) {
            HAPLogError(&kHAPLog_Default, "Failed to read video codec params tlv");
            HAPAssert(err = kHAPError_InvalidData);
            return err;
        }

        if (profileIDTLV.value.numBytes != sizeof(videoCodecParams->profile_id) ||
            levelTLV.value.numBytes != sizeof(videoCodecParams->level) ||
            packetModeTLV.value.numBytes != sizeof(videoCodecParams->packetization_mode)) {
            // cvoEnabledTLV.value.numBytes != sizeof(videoCodecParams->cvo_enabled)) {
            // cvoIDTLV.value.numBytes != sizeof(videoCodecParams->cvo_id)) {
            HAPLogError(&kHAPLog_Default, "Something is incorrect size in video codec params tlv: %zu, %zu", profileIDTLV.value.numBytes, sizeof(videoCodecParams->profile_id));
            return kHAPError_InvalidData;
        }

        videoCodecParams->profile_id = *(const uint8_t*)profileIDTLV.value.bytes;
        videoCodecParams->level = *(const uint8_t*)levelTLV.value.bytes;
        videoCodecParams->packetization_mode = *(const uint8_t*)packetModeTLV.value.bytes;
        // videoCodecParams->cvo_enabled = *(const uint8_t*)cvoEnabledTLV.value.bytes;
        // videoCodecParams->cvo_id = *(const uint8_t*)cvoIDTLV.value.bytes;
    }
    // if (videoCodecParams->cvo_enabled) {
    //     HAPLogError(&kHAPLog_Default, "CVO cannot be enabled");
    //     return kHAPError_InvalidData;
    // }

    HAPTLVReaderCreate(&subsubreader, (void*)videoAttributesTLV.value.bytes, videoAttributesTLV.value.numBytes);

    HAPTLV imageWidthTLV, imageHeightTLV, frameRateTLV;

    imageWidthTLV.type = kHAPVideoAttributesType_ImageWidth;
    imageHeightTLV.type = kHAPVideoAttributesType_ImageHeight;
    frameRateTLV.type = kHAPVideoAttributesType_FrameRate;
    typeof(videoParams->attributes) * videoAttributes = &videoParams->attributes;

    err = HAPTLVReaderGetAll(&subsubreader, (HAPTLV * const[]){&imageWidthTLV, &imageHeightTLV, &frameRateTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read video attributes tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }
    if (imageWidthTLV.value.numBytes != sizeof(videoAttributes->image_width) ||
        imageHeightTLV.value.numBytes != sizeof(videoAttributes->image_height) ||
        frameRateTLV.value.numBytes != sizeof(videoAttributes->frame_rate)) {
        HAPLogError(&kHAPLog_Default, "Something is incorrect size in video attributes tlv");
        return kHAPError_InvalidData;
    }

    HAPLogInfo(&kHAPLog_Default, "%u, %u, %u", *(const uint16_t*)imageWidthTLV.value.bytes,
        *(const uint16_t*)imageHeightTLV.value.bytes, *(const uint16_t*)frameRateTLV.value.bytes);

    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Reconfigure) {
        videoAttributes->image_width = *(const uint16_t*)imageWidthTLV.value.bytes;
        videoAttributes->image_height = *(const uint16_t*)imageHeightTLV.value.bytes;
        videoAttributes->frame_rate = *(const uint16_t*)frameRateTLV.value.bytes;
    }

    HAPTLVReaderCreate(&subsubreader, (void*)videoRTPParamsTLV.value.bytes, videoRTPParamsTLV.value.numBytes);

    HAPTLV payloadTypeTLV, videoSSRCTLV, maxBitrateTLV, minRTCPTLV, maxMTUTLV;

    payloadTypeTLV.type = kHAPSelectedVideoRTPParametersType_PayloadType;
    videoSSRCTLV.type = kHAPSelectedVideoRTPParametersType_SSRC;
    maxBitrateTLV.type = kHAPSelectedVideoRTPParametersType_MaxBitrate;
    minRTCPTLV.type = kHAPSelectedVideoRTPParametersType_MinRTCPInterval;
    maxMTUTLV.type = kHAPSelectedVideoRTPParametersType_MaxMTU;

    typeof(videoParams->rtp_parameters) * videoRTPParams = &videoParams->rtp_parameters;
    err = HAPTLVReaderGetAll(&subsubreader, (HAPTLV * const[]){&payloadTypeTLV, &videoSSRCTLV, &maxBitrateTLV,
        &minRTCPTLV, &maxMTUTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read video RTP params tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }
    if (payloadTypeTLV.value.numBytes != sizeof(videoRTPParams->payload_type) ||
        videoSSRCTLV.value.numBytes != sizeof(videoRTPParams->sync_source) ||
        maxBitrateTLV.value.numBytes != sizeof(videoRTPParams->max_bitrate) ||
        minRTCPTLV.value.numBytes != sizeof(videoRTPParams->min_rtcp_interval) ||
        maxMTUTLV.value.numBytes != sizeof(videoRTPParams->max_mtu)) {
        HAPLogError(&kHAPLog_Default, "Something is incorrect size in video rtp params tlv: %zu. %zu", videoSSRCTLV.value.numBytes, sizeof(videoRTPParams->sync_source));
        return kHAPError_InvalidData;
    }
    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start) {
        videoRTPParams->payload_type = *(const uint8_t*)payloadTypeTLV.value.bytes;
        videoRTPParams->sync_source = *(const uint32_t*)videoSSRCTLV.value.bytes;
        videoRTPParams->max_bitrate = *(const uint16_t*)maxBitrateTLV.value.bytes;
        videoRTPParams->min_rtcp_interval = *(const float*)minRTCPTLV.value.bytes;
        videoRTPParams->max_mtu = *(const uint16_t*)maxMTUTLV.value.bytes;
    }
    else if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Reconfigure) {
        videoRTPParams->max_bitrate = *(const uint16_t*)maxBitrateTLV.value.bytes;
        videoRTPParams->min_rtcp_interval = *(const float*)minRTCPTLV.value.bytes;
    }

    HAPTLVReaderCreate(&subreader, (void*)selectedAudioParamsTLV.value.bytes, selectedAudioParamsTLV.value.numBytes);

    HAPTLV audioCodecTLV, audioCodecParamsTLV, audioRTPParamsTLV, comfortNoiseTLV;

    audioCodecTLV.type = kHAPSelectedAudioParametersType_CodecType;
    audioCodecParamsTLV.type = kHAPSelectedAudioParametersType_CodecParameters;
    audioRTPParamsTLV.type = kHAPSelectedAudioParametersType_RTPParameters;
    comfortNoiseTLV.type = kHAPSelectedAudioParametersType_ComfortNoise;

    typeof(accessoryConfiguration.state.selected_rtp_config.selected_audio_parameters) * audioParams =
        &accessoryConfiguration.state.selected_rtp_config.selected_audio_parameters;

    err = HAPTLVReaderGetAll(&subreader, (HAPTLV * const[]){&audioCodecTLV, &audioCodecParamsTLV, &audioRTPParamsTLV,
        &comfortNoiseTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read audio params tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }
    if (audioCodecTLV.value.numBytes != sizeof(audioParams->codec_type) ||
        comfortNoiseTLV.value.numBytes != sizeof(audioParams->comfort_noise)) {
        HAPLogError(&kHAPLog_Default, "Something incorrect size in audio params tlv");
        return kHAPError_InvalidData;
    }
    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start) {
        audioParams->codec_type = *(const uint8_t*)audioCodecTLV.value.bytes;
        audioParams->comfort_noise = *(const uint8_t*)comfortNoiseTLV.value.bytes;
    }

    HAPTLVReaderCreate(&subsubreader, (void*)audioCodecParamsTLV.value.bytes, audioCodecParamsTLV.value.numBytes);

    HAPTLV channelsTLV, audioBitrateTLV, sampleRateTLV, rtpTimeTLV;

    channelsTLV.type = kHAPAudioCodecParametersType_Channels;
    audioBitrateTLV.type = kHAPAudioCodecParametersType_Bitrate;
    sampleRateTLV.type = kHAPAudioCodecParametersType_SampleRate;
    rtpTimeTLV.type = kHAPAudioCodecParametersType_RTPTime;

    typeof(audioParams->codec_parameters) *audioCodecParams = &audioParams->codec_parameters;

    err = HAPTLVReaderGetAll(&subsubreader, (HAPTLV * const[]){&channelsTLV, &audioBitrateTLV, &sampleRateTLV,
        &rtpTimeTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read audio codec params tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }
    if (channelsTLV.value.numBytes != sizeof(audioCodecParams->channels) ||
        audioBitrateTLV.value.numBytes != sizeof(audioCodecParams->bitrate) ||
        sampleRateTLV.value.numBytes != sizeof(audioCodecParams->samplerate) ||
        rtpTimeTLV.value.numBytes != sizeof(audioCodecParams->rtp_time)) {
        HAPLogError(&kHAPLog_Default, "Something incorrect size in audio codec params tlv");
        return kHAPError_InvalidData;
    }
    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start) {
        audioCodecParams->channels = *(const uint8_t*)channelsTLV.value.bytes;
        audioCodecParams->bitrate = *(const uint8_t*)audioBitrateTLV.value.bytes;
        audioCodecParams->samplerate = *(const uint8_t*)sampleRateTLV.value.bytes;
        audioCodecParams->rtp_time = *(const uint8_t*)rtpTimeTLV.value.bytes;
    }
    else if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Reconfigure) {
        audioCodecParams->samplerate = *(const uint8_t*)sampleRateTLV.value.bytes;
    }

    HAPTLVReaderCreate(&subsubreader, (void*)audioRTPParamsTLV.value.bytes, audioRTPParamsTLV.value.numBytes);

    HAPTLV payloadTypeAudioTLV, audioSSRCTLV, audioMaxBitrateTLV, audioMinRTCPTLV, comfortNoisePayloadTypeTLV;

    payloadTypeAudioTLV.type = kHAPSelectedAudioRTPParametersType_PayloadType;
    audioSSRCTLV.type = kHAPSelectedAudioRTPParametersType_SSRC;
    audioMaxBitrateTLV.type = kHAPSelectedAudioRTPParametersType_MaxBitrate;
    audioMinRTCPTLV.type = kHAPSelectedAudioRTPParametersType_MinRTCPInterval;
    comfortNoisePayloadTypeTLV.type = kHAPSelectedAudioRTPParametersType_ComfortNoisePayloadType;

    typeof(audioParams->rtp_parameters) *audioRTPParams = &audioParams->rtp_parameters;

    err = HAPTLVReaderGetAll(&subsubreader, (HAPTLV * const[]){&payloadTypeAudioTLV, &audioSSRCTLV, &audioMaxBitrateTLV,
        &audioMinRTCPTLV, &comfortNoisePayloadTypeTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read audio rtp params tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }
    if (payloadTypeAudioTLV.value.numBytes != sizeof(audioRTPParams->payload_type) ||
        audioSSRCTLV.value.numBytes != sizeof(audioRTPParams->sync_source) ||
        audioMaxBitrateTLV.value.numBytes != sizeof(audioRTPParams->max_bitrate) ||
        audioMinRTCPTLV.value.numBytes != sizeof(audioRTPParams->min_rtcp_interval) ||
        comfortNoisePayloadTypeTLV.value.numBytes != sizeof(audioRTPParams->comfort_noise_payload_type)) {
        HAPLogError(&kHAPLog_Default, "Something incorrect size in audio rtp params tlv");
        return kHAPError_InvalidData;
    }
    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start) {
        audioRTPParams->payload_type = *(const uint8_t*)payloadTypeAudioTLV.value.bytes;
        audioRTPParams->sync_source = *(const uint32_t*)audioSSRCTLV.value.bytes;
        audioRTPParams->max_bitrate = *(const uint16_t*)audioMaxBitrateTLV.value.bytes;
        audioRTPParams->min_rtcp_interval = *(const float*)audioMinRTCPTLV.value.bytes;
        audioRTPParams->comfort_noise_payload_type = *(const uint8_t*)comfortNoisePayloadTypeTLV.value.bytes;
    }
    else if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Reconfigure) {
        audioRTPParams->max_bitrate = *(const uint16_t*)audioMaxBitrateTLV.value.bytes;
        audioRTPParams->min_rtcp_interval = *(const float*)audioMinRTCPTLV.value.bytes;
    }

    return kHAPError_None;
}

// STEP 3 CONTROLLER SELECTS STREAM CONFIGURATION AND STREAM STARTS (final in configuration)
HAP_RESULT_USE_CHECK
HAPError HandleSelectedRTPStreamConfigWrite(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReaderRef* requestReader,
        void* _Nullable context HAP_UNUSED) {

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPError err;
    GstStateChangeReturn ret;
    
    // Only should be called if already streaming or starting stream
    if (!((accessoryConfiguration.state.stream_session_state == STREAMING ||
        accessoryConfiguration.state.stream_session_state == ENDPOINTS_EXCHANGED) &&
        accessoryConfiguration.state.active_stream_session == request->session)) {
        HAPLogInfo(&kHAPLog_Default, "Writing stream config when there is already an active stream ");
        return kHAPError_InvalidState;
    }

    HAPTLVReaderRef subreader;
    HAPTLV sessionControlTLV;
    sessionControlTLV.type = kHAPSelectedRTPConfigurationType_SessionControl;


    err = HAPTLVReaderGetAll(requestReader, (HAPTLV * const[]) {&sessionControlTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read selected rtp config tlv");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    HAPTLVReaderCreate(&subreader, (void*)sessionControlTLV.value.bytes, sessionControlTLV.value.numBytes);

    HAPTLV sessionIDTLV, commandTLV;

    sessionIDTLV.type = kHAPSessionControlType_SessionID;
    commandTLV.type = kHAPSessionControlType_Command;

    err = HAPTLVReaderGetAll(&subreader, (HAPTLV * const[]) {&sessionIDTLV, &commandTLV, NULL});

    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read session control tlv");
        HAPAssert(err = kHAPError_InvalidData);
        return err;
    }
    if (sessionIDTLV.value.numBytes != sizeof(accessoryConfiguration.state.selected_rtp_config.session_id) ||
        commandTLV.value.numBytes != sizeof(accessoryConfiguration.state.selected_rtp_config.command)) {
        HAPLogError(&kHAPLog_Default, "Session ID or Command is not the right size");
        return kHAPError_InvalidData;
        }

    accessoryConfiguration.state.selected_rtp_config.command = *(const uint8_t*)commandTLV.value.bytes;
    HAPLogError(&kHAPLog_Default, "COMMAND: %u", accessoryConfiguration.state.selected_rtp_config.command);

    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start) {
        if (accessoryConfiguration.state.stream_session_state == STREAMING) {
            HAPLogError(&kHAPLog_Default, "Session attempting to start while already streaming");
            return kHAPError_InvalidState;
        }
        memcpy(accessoryConfiguration.state.selected_rtp_config.session_id, sessionIDTLV.value.bytes,
            sessionIDTLV.value.numBytes);
    }
    else if (memcmp(accessoryConfiguration.state.selected_rtp_config.session_id, sessionIDTLV.value.bytes,
        sessionIDTLV.value.numBytes)) {
        HAPLogError(&kHAPLog_Default, "Invalid Session ID");
        return kHAPError_InvalidData;
    }

    if (accessoryConfiguration.state.selected_rtp_config.command != kHAPTLVValue_SessionControl_Start &&
        accessoryConfiguration.state.stream_session_state == ENDPOINTS_EXCHANGED) {
        HAPLogError(&kHAPLog_Default, "Session attempting to execute non start command while stream has not begun");
        return kHAPError_InvalidState;
        }

    if (accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Start ||
        accessoryConfiguration.state.selected_rtp_config.command == kHAPTLVValue_SessionControl_Reconfigure) {
        err = DecodeSelectedRTPStreamConfigTLV(requestReader);

        if (err) {
            HAPLogError(&kHAPLog_Default, "Decoding of RTP Stream Config Write Failed");
            accessoryConfiguration.state.stream_session_state = NO_ACTIVE_CONNECTION;
            accessoryConfiguration.state.active_stream_session = NULL;
            memcpy(accessoryConfiguration.state.selected_rtp_config.session_id, sessionIDNULL, sizeof(sessionIDNULL));
            return err;
        }
    }

    // Gstreamer logic depending on stream
    switch (accessoryConfiguration.state.selected_rtp_config.command) {
        case kHAPTLVValue_SessionControl_End:
            HAPLogInfo(&kHAPLog_Default, "ENDING STREAM");
            accessoryConfiguration.state.stream_session_state = NO_ACTIVE_CONNECTION;
            accessoryConfiguration.state.active_stream_session = NULL;
            memcpy(accessoryConfiguration.state.selected_rtp_config.session_id, sessionIDNULL, sizeof(sessionIDNULL));
            ret = gst_element_set_state (pipeline, GST_STATE_READY);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                HAPLogError(&kHAPLog_Default, "Unable to set the pipeline to the playing state.");
                gst_object_unref (pipeline);
                return kHAPError_Unknown;
            }
            break;
        case kHAPTLVValue_SessionControl_Start:
            HAPLogInfo(&kHAPLog_Default, "STARTING STREAM");
            accessoryConfiguration.state.stream_session_state = STREAMING;
            ConfigurePipeline(&accessoryConfiguration.state.controller_endpoints,
                &accessoryConfiguration.state.accessory_endpoints, &accessoryConfiguration.state.selected_rtp_config);
            /* Start playing */
            ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                HAPLogError(&kHAPLog_Default, "Unable to set the pipeline to the playing state.");
                gst_object_unref (pipeline);
                return kHAPError_Unknown;
            }
            break;
        case kHAPTLVValue_SessionControl_Suspend:
            HAPLogInfo(&kHAPLog_Default, "PAUSING STREAM");
            /* Pause Stream */
            ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                HAPLogError(&kHAPLog_Default, "Unable to set the pipeline to the pause state.");
                gst_object_unref (pipeline);
                return -1;
            }
            break;
        case kHAPTLVValue_SessionControl_Resume:
            HAPLogInfo(&kHAPLog_Default, "RESUMING STREAM");
            /* Resume playing */
            ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                HAPLogError(&kHAPLog_Default, "Unable to set the pipeline to the playing state.");
                gst_object_unref (pipeline);
                return -1;
            }
            break;
        case kHAPTLVValue_SessionControl_Reconfigure:
            HAPLogInfo(&kHAPLog_Default, "RECONFIGURING STREAM");
            ReconfigurePipeline(&accessoryConfiguration.state.selected_rtp_config);
            break;
        default:
            HAPLogInfo(&kHAPLog_Default, "INVALID STREAM COMMAND");
            accessoryConfiguration.state.stream_session_state = NO_ACTIVE_CONNECTION;
            accessoryConfiguration.state.active_stream_session = NULL;
            memcpy(accessoryConfiguration.state.selected_rtp_config.session_id, sessionIDNULL, sizeof(sessionIDNULL));
            ret = gst_element_set_state (pipeline, GST_STATE_READY);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                HAPLogError(&kHAPLog_Default, "Unable to set the pipeline to the playing state.");
                gst_object_unref (pipeline);
                return -1;
            }
            return kHAPError_InvalidData;
            break;
    }
    return kHAPError_None;
}

// STEP 2 - CONTROLLER RESPONDS WITH ITS ENDPOINTS AND SRTP CONFIG
HAP_RESULT_USE_CHECK
HAPError HandleSetupEndpointsRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriterRef* responseWriter,
        void* _Nullable context HAP_UNUSED) {

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPError err = kHAPError_None;

    // No active session. Last write performed by same session.
    if (accessoryConfiguration.state.stream_session_state == NO_ACTIVE_CONNECTION &&
        accessoryConfiguration.state.new_stream_session == request->session) {
        err = BuildSetupEndpointsRead(responseWriter);

        if (err) {
            HAPAssert(err = kHAPError_InvalidData);
            accessoryConfiguration.state.stream_session_state = NO_ACTIVE_CONNECTION;
            accessoryConfiguration.state.new_stream_session = NULL;
            return err;
        }

        // Lock in as active session.
        memcpy(accessoryConfiguration.state.endpoints_session_id, sessionIDNULL,
            sizeof(accessoryConfiguration.state.endpoints_session_id));
        accessoryConfiguration.state.active_stream_session = accessoryConfiguration.state.new_stream_session;
        accessoryConfiguration.state.new_stream_session = NULL;
        accessoryConfiguration.state.stream_session_state = ENDPOINTS_EXCHANGED;

        return kHAPError_None;
    }

    uint8_t status;
    // Not the same session as last write performed
    if (accessoryConfiguration.state.new_stream_session != request->session) {
        status = kHAPTLVValue_SetupEndpointsStatus_Error;
        APPEND_TLV_WRITER_VAL(responseWriter, kHAPSetupEndpointsType_Status, status);
        return kHAPError_None;
    }

    status = kHAPTLVValue_SetupEndpointsStatus_Busy;
    // Same session, but stream already in progress
    APPEND_TLV_WRITER_ARR(responseWriter, kHAPSetupEndpointsType_SessionID,
        accessoryConfiguration.state.endpoints_session_id);
    APPEND_TLV_WRITER_VAL(responseWriter, kHAPSetupEndpointsType_Status, status);

    return kHAPError_None;
}


// STEP 1 - CONTROLLER WRITES TO CAMERA ITS ENDPOINTS AND SRTP CONFIG (For rtcp feedback and optionally audio)
HAP_RESULT_USE_CHECK
HAPError HandleSetupEndpointsWrite(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReaderRef* requestReader,
        void* _Nullable context HAP_UNUSED) {

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPTLVReaderRef subReader;

    HAPTLV sessionIDTLV, controllerAddressTLV, videoParamsTLV, audioParamsTLV;
    sessionIDTLV.type = kHAPSetupEndpointsType_SessionID;
    controllerAddressTLV.type = kHAPSetupEndpointsType_Address;
    videoParamsTLV.type = kHAPSetupEndpointsType_SRTPVideoParams;
    audioParamsTLV.type = kHAPSetupEndpointsType_SRTPAudioParams;

    HAPError err = HAPTLVReaderGetAll(requestReader, (HAPTLV* const[]){ &sessionIDTLV, &controllerAddressTLV,
        &videoParamsTLV, &audioParamsTLV, NULL});
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLogError(&kHAPLog_Default, "Error getting tlv data from reader");
        return err;
    }

    if (sessionIDTLV.value.numBytes != sizeof(accessoryConfiguration.state.endpoints_session_id)) {
        HAPLogError(&kHAPLog_Default, "Session ID not same size");
        return kHAPError_InvalidData;
    }

    // Copy to respond even if stream already existing
    memcpy(accessoryConfiguration.state.endpoints_session_id, &sessionIDTLV.value.bytes,
        sessionIDTLV.value.numBytes);
    HAPLogInfo(&kHAPLog_Default, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
            accessoryConfiguration.state.endpoints_session_id[0], accessoryConfiguration.state.endpoints_session_id[1], accessoryConfiguration.state.endpoints_session_id[2], accessoryConfiguration.state.endpoints_session_id[3], accessoryConfiguration.state.endpoints_session_id[4], accessoryConfiguration.state.endpoints_session_id[5], accessoryConfiguration.state.endpoints_session_id[6], accessoryConfiguration.state.endpoints_session_id[7], accessoryConfiguration.state.endpoints_session_id[8], accessoryConfiguration.state.endpoints_session_id[9], accessoryConfiguration.state.endpoints_session_id[10], accessoryConfiguration.state.endpoints_session_id[11], accessoryConfiguration.state.endpoints_session_id[12], accessoryConfiguration.state.endpoints_session_id[13], accessoryConfiguration.state.endpoints_session_id[14], accessoryConfiguration.state.endpoints_session_id[15]);


    accessoryConfiguration.state.new_stream_session = request->session;

    // Active session already sent controller endpoints
    if (accessoryConfiguration.state.stream_session_state != NO_ACTIVE_CONNECTION) {
        // Active session calling which is unexpected behaviour
        if (accessoryConfiguration.state.active_stream_session == request->session) {
            memcpy(accessoryConfiguration.state.endpoints_session_id, sessionIDNULL,
        sizeof(sessionIDNULL));
            accessoryConfiguration.state.new_stream_session = NULL;
            HAPLogError(&kHAPLog_Default, "Session already wrote endpoints");
            return kHAPError_InvalidState;
        }
        // Other session trying to start stream while stream is active elsewhere
        HAPLogError(&kHAPLog_Default, "External session attempting while active stream");
        return kHAPError_None;
    }

    // CONTROLLER ADDRESS
    HAPTLVReaderCreate(&subReader, (void*)controllerAddressTLV.value.bytes, controllerAddressTLV.value.numBytes);
    HAPTLV ipVersionTLV, ipAddressTLV, videoRTPPortTLV, audioRTPPortTLV;
    typeof(accessoryConfiguration.state.controller_endpoints.address) *controllerAddress =
        &accessoryConfiguration.state.controller_endpoints.address;

    ipVersionTLV.type = kHAPControllerAddressType_IPVersion;
    ipAddressTLV.type = kHAPControllerAddressType_IPAddress;
    videoRTPPortTLV.type = kHAPControllerAddressType_VideoRTPPort;
    audioRTPPortTLV.type = kHAPControllerAddressType_AudioRTPPort;

    err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]){ &ipVersionTLV, &ipAddressTLV, &videoRTPPortTLV,
        &audioRTPPortTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read controller address tlv");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (ipVersionTLV.value.numBytes != sizeof(controllerAddress->ip_version) ||
        *(const uint8_t*)ipVersionTLV.value.bytes != kHAPTLVValue_IPAddressVersion_IPV4 ||
        ipAddressTLV.value.numBytes >= sizeof(controllerAddress->ip_address)) {
        HAPLogError(&kHAPLog_Default, "IP version not IPv4 or IP address is too big: %zu",
            ipAddressTLV.value.numBytes);
        return kHAPError_InvalidData;
    }

    controllerAddress->ip_version = *(const uint8_t*)ipVersionTLV.value.bytes;
    memcpy(controllerAddress->ip_address, ipAddressTLV.value.bytes, ipAddressTLV.value.numBytes);
    HAPLogInfo(&kHAPLog_Default, "IP Address: %s", controllerAddress->ip_address);

    if (!(videoRTPPortTLV.value.numBytes == audioRTPPortTLV.value.numBytes &&
        videoRTPPortTLV.value.numBytes == sizeof(controllerAddress->video_rtp_port))) {
        HAPLogError(&kHAPLog_Default, "Video or Audio RTP port not equal size");
        return kHAPError_InvalidData;
    }
    controllerAddress->video_rtp_port = *(const uint16_t*)videoRTPPortTLV.value.bytes;
    controllerAddress->audio_rtp_port = *(const uint16_t*)audioRTPPortTLV.value.bytes;

    // VIDEO CRYPTO SUITE
    HAPTLVReaderCreate(&subReader, (void*)videoParamsTLV.value.bytes, videoParamsTLV.value.numBytes);
    HAPTLV cryptoSuiteTLV, cryptoMasterKeyTLV, cryptoMasterSaltTLV;

    typeof(accessoryConfiguration.state.controller_endpoints.video_crypto_params) *videoCryptoParams =
        &accessoryConfiguration.state.controller_endpoints.video_crypto_params;

    cryptoSuiteTLV.type = kHAPHAPSRTPCryptoParamsType_CryptoSuite;
    cryptoMasterKeyTLV.type = kHAPHAPSRTPCryptoParamsType_MasterKey;
    cryptoMasterSaltTLV.type = kHAPHAPSRTPCryptoParamsType_MasterSalt;

    err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]){ &cryptoSuiteTLV, &cryptoMasterKeyTLV,
        &cryptoMasterSaltTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read video crypto suite tlv");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (cryptoSuiteTLV.value.numBytes != sizeof(accessoryConfiguration.state.controller_endpoints.crypto_suite) ||
        cryptoMasterKeyTLV.value.numBytes != sizeof(videoCryptoParams->master_key) ||
        cryptoMasterSaltTLV.value.numBytes != sizeof(videoCryptoParams->master_salt)) {
        HAPLogError(&kHAPLog_Default, "Video Crypto Suite, Master Key, or Master Salt not the correct size");
        return kHAPError_InvalidData;
    }
    // Check if crypto suite is supported
    if (*(const uint8_t*)cryptoSuiteTLV.value.bytes != accessoryConfiguration.state.supported_rtp_config.crypto_suite) {
        HAPLogError(&kHAPLog_Default, "Not a supported crypto suite");
        return kHAPError_InvalidData;
    }
    accessoryConfiguration.state.controller_endpoints.crypto_suite = *(const uint8_t*)cryptoSuiteTLV.value.bytes;
    memcpy(videoCryptoParams->master_key, cryptoMasterKeyTLV.value.bytes, cryptoMasterKeyTLV.value.numBytes);
    memcpy(videoCryptoParams->master_salt, cryptoMasterSaltTLV.value.bytes, cryptoMasterSaltTLV.value.numBytes);

    // AUDIO CRYPTO SUITE
    HAPTLVReaderCreate(&subReader, (void*)audioParamsTLV.value.bytes, audioParamsTLV.value.numBytes);
    typeof(accessoryConfiguration.state.controller_endpoints.audio_crypto_params) *audioCryptoParams =
        &accessoryConfiguration.state.controller_endpoints.audio_crypto_params;

    err = HAPTLVReaderGetAll(&subReader, (HAPTLV* const[]){ &cryptoSuiteTLV, &cryptoMasterKeyTLV,
            &cryptoMasterSaltTLV, NULL});
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to read audio crypto suite tlv");
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    if (cryptoSuiteTLV.value.numBytes != sizeof(accessoryConfiguration.state.controller_endpoints.crypto_suite) ||
        cryptoMasterKeyTLV.value.numBytes != sizeof(audioCryptoParams->master_key) ||
        cryptoMasterSaltTLV.value.numBytes != sizeof(audioCryptoParams->master_salt)) {
        HAPLogError(&kHAPLog_Default, "Video Crypto Suite, Master Key, or Master Salt not the correct size");

        return kHAPError_InvalidData;
    }
    if (*(const uint8_t*)cryptoSuiteTLV.value.bytes != accessoryConfiguration.state.supported_rtp_config.crypto_suite) {
        HAPLogError(&kHAPLog_Default, "Not a supported crypto suite");
        return kHAPError_InvalidData;
    }
    memcpy(audioCryptoParams->master_key, cryptoMasterKeyTLV.value.bytes, cryptoMasterKeyTLV.value.numBytes);
    memcpy(audioCryptoParams->master_salt, cryptoMasterSaltTLV.value.bytes, cryptoMasterSaltTLV.value.numBytes);

    // Successfully received controller endpoints
    // accessoryConfiguration.state.stream_session_state = CONTROLLER_ENDPOINTS_SENT;
    accessoryConfiguration.state.new_stream_session = request->session;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSupportedAudioStreamConfigurationRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED,
        HAPTLVWriterRef* responseWriter,
        void* _Nullable context HAP_UNUSED) {

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPTLVWriterRef subwriter;
    HAPTLVWriterRef subsubwriter;

    CreateNestedTLVWriter(responseWriter, &subwriter);
    const typeof(accessoryConfiguration.state.supported_audio_config.codec_config) *codec_config =
        &accessoryConfiguration.state.supported_audio_config.codec_config;
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPSupportedAudioCodecConfigurationType_Codec, codec_config->codec_type);

    CreateNestedTLVWriter(&subwriter, &subsubwriter);

    const typeof(codec_config->codec_parameters) *codecParams = &codec_config->codec_parameters;
    // --- Audio Codec Configuration
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_Channels, codecParams->channels);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_Bitrate, codecParams->bitrate);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_SampleRate, codecParams->samplerate);
    APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPAudioCodecParametersType_RTPTime, codecParams->rtp_time);

    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter,
        kHAPSupportedAudioCodecConfigurationType_CodecParameters);

    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter,
        kHAPSupportedAudioConfigurationType_AudioCodecConfiguration);

    APPEND_TLV_WRITER_VAL(responseWriter, kHAPSupportedAudioConfigurationType_ComfortNoiseSupport,
        accessoryConfiguration.state.supported_audio_config.comfort_noise_support);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSupportedRTPConfigurationRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED,
        HAPTLVWriterRef* responseWriter,
        void* _Nullable context HAP_UNUSED) {

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    APPEND_TLV_WRITER_VAL(responseWriter, kHAPSupportedRTPConfigurationType_CryptoSuite,
        accessoryConfiguration.state.supported_rtp_config.crypto_suite);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleSupportedVideoStreamConfigurationRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED,
        HAPTLVWriterRef* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPTLVWriterRef subwriter;
    HAPTLVWriterRef subsubwriter;

    CreateNestedTLVWriter(responseWriter, &subwriter);
    const typeof(accessoryConfiguration.state.supported_video_config.codec_config) *codecConfig =
        &accessoryConfiguration.state.supported_video_config.codec_config;
    APPEND_TLV_WRITER_VAL(&subwriter, kHAPVideoCodecConfigurationType_CodecType, codecConfig->codec_type);

    // --- Video Codec Parameters
    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    size_t length = sizeof(codecConfig->codec_parameters) / sizeof(VideoCodecParameters);
    HAPLogInfo(&kHAPLog_Default, "Video Codec Config Length: %zu", length);
    const VideoCodecParameters *codec_params = codecConfig->codec_parameters;
    for (size_t i = 0; i < length; i++) {
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_ProfileID, codec_params[i].profile_id);
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_Level, codec_params[i].level);
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_PacketizationMode,
            codec_params[i].packetization_mode);
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_CVOEnabled, codec_params[i].cvo_enabled);
        if (codec_params[i].cvo_enabled) {
            APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoCodecParametersType_CVOID, codec_params[i].cvo_id);
        }
        }
    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPVideoCodecConfigurationType_CodecParameters);


    CreateNestedTLVWriter(&subwriter, &subsubwriter);
    length = sizeof(codecConfig->attributes) / sizeof(Attributes);
    HAPLogInfo(&kHAPLog_Default, "Video Attributes Length: %zu", length);
    const Attributes *attributes = codecConfig->attributes;
    for (size_t i = 0; i < length; i++) {
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoAttributesType_ImageWidth, attributes[i].image_width);
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoAttributesType_ImageHeight, attributes[i].image_height);
        APPEND_TLV_WRITER_VAL(&subsubwriter, kHAPVideoAttributesType_FrameRate, attributes[i].frame_rate);

    }
    APPEND_TLV_WRITER_NESTED_TLV(&subwriter, &subsubwriter, kHAPVideoCodecConfigurationType_Attributes);


    APPEND_TLV_WRITER_NESTED_TLV(responseWriter, &subwriter,
        kHAPSupportedVideoConfigurationType_VideoCodecConfiguration);

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void AccessoryNotification(
        const HAPAccessory* accessory,
        const HAPService* service,
        const HAPCharacteristic* characteristic,
        void* ctx HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "Accessory Notification");

    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, accessory);
}

void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
}

void AppRelease(void) {
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}

//----------------------------------------------------------------------------------------------------------------------

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(!context);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            return;
        }
    }
    HAPFatalError();
}

const HAPAccessory* AppGetAccessoryInfo() {
    return &accessory;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions HAP_UNUSED,
        HAPPlatform* hapPlatform HAP_UNUSED,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks HAP_UNUSED) {

    pipeline = InitPipeline(&accessoryConfiguration.state.accessory_endpoints);
    if (!pipeline) {
        HAPLogInfo(&kHAPLog_Default, "GStreamer pipeline failed to initialize.");
    }
    /*no-op*/
}

void AppDeinitialize() {
    /*no-op*/
}
