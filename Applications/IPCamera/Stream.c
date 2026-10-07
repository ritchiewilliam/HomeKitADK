//
// Created by william on 2026-08-24.
//

#include "Stream.h"
#include <stdio.h>
#include <gst/gst.h>


typedef struct _VideoData
{
    GstElement *source;
    GstElement *demux;
    GstElement *parse;
    GstElement *rate;
    GstElement *ratecaps;
    GstElement *caps;
    GstElement *decode;
    GstElement *scale;
    GstElement *download;
    GstElement *convert;
    GstElement *encode;
    GstElement *payload;
} VideoData;

typedef struct _AudioData
{
    GstElement *source;
    GstElement *demux;
    GstElement *decode;
    GstElement *identity;
    GstElement *convert;
    GstElement *resample;
    GstElement *encode;
    GstElement *payload;
} AudioData;

typedef struct _NetworkData
{
    GstElement *rtp;
    GstElement *src_srtp;
    GstElement *src_srtpcaps;
    GstElement *sink_srtp;
    GstElement *funnel;
    GstElement *src_udp; // RTCP feedback
    GstElement *sink_udp; // RTP sent
    // GstElement *sink_rtcp_udp; // RTCP sent
} NetworkData;

typedef struct _HandlerParams {
    GstElement *sink;
    char *type;
} HandlerParams;


const SupportedAudioConfig audioConfig = {
    .codec_config = {
        .codec_type = kHAPTLVValue_AudioCodecType_Opus,
        .codec_parameters = {
            .channels = 1,
            .bitrate = kHAPTLVValue_AudioCodecBitrate_Variable,
            .samplerate = kHAPTLVValue_AudioCodecSampleRate_16KHZ,
            .rtp_time = 20, // In miliseconds (default for VOIP)
        },
    },
    .comfort_noise_support = false,
};

const SupportedRTPConfig rtpConfig = {
    .crypto_suite = kHAPTLVValue_SRTPCryptoSuite_AES128
};

const SupportedVideoConfig videoConfig = {
    .codec_config = {
        .codec_type = kHAPTLVValue_VideoCodecType_H264,
        .codec_parameters = {
            {
                .profile_id = kHAPTLVValue_VideoCodecProfileID_Baseline, // Set on INIT
                .level = kHAPTLVValue_VideoCodecLevel_31, // Set on INIT
                .packetization_mode = 0, // Set on INIT
                .cvo_enabled = 0, // No setting as disabled
                .cvo_id = 0 // no setting as disabled
            }, {
                .profile_id = kHAPTLVValue_VideoCodecProfileID_Main,
                .level = kHAPTLVValue_VideoCodecLevel_32,
                .packetization_mode = 0,
                .cvo_enabled = 0,
                .cvo_id = 0
            }

        },
        //
        .attributes = {
            {
                .image_width = 1280,
                .image_height = 720,
                .frame_rate = 30
            },
            {
                .image_width = 640,
                .image_height = 360,
                .frame_rate = 30
            },
            {
                .image_width = 480,
                .image_height = 270, // Fixed to standard 16:9
                .frame_rate = 30
            },
            {
                .image_width = 320,
                .image_height = 180, // Recommended for Apple Watch / PiP
                .frame_rate = 30
            }
        }
    }
};

const Endpoints globalAccessoryEndpoints = {
    .crypto_suite = kHAPTLVValue_SRTPCryptoSuite_AES128,
    .address = {
        .ip_version = kHAPTLVValue_IPAddressVersion_IPV4,
        .ip_address =  "192.168.0.112",
        .video_rtp_port = 5000,
        .audio_rtp_port = 5001,
    }
};

// Correspond to values in: HAPTLVValue_VideoCodecProfileID
static const char profileID[][21] = {"constrained-baseline", "main", "high"};

// Correspond to values in: HAPTLVValue_VideoCodecLevel
static const char level[][4] = {"3.1", "3.2", "4"};

// Correspond to values in: HAPTLVValue_AudioCodecSampleRate
static const int sampleRate[] = {8000, 16000, 24000};
static const int bandwidth[] = {1101, 1103, 1104};

