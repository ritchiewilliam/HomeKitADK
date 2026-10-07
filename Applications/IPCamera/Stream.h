//
// Created by william on 2026-08-24.
//

#ifndef HOMEKITADK_STREAM_H
#define HOMEKITADK_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif
#include <gst/gst.h>
#include "HAP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

typedef struct {
    uint8_t profile_id; // Type: 1, Length: 1
    uint8_t level; // Type: 2, Length: 1
    uint8_t packetization_mode; // Type: 3, Length: 1
    uint8_t cvo_enabled; // Type: 4, Length: 1
    uint8_t cvo_id; // Type: 5, Length: 1
} VideoCodecParameters;

typedef struct {
    uint16_t image_width; // Type: 1, Length: 2
    uint16_t image_height; // Type: 2, Length: 2
    uint8_t frame_rate; // Type: 3, Length: 1
} Attributes;

//SELECTED RTP STREAM CONFIGURATION - Section 9.91
typedef struct {
    uint8_t session_id[16];
    // Session Control - Table 9-8, Type: 1
    // struct {
    //     // uint8_t session_id[16]; // Type: 1, Length: 16
    uint8_t command; // Type: 2, Length: 1
    // } session_control;

    // Selected Video Parameters - Table 9-9, Type: 2
    struct {
        uint8_t codec_type; // Type: 1, Length: 1

        // Video Codec Parameters - Table 9-27, Type: 2
        VideoCodecParameters codec_parameters;

        // Video Attributes - Table 9-28, Type: 3
        Attributes attributes;

        // Selected Video RTP Parameters - Table 9-10, Type: 4
        struct {
            uint8_t payload_type; // Type: 1, Length: 1
            uint32_t sync_source; // Type: 2, Length: 4
            uint16_t max_bitrate; // Type: 3, Length: 2
            float min_rtcp_interval; // Type: 4, Length: 4
            uint16_t max_mtu; // Type: 5, Length: 2
        } rtp_parameters;

    } selected_video_parameters;

    // Selected Audio Parameters - Table 9-11, Type: 3
    struct {
        uint8_t codec_type; // Type: 1, Length: 1

        // Selected Audio Codec parameters - Table 9-21, Type: 2
        struct {
            uint8_t channels; // Type: 1, Length: 1
            uint8_t bitrate; // Type: 2, Length: 1
            uint8_t samplerate; // Type: 3, Length: 1
            uint8_t rtp_time; // Type: 4, Length: 1
        } codec_parameters;

        // Selected Audio RTP parameters - Table 9-12, Type: 3
        struct {
            uint8_t payload_type; // Type: 1, Length: 1
            uint32_t sync_source; // Type: 2, Length: 4
            uint16_t max_bitrate; // Type: 3, Length: 2
            float min_rtcp_interval; // Type: 4, Length: 4
            uint8_t comfort_noise_payload_type; // Type: 6, Length: 1
        } rtp_parameters;

        bool comfort_noise; // Type: 4, Length: 1

    } selected_audio_parameters;

} SelectedRTPStream;

// SETUP ENDPOINTS - Section 9.92
typedef struct {
    uint8_t session_id[16]; // Type: 1, Length: 16
    uint8_t crypto_suite;
    // Controller Address - Table: 9-14, Type: 3
    struct {
        uint8_t ip_version; // Type: 1, Length: 1
        char ip_address[16]; // Type: 2, Length : N
        uint16_t video_rtp_port; // Type: 3, Length: 2
        uint16_t audio_rtp_port; // Type: 4, Length: 2
    } address;

    // struct {
    //     uint8_t ip_version; // Type: 1, Length: 1
    //     char ip_address[16]; // Type: 2, Length : N
    //     uint16_t video_rtp_port; // Type: 3, Length: 2
    //     uint16_t audio_rtp_port; // Type: 4, Length: 2
    // } accessory_address;


    // SRTP Parameters for Video & Audio - Table 9-15, Type: 4
    struct {
        // uint8_t crypto_suite; // Type: 1, Length: 1
        uint8_t master_key[16]; // Type: 2, Length: 16 or (32)
        uint8_t master_salt[14]; // Type: 3, Length: 14
    } video_crypto_params;

    // SRTP Parameters for Audio - Table 9-15, Type: 5
    struct {
        // uint8_t crypto_suite; // Type: 1, Length: 1
        uint8_t master_key[16]; // Type: 2, Length: 16 or (32)
        uint8_t master_salt[14]; // Type: 3, Length: 14
    } audio_crypto_params;

    uint32_t video_ssrc;
    uint32_t audio_ssrc;

} Endpoints;

// SUPPORTED AUDIO STREAM CONFIGURATION - Section 9.102
typedef struct {
    // Audio Codec Configuration - Table 9-19, Type: 1
    struct {
        uint16_t codec_type; // Type: 1, Length: 2

        // Audio Codec Parameters - Table 9-21, Type: 2
        struct {
            uint8_t channels; // Type: 1, Length: 1
            uint8_t bitrate; // Type: 2, Length: 1
            uint8_t samplerate; // Type: 3, Length: 1
            uint8_t rtp_time; // Type: 4, Length: 1
        } codec_parameters;

    } codec_config;

    bool comfort_noise_support; // Type: 2, Length: 1

} SupportedAudioConfig;

// SUPPORTED RTP CONFIGURATION - Section 9.104
typedef struct {
    uint8_t crypto_suite; // Type 2, Length: 1
} SupportedRTPConfig;

// SUPPORTED VIDEO STREAM CONFIGURATION - Section 9.105
typedef struct {

    // Video Codec Configuration - Table 9-26, Type: 1
    struct {
        uint8_t codec_type;

        // Video Codec Parameters - Table 9-27, Type: 2
        // struct {
        //     uint8_t profile_id; // Type: 1, Length: 1
        //     uint8_t level; // Type: 2, Length: 1
        //     uint8_t packetization_mode; // Type: 3, Length: 1
        //     uint8_t cvo_enabled; // Type: 4, Length: 1
        //     uint8_t cvo_id; // Type: 5, Length: 1
        // } codec_parameters;
        VideoCodecParameters codec_parameters[2];

        // Video Attributes - Table 9-28, Type: 3
        Attributes attributes[4];

    } codec_config;

} SupportedVideoConfig;

extern const SupportedAudioConfig audioConfig;
extern const SupportedRTPConfig rtpConfig;
extern const SupportedVideoConfig videoConfig;
extern const Endpoints globalAccessoryEndpoints;

GstElement * _Nullable InitPipeline(const Endpoints *accessoryEndpoints);

void ConfigurePipeline(const Endpoints *controllerEndpoints, const Endpoints *accessoryEndpoints, const SelectedRTPStream* config);

void ReconfigurePipeline(const SelectedRTPStream* config);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif //HOMEKITADK_STREAM_H