// Correspond to values in: HAPTLVValue_SRTPCryptoSuite
static const unsigned int cipherType[] = {1, 2, 0};
static const unsigned int authType[] = {2, 2, 0};
static const char cipherTypeCaps[][12] = {"aes-128-icm", "aes-256-icm", "null"};
static const char authTypeCaps[][13] = {"hmac-sha1-80", "hmac-sha1-80", "null"};

static VideoData video;
static AudioData audio;
static NetworkData network;
static NetworkData networkAudio;

static HandlerParams source_params = {NULL, "multipart/x-mixed-replace"};
static HandlerParams demux_params = {NULL, "image/jpeg"};
static HandlerParams demux_audio_params = {NULL, "audio/x-opus"};

/* This function will be called by the pad-added signal */
static void
pad_added_handler (GstElement * src, GstPad * new_pad, HandlerParams * data)
{
    GstPad *sink_pad = gst_element_get_static_pad (data->sink, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;
    HAPLogInfo(&kHAPLog_Default, "PAD HANDLER CALL");

    HAPLogInfo(&kHAPLog_Default, "Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
        HAPLogInfo(&kHAPLog_Default, "We are already linked. Ignoring.");
        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);

    if (!new_pad_caps) {
        new_pad_caps = gst_pad_query_caps(new_pad, NULL);
    }

    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, data->type) || data->type == NULL) {
        HAPLogInfo(&kHAPLog_Default, "It has type '%s' which is not raw video. Ignoring.", new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
        HAPLogInfo(&kHAPLog_Default, "Type is '%s' but link failed.", new_pad_type);
    } else {
        HAPLogInfo(&kHAPLog_Default, "Link succeeded (type '%s') to %s.", new_pad_type, GST_ELEMENT_NAME(data->sink));
    }

    exit:
      /* Unreference the new pad's caps, if we got them */
      if (new_pad_caps != NULL)
          gst_caps_unref (new_pad_caps);

    /* Unreference the sink pad */
    gst_object_unref (sink_pad);
}

static void setup_source_timestamp(GstElement *urisourcebin HAP_UNUSED, GstElement *source, gpointer user_data HAP_UNUSED) {
  if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "do-timestamp")) {
    g_object_set(source, "do-timestamp", TRUE, NULL);
  }
  if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "is-live")) {
    g_object_set(source, "is-live", TRUE, NULL);
  }
}

GstBuffer * createKeyBuffer(const uint8_t *masterKey, size_t keySize, const uint8_t *masterSalt, size_t saltSize) {
    GstBuffer *buffer = gst_buffer_new_allocate(NULL, keySize + saltSize, NULL);
    if (!buffer) {
        HAPLogError(&kHAPLog_Default, "%s: Failed to allocate key buffer", __func__);
        return buffer;
    }
    gst_buffer_fill(buffer, 0, masterKey, keySize);
    gst_buffer_fill(buffer, keySize, masterSalt, saltSize);
    return buffer;
}

GstElement * _Nullable InitPipeline(const Endpoints *accessoryEndpoints) {
    gst_init(NULL, NULL);

    HAPLogInfo(&kHAPLog_Default, "Initializing video elements");
    video.source = gst_element_factory_make ("urisourcebin", "source"); // Fetches the data stream
    video.demux  = gst_element_factory_make ("multipartdemux", "demux");    // Parses the MJPEG frames
    video.parse  = gst_element_factory_make ("jpegparse", "parser");    // Parses the MJPEG frames
    video.rate = gst_element_factory_make("videorate", "rate"); // Maintains constant framerate
    video.ratecaps = gst_element_factory_make("capsfilter", "ratefilter");
    video.decode = gst_element_factory_make ("nvjpegdec", "gpu_dec");   // Hardware MJPEG decoder
    video.scale  = gst_element_factory_make ("cudaconvertscale", "scale"); // Hardware scaler/converter
    video.caps = gst_element_factory_make("capsfilter", "filter");
    video.encode = gst_element_factory_make ("nvh264enc", "encode");    // Hardware encoder
    video.payload = gst_element_factory_make ("rtph264pay", "payload");

    HAPLogInfo(&kHAPLog_Default, "Initializing audio elements");
    audio.source = gst_element_factory_make("filesrc", "source_audio");
    audio.demux = gst_element_factory_make("oggdemux", "demux_audio");
    audio.decode = gst_element_factory_make("opusdec", "decode_audio");
    audio.identity = gst_element_factory_make("identity", "identity");
    audio.convert = gst_element_factory_make("audioconvert", "convert_audio");
    audio.resample = gst_element_factory_make("audioresample", "resample_audio");
    audio.encode = gst_element_factory_make("opusenc", "encode_audio");
    audio.payload = gst_element_factory_make("rtpopuspay", "payload_audio");

    HAPLogInfo(&kHAPLog_Default, "Initializing video network elements");
    network.rtp = gst_element_factory_make ("rtpbin", "rtp");
    network.sink_srtp = gst_element_factory_make("srtpenc", "srtpsink");
    network.src_srtp = gst_element_factory_make("srtpdec", "srtpsrc");
    network.src_srtpcaps = gst_element_factory_make("capsfilter", "srtpsrcfilter");
    network.funnel = gst_element_factory_make("funnel", "funnel");
    network.sink_udp = gst_element_factory_make("udpsink", "udpsink");
    network.src_udp = gst_element_factory_make("udpsrc", "udpsrc");

    HAPLogInfo(&kHAPLog_Default, "Initializing audio network elements");
    networkAudio.rtp = network.rtp;
    networkAudio.sink_srtp = gst_element_factory_make("srtpenc", "srtpsink_audio");
    networkAudio.src_srtp = gst_element_factory_make("srtpdec", "srtpsrc_audio");
    networkAudio.src_srtpcaps = gst_element_factory_make("capsfilter", "srtpsrcfilter_audio");
    networkAudio.funnel = gst_element_factory_make("funnel", "funnel_audio");
    networkAudio.sink_udp = gst_element_factory_make("udpsink", "udpsink_audio");
    networkAudio.src_udp = gst_element_factory_make("udpsrc", "udpsrc_audio");

    HAPLogInfo(&kHAPLog_Default, "Creating pipeline");
    GstElement* pipeline = gst_pipeline_new("camera-pipeline");

    if (!pipeline || !video.source || !video.demux || !video.parse || !video.rate || !video.ratecaps || !video.decode ||
        !video.scale || !video.caps || !video.encode || !video.payload ||
        !audio.source || !audio.demux || !audio.decode || !audio.identity || !audio.convert || !audio.resample ||
        !audio.encode || !audio.payload ||
        !network.rtp || !network.sink_srtp || !network.src_srtp || !network.src_srtpcaps|| !network.funnel ||
        !network.sink_udp || !network.src_udp ||
        !networkAudio.sink_srtp || !networkAudio.src_srtp || !networkAudio.src_srtpcaps || !networkAudio.funnel ||
        !networkAudio.sink_udp || !networkAudio.src_udp) {
        HAPLogError(&kHAPLog_Default, "Not all elements could be created.");
        return NULL;
    }

    GstCaps *scale_caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, (int)videoConfig.codec_config.attributes[0].image_width,
        "height", G_TYPE_INT, (int)videoConfig.codec_config.attributes[0].image_height,
        "format", G_TYPE_STRING, "NV12",
        NULL);
    gst_caps_set_features(
        scale_caps,
        0,
        gst_caps_features_new("memory:CUDAMemory", NULL)
    );
    g_object_set(video.caps, "caps", scale_caps, NULL);
    gst_caps_unref(scale_caps);

    // The minimum keyframe interval shall be 5 seconds ( 150 frames / 30 fps = 5 seconds)
    g_object_set(video.encode, "gop-size", 150, NULL);

    // SPS PPS must use STAP-A fragmentation -
    g_object_set(video.payload, "aggregate-mode", 1, "config-interval", 4, NULL);

    // SET THIS WHEN SELECTED RTP STREAM CONFIG COMES IN
    // g_object_set(audio.payload, "pt", 97, NULL);

    // RTP Profile  GST_RTP_PROFILE_AVPF (3) – the Audio/Visual profile with feedback (RFC 4585)
    g_object_set(network.rtp, "rtp-profile", 3, NULL);

    /* Set the URI to play */
    g_object_set (audio.source, "location", "mariokart.opus", NULL);

    g_object_set(audio.identity, "sync", TRUE, NULL);
    g_object_set(audio.encode, "bandwidth", bandwidth[audioConfig.codec_config.codec_parameters.samplerate], NULL);


    g_object_set(network.src_udp, "port", accessoryEndpoints->address.video_rtp_port, NULL);
    g_object_set(networkAudio.src_udp,  "port", accessoryEndpoints->address.audio_rtp_port, NULL);

    gst_bin_add_many (GST_BIN (pipeline), video.source, video.demux, video.parse, video.rate, video.ratecaps,
        video.decode, video.scale, video.caps, /*video.download, video.convert,*/ video.encode, video.payload,
        audio.source, audio.demux, audio.decode, audio.identity, audio.convert, audio.resample, audio.encode, audio.payload,
        network.rtp, network.sink_srtp, network.src_srtp, network.src_srtpcaps, network.funnel, network.sink_udp, /*network.sink_rtcp_udp,*/ network.src_udp,
        networkAudio.sink_srtp, networkAudio.src_srtp, networkAudio.src_srtpcaps, networkAudio.funnel, networkAudio.sink_udp, networkAudio.src_udp, NULL);

    GstCaps *jpeg_caps = gst_caps_from_string ("image/jpeg");
    gboolean link_parse_rate = gst_element_link_filtered(video.parse, video.rate, jpeg_caps);
    gst_caps_unref(jpeg_caps);

    GstCaps *rate_caps = gst_caps_new_simple("image/jpeg",
        "framerate", GST_TYPE_FRACTION, (int)videoConfig.codec_config.attributes[0].frame_rate, 1, NULL);
    // gboolean link_rate_decode = gst_element_link_filtered(video.rate, video.encode, rate_caps);
    g_object_set(video.ratecaps, "caps", rate_caps, NULL);
    gst_caps_unref(rate_caps);

    gboolean link_rest = gst_element_link_many(video.rate, video.ratecaps, video.decode, video.scale, video.caps, video.encode, NULL);

    GstCaps *encode_caps = gst_caps_new_simple("video/x-h264",
        "profile", G_TYPE_STRING, profileID[videoConfig.codec_config.codec_parameters[0].profile_id],
        "level", G_TYPE_STRING, level[videoConfig.codec_config.codec_parameters[0].level],
        NULL);
    gboolean link_encode_payload = gst_element_link_filtered(video.encode, video.payload, encode_caps);
    gst_caps_unref(encode_caps);

    if (!link_parse_rate || !link_rest || !link_encode_payload) {
        HAPLogError(&kHAPLog_Default, "Video elements could not be linked.\n");
        gst_object_unref (pipeline);
        return NULL;
    }

    GstPad *rtpSink0 HAP_UNUSED = gst_element_request_pad_simple(network.rtp, "send_rtp_sink_0");


    // Packetization mode always 1
    GstCaps *rtp_caps = gst_caps_from_string (
      "application/x-rtp, "
      "media=(string)video, "
      "encoding-name=(string)H264, "
      "packetization-mode=(int)1"
    );
    // Link should cause trigger of previous callback to link rtpbin with funnel
    gst_element_link_pads_filtered(video.payload, "src", network.rtp, "send_rtp_sink_0",
    rtp_caps);

    gst_pad_link(gst_element_get_static_pad(network.rtp, "send_rtp_src_0"), gst_element_request_pad_simple(network.sink_srtp, "rtp_sink_0"));
    gst_pad_link(gst_element_request_pad_simple(network.rtp, "send_rtcp_src_0"), gst_element_request_pad_simple(network.sink_srtp, "rtcp_sink_0"));
    gst_pad_link(gst_element_get_static_pad(network.sink_srtp, "rtp_src_0"), gst_element_request_pad_simple(network.funnel, "sink_0"));
    gst_pad_link(gst_element_get_static_pad(network.sink_srtp, "rtcp_src_0"), gst_element_request_pad_simple(network.funnel, "sink_1"));
    gst_pad_link(gst_element_get_static_pad(network.funnel, "src"), gst_element_get_static_pad(network.sink_udp, "sink"));

    gst_pad_link(gst_element_get_static_pad(network.src_udp, "src"), gst_element_get_static_pad(network.src_srtp, "rtcp_sink"));

    // VIDEO RTCP INPUT
    gst_element_link(network.src_srtp, network.src_srtpcaps);
    gst_pad_link(gst_element_get_static_pad(network.src_srtpcaps, "src"),
        gst_element_request_pad_simple(network.rtp, "recv_rtcp_sink_0"));

    // AUDIO LINKING
    GstCaps *resample_caps = gst_caps_new_simple("audio/x-raw",
        "rate", G_TYPE_INT, sampleRate[audioConfig.codec_config.codec_parameters.samplerate],
        "channels", G_TYPE_INT, (int)audioConfig.codec_config.codec_parameters.channels, NULL);
    gst_element_link_many(audio.decode, audio.identity, audio.convert, audio.resample, NULL);
    gst_element_link_filtered(audio.resample, audio.encode, resample_caps);
    gst_caps_unref(resample_caps);
    gst_element_link(audio.encode, audio.payload);

    gst_pad_link(gst_element_get_static_pad(audio.payload, "src"), gst_element_request_pad_simple(networkAudio.rtp, "send_rtp_sink_1"));
    gst_pad_link(gst_element_get_static_pad(networkAudio.rtp, "send_rtp_src_1"), gst_element_request_pad_simple(networkAudio.sink_srtp, "rtp_sink_0"));
    gst_pad_link(gst_element_get_static_pad(networkAudio.sink_srtp, "rtp_src_0"), gst_element_request_pad_simple(networkAudio.funnel, "sink_0"));
    gst_pad_link(gst_element_request_pad_simple(networkAudio.rtp, "send_rtcp_src_1"), gst_element_request_pad_simple(networkAudio.sink_srtp, "rtcp_sink_0"));
    gst_pad_link(gst_element_get_static_pad(networkAudio.sink_srtp, "rtcp_src_0"), gst_element_request_pad_simple(networkAudio.funnel, "sink_1"));
    gst_pad_link(gst_element_get_static_pad(networkAudio.funnel, "src"), gst_element_get_static_pad(networkAudio.sink_udp, "sink"));

    gst_element_link(networkAudio.src_udp, networkAudio.src_srtp);

    gst_element_link(networkAudio.src_srtp, networkAudio.src_srtpcaps);
    gst_pad_link(gst_element_get_static_pad(networkAudio.src_srtpcaps, "src"),
        gst_element_request_pad_simple(networkAudio.rtp, "recv_rtcp_sink_1"));

    g_signal_connect(video.source, "source-setup", G_CALLBACK(setup_source_timestamp), NULL);

    /* Set the URI to play */
    g_object_set (video.source, "uri",
        "http://192.168.0.23:8080/stream.mjpg",
        NULL);

    source_params.sink = video.demux;
    demux_params.sink = video.parse;
    demux_audio_params.sink = audio.payload;

    /* Connect to the pad-added signal */
    g_signal_connect (video.source, "pad-added", G_CALLBACK (pad_added_handler),
        &source_params);

    g_signal_connect (video.demux, "pad-added", G_CALLBACK (pad_added_handler),
        &demux_params);

    // g_signal_connect (audio.source, "pad-added", G_CALLBACK (pad_added_handler),
    //     &source_audio_params);
    g_signal_connect (audio.demux, "pad-added", G_CALLBACK (pad_added_handler),
      &demux_audio_params);

    gboolean link_aud_source_demux = gst_element_link(audio.source, audio.demux);

    if (!link_aud_source_demux) {
        HAPLogError(&kHAPLog_Default, "Audio elements could not be linked.\n");
        gst_object_unref (pipeline);
        return NULL;
    }

    HAPLogInfo(&kHAPLog_Default, "Finished creating pipeline");

    return pipeline;
}

void ConfigurePipeline(const Endpoints *controllerEndpoints, const Endpoints *accessoryEndpoints, const SelectedRTPStream* config) {

    GstBuffer *controllerVideoKeyBuffer = createKeyBuffer(controllerEndpoints->video_crypto_params.master_key,
        sizeof(controllerEndpoints->video_crypto_params.master_key),
        controllerEndpoints->video_crypto_params.master_salt,
        sizeof(controllerEndpoints->video_crypto_params.master_salt));
    GstBuffer *controllerAudioKeyBuffer = createKeyBuffer(controllerEndpoints->audio_crypto_params.master_key,
        sizeof(controllerEndpoints->audio_crypto_params.master_key),
        controllerEndpoints->audio_crypto_params.master_salt,
        sizeof(controllerEndpoints->audio_crypto_params.master_salt));
    GstBuffer *accessoryVideoKeyBuffer = createKeyBuffer(accessoryEndpoints->video_crypto_params.master_key,
        sizeof(accessoryEndpoints->video_crypto_params.master_key),
        accessoryEndpoints->video_crypto_params.master_salt,
        sizeof(accessoryEndpoints->video_crypto_params.master_salt));
    GstBuffer *accessoryAudioKeyBuffer = createKeyBuffer(accessoryEndpoints->audio_crypto_params.master_key,
        sizeof(accessoryEndpoints->audio_crypto_params.master_key),
        accessoryEndpoints->audio_crypto_params.master_salt,
        sizeof(accessoryEndpoints->audio_crypto_params.master_salt));

    g_object_set(network.sink_srtp, "key", accessoryVideoKeyBuffer,
        "rtcp-auth", authType[accessoryEndpoints->crypto_suite],
        "rtcp-cipher", cipherType[accessoryEndpoints->crypto_suite],
        "rtp-auth", authType[accessoryEndpoints->crypto_suite],
        "rtp-cipher", cipherType[accessoryEndpoints->crypto_suite], NULL);
    g_object_set(networkAudio.sink_srtp, "key", accessoryAudioKeyBuffer,
        "rtcp-auth", authType[accessoryEndpoints->crypto_suite],
        "rtcp-cipher", cipherType[accessoryEndpoints->crypto_suite],
      "rtp-auth", authType[accessoryEndpoints->crypto_suite],
      "rtp-cipher", cipherType[accessoryEndpoints->crypto_suite], NULL);

    GstCaps *srtpdeccaps = gst_caps_new_simple("application/x-srtcp",
        "ssrc", G_TYPE_UINT, accessoryEndpoints->video_ssrc,
        "srtp-key", GST_TYPE_BUFFER, controllerVideoKeyBuffer,
        "srtp-cipher", G_TYPE_STRING, cipherTypeCaps[controllerEndpoints->crypto_suite],
        "srtp-auth", G_TYPE_STRING, authTypeCaps[controllerEndpoints->crypto_suite],
        "srtcp-cipher", G_TYPE_STRING, cipherTypeCaps[controllerEndpoints->crypto_suite],
        "srtcp-auth", G_TYPE_STRING, authTypeCaps[controllerEndpoints->crypto_suite],
        NULL);
    g_object_set(network.src_srtpcaps, "caps", srtpdeccaps, NULL);
    gst_caps_unref(srtpdeccaps);
    GstCaps *srtpdeccaps_audio = gst_caps_new_simple("application/x-srtcp",
        "ssrc", G_TYPE_UINT, accessoryEndpoints->audio_ssrc,
        "srtp-key", GST_TYPE_BUFFER, controllerAudioKeyBuffer,
        "srtp-cipher", G_TYPE_STRING, cipherTypeCaps[controllerEndpoints->crypto_suite],
        "srtp-auth", G_TYPE_STRING, authTypeCaps[controllerEndpoints->crypto_suite],
        "srtcp-cipher", G_TYPE_STRING, cipherTypeCaps[controllerEndpoints->crypto_suite],
        "srtcp-auth", G_TYPE_STRING, authTypeCaps[controllerEndpoints->crypto_suite],
        NULL);
    g_object_set(networkAudio.src_srtpcaps, "caps", srtpdeccaps_audio, NULL);
    gst_caps_unref(srtpdeccaps_audio);

    gst_buffer_unref(controllerVideoKeyBuffer);
    gst_buffer_unref(controllerAudioKeyBuffer);
    gst_buffer_unref(accessoryVideoKeyBuffer);
    gst_buffer_unref(accessoryAudioKeyBuffer);

    g_object_set(network.sink_udp, "sync", FALSE, "async", FALSE, "host", controllerEndpoints->address.ip_address,
        "port", controllerEndpoints->address.video_rtp_port, NULL);
    g_object_set(networkAudio.sink_udp, "sync", FALSE, "async", FALSE, "host", controllerEndpoints->address.ip_address,
        "port", controllerEndpoints->address.audio_rtp_port, NULL);

    // Profile and Level do not need to be redone as only one option.

    // Payload Types and bitrates
    g_object_set(video.encode, "bitrate", (int)config->selected_video_parameters.rtp_parameters.max_bitrate, NULL);
    g_object_set(video.payload, "pt", (int)config->selected_video_parameters.rtp_parameters.payload_type,
        "mtu", (int)config->selected_video_parameters.rtp_parameters.max_mtu, NULL);

    g_object_set(audio.encode, "bitrate", (int)config->selected_audio_parameters.rtp_parameters.max_bitrate * 1000, NULL);
    g_object_set(audio.payload, "pt", (int)config->selected_audio_parameters.rtp_parameters.payload_type, NULL);

    // Min RTCP Interval
    GstElement *session;
    g_signal_emit_by_name(network.rtp, "get-internal-session", 0, &session);
    g_object_set(session, "internal-ssrc", accessoryEndpoints->video_ssrc, NULL);
    g_object_set(session, "rtcp-min-interval",
        (long)(config->selected_video_parameters.rtp_parameters.min_rtcp_interval * 1000000000.0), NULL);
    g_object_unref(session);
    g_signal_emit_by_name(network.rtp, "get-internal-session", 1, &session);
    g_object_set(session, "internal-ssrc", accessoryEndpoints->audio_ssrc, NULL);
    g_object_set(session, "rtcp-min-interval",
        (long)(config->selected_audio_parameters.rtp_parameters.min_rtcp_interval * 1000000000.0), NULL);
    g_object_unref(session);

}

void ReconfigurePipeline(const SelectedRTPStream* config) {

    // Bitrates
    g_object_set(video.encode, "bitrate", (int)config->selected_video_parameters.rtp_parameters.max_bitrate, NULL);

    g_object_set(audio.encode, "bitrate", (int)config->selected_audio_parameters.rtp_parameters.max_bitrate, NULL);

    // Min RTCP Interval
    GstElement *session;
    g_signal_emit_by_name(network.rtp, "get-internal-session", 0, &session);
    g_object_set(session, "rtcp-min-interval",
        (long)(config->selected_video_parameters.rtp_parameters.min_rtcp_interval * 1000000000.0), NULL);
    g_object_unref(session);
    g_signal_emit_by_name(network.rtp, "get-internal-session", 1, &session);
    g_object_set(session, "rtcp-min-interval",
        (long)(config->selected_audio_parameters.rtp_parameters.min_rtcp_interval * 1000000000.0), NULL);
    g_object_unref(session);

    // Caps for dynamic scaling
    GstCaps *scale_caps = gst_caps_new_simple("video/x-raw",
    "format", G_TYPE_STRING, "NV12",
            "width", G_TYPE_INT, (int)config->selected_video_parameters.attributes.image_width,
            "height", G_TYPE_INT, (int)config->selected_video_parameters.attributes.image_height,
            NULL);  // GstCaps *scale_caps = gst_caps_from_string ("video/x-raw, width=1280, height=720, format=NV12");
    gst_caps_set_features(
      scale_caps,
      0,
      gst_caps_features_new("memory:CUDAMemory", NULL)
    );
    g_object_set(video.caps, "caps", scale_caps, NULL);
    gst_caps_unref(scale_caps);

    GstCaps *rate_caps = gst_caps_new_simple("image/jpeg",
        "framerate", GST_TYPE_FRACTION, (int)config->selected_video_parameters.attributes.frame_rate, 1, NULL);
    // gboolean link_rate_decode = gst_element_link_filtered(video.rate, video.encode, rate_caps);
    g_object_set(video.ratecaps, "caps", rate_caps, NULL);
    gst_caps_unref(rate_caps);

}