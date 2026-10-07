// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// This file contains the accessory attribute database that defines the accessory information service, HAP Protocol
// Information Service, the Pairing service and finally the service signature exposed by the light bulb.

#include "App.h"
#include "DB.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_AccessoryInformation                 ((uint64_t) 0x0001)
#define kIID_AccessoryInformationIdentify         ((uint64_t) 0x0002)
#define kIID_AccessoryInformationManufacturer     ((uint64_t) 0x0003)
#define kIID_AccessoryInformationModel            ((uint64_t) 0x0004)
#define kIID_AccessoryInformationName             ((uint64_t) 0x0005)
#define kIID_AccessoryInformationSerialNumber     ((uint64_t) 0x0006)
#define kIID_AccessoryInformationFirmwareRevision ((uint64_t) 0x0007)
#define kIID_AccessoryInformationHardwareRevision ((uint64_t) 0x0008)
#define kIID_AccessoryInformationADKVersion       ((uint64_t) 0x0009)
#define kIID_AccessoryInformationProductData      ((uint64_t) 0x000A)

#define kIID_HAPProtocolInformation                 ((uint64_t) 0x0010)
#define kIID_HAPProtocolInformationServiceSignature ((uint64_t) 0x0011)
#define kIID_HAPProtocolInformationVersion          ((uint64_t) 0x0012)

#define kIID_RTPStream                     ((uint64_t) 0x0020)
#define kIID_RTPStreamStatus               ((uint64_t) 0x0021)
#define kIID_RTPStreamSelectedRTPConfig    ((uint64_t) 0x0022)
#define kIID_RTPStreamSetupEndpoints       ((uint64_t) 0x0023)
#define kIID_RTPStreamSupportedAudioConfig ((uint64_t) 0x0024)
#define kIID_RTPStreamSupportedRTPConfig   ((uint64_t) 0x0025)
#define kIID_RTPStreamSupportedVideoConfig ((uint64_t) 0x0026)

#define kIID_Microphone     ((uint64_t) 0x0030)
#define kIID_MicrophoneMute ((uint64_t) 0x0031)

// #define kIID_LightBulb                 ((uint64_t) 0x0030)
// #define kIID_LightBulbServiceSignature ((uint64_t) 0x0031)
// #define kIID_LightBulbName             ((uint64_t) 0x0032)
// #define kIID_LightBulbOn               ((uint64_t) 0x0033)




HAP_STATIC_ASSERT(kAttributeCount == 9 + 3 + 7 + 2, AttributeCount_mismatch);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_AccessoryInformationIdentify,
    .characteristicType = &kHAPCharacteristicType_Identify,
    .debugDescription = kHAPCharacteristicDebugDescription_Identify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = NULL, .handleWrite = HAPHandleAccessoryInformationIdentifyWrite }
};

const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationManufacturer,
    .characteristicType = &kHAPCharacteristicType_Manufacturer,
    .debugDescription = kHAPCharacteristicDebugDescription_Manufacturer,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationManufacturerRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationModelCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationModel,
    .characteristicType = &kHAPCharacteristicType_Model,
    .debugDescription = kHAPCharacteristicDebugDescription_Model,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationModelRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationNameRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationSerialNumber,
    .characteristicType = &kHAPCharacteristicType_SerialNumber,
    .debugDescription = kHAPCharacteristicDebugDescription_SerialNumber,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationSerialNumberRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationFirmwareRevision,
    .characteristicType = &kHAPCharacteristicType_FirmwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationFirmwareRevisionRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationHardwareRevision,
    .characteristicType = &kHAPCharacteristicType_HardwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_HardwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationHardwareRevisionRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationADKVersion,
    .characteristicType = &kHAPCharacteristicType_ADKVersion,
    .debugDescription = kHAPCharacteristicDebugDescription_ADKVersion,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationADKVersionRead, .handleWrite = NULL }
};

const HAPService accessoryInformationService = {
    .iid = kIID_AccessoryInformation,
    .serviceType = &kHAPServiceType_AccessoryInformation,
    .debugDescription = kHAPServiceDebugDescription_AccessoryInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &accessoryInformationIdentifyCharacteristic,
                                                            &accessoryInformationManufacturerCharacteristic,
                                                            &accessoryInformationModelCharacteristic,
                                                            &accessoryInformationNameCharacteristic,
                                                            &accessoryInformationSerialNumberCharacteristic,
                                                            &accessoryInformationFirmwareRevisionCharacteristic,
                                                            &accessoryInformationHardwareRevisionCharacteristic,
                                                            &accessoryInformationADKVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPDataCharacteristic hapProtocolInformationServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HAPProtocolInformationServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic hapProtocolInformationVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HAPProtocolInformationVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleHAPProtocolInformationVersionRead, .handleWrite = NULL }
};

const HAPService hapProtocolInformationService = {
    .iid = kIID_HAPProtocolInformation,
    .serviceType = &kHAPServiceType_HAPProtocolInformation,
    .debugDescription = kHAPServiceDebugDescription_HAPProtocolInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = true } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &hapProtocolInformationServiceSignatureCharacteristic,
                                                            &hapProtocolInformationVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

// /**
//  * The 'Name' characteristic of the Light Bulb service.
//  */
// static const HAPStringCharacteristic ipCameraNameCharacteristic = {
//     .format = kHAPCharacteristicFormat_String,
//     .iid = kIID_LightBulbName,
//     .characteristicType = &kHAPCharacteristicType_Name,
//     .debugDescription = kHAPCharacteristicDebugDescription_Name,
//     .manufacturerDescription = NULL,
//     .properties = { .readable = true,
//                     .writable = false,
//                     .supportsEventNotification = false,
//                     .hidden = false,
//                     .requiresTimedWrite = false,
//                     .supportsAuthorizationData = false,
//                     .ip = { .controlPoint = false, .supportsWriteResponse = false },
//                     .ble = { .supportsBroadcastNotification = false,
//                              .supportsDisconnectedNotification = false,
//                              .readableWithoutSecurity = false,
//                              .writableWithoutSecurity = false } },
//     .constraints = { .maxLength = 64 },
//     .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
// };

/**
 * The 'Mute' characteristic of the Microphone service.
 */
const HAPBoolCharacteristic microphoneMuteCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_MicrophoneMute,
    .characteristicType = &kHAPCharacteristicType_Mute,
    .debugDescription = kHAPCharacteristicDebugDescription_Mute,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HandleMicrophoneMuteRead, .handleWrite = HandleMicrophoneMuteWrite }
};

/**
 * The Light Bulb service that contains the 'On' characteristic.
 */
// const HAPService lightBulbService = {
//     .iid = kIID_LightBulb,
//     .serviceType = &kHAPServiceType_LightBulb,
//     .debugDescription = kHAPServiceDebugDescription_LightBulb,
//     .name = "Light Bulb",
//     .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
//     .linkedServices = NULL,
//     .characteristics = (const HAPCharacteristic* const[]) { &lightBulbServiceSignatureCharacteristic,
//                                                             &lightBulbNameCharacteristic,
//                                                             &lightBulbOnCharacteristic,
//                                                             NULL }
// };

const HAPTLV8Characteristic streamingStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_RTPStreamStatus,
    .characteristicType = &kHAPCharacteristicType_StreamingStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_StreamingStatus,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = { .controlPoint = false, .supportsWriteResponse = false },
    },
    .callbacks = { .handleRead = HandleStreamingStatusRead, .handleWrite = NULL}
};

const HAPTLV8Characteristic selectedRTPStreamConfigCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_RTPStreamSelectedRTPConfig,
    .characteristicType = &kHAPCharacteristicType_SelectedRTPStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SelectedRTPStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = { .controlPoint = false, .supportsWriteResponse = false },
    },
    .callbacks = { .handleRead = HandleSelectedRTPStreamConfigRead, .handleWrite = HandleSelectedRTPStreamConfigWrite}
};

const HAPTLV8Characteristic setupEndpointsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_RTPStreamSetupEndpoints,
    .characteristicType = &kHAPCharacteristicType_SetupEndpoints,
    .debugDescription = kHAPCharacteristicDebugDescription_SetupEndpoints,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = { .controlPoint = false, .supportsWriteResponse = false },
    },
    .callbacks = { .handleRead = HandleSetupEndpointsRead, .handleWrite = HandleSetupEndpointsWrite}
};

const HAPTLV8Characteristic supportedAudioStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_RTPStreamSupportedAudioConfig,
    .characteristicType = &kHAPCharacteristicType_SupportedAudioStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedAudioStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = { .controlPoint = false, .supportsWriteResponse = false },
    },
    .callbacks = { .handleRead = HandleSupportedAudioStreamConfigurationRead, .handleWrite = NULL}
};

const HAPTLV8Characteristic supportedRTPConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_RTPStreamSupportedRTPConfig,
    .characteristicType = &kHAPCharacteristicType_SupportedRTPConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedRTPConfiguration,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = { .controlPoint = false, .supportsWriteResponse = false },
    },
    .callbacks = { .handleRead = HandleSupportedRTPConfigurationRead, .handleWrite = NULL}
};

const HAPTLV8Characteristic supportedVideoStreamConfigurationCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_RTPStreamSupportedVideoConfig,
    .characteristicType = &kHAPCharacteristicType_SupportedVideoStreamConfiguration,
    .debugDescription = kHAPCharacteristicDebugDescription_SupportedVideoStreamConfiguration,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = false,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = { .controlPoint = false, .supportsWriteResponse = false },
    },
    .callbacks = { .handleRead = HandleSupportedVideoStreamConfigurationRead, .handleWrite = NULL}
};

const HAPService rtpStreamManagementService = {
    .iid = kIID_RTPStream,
    .serviceType = &kHAPServiceType_CameraRTPStreamManagement,
    .debugDescription = kHAPServiceDebugDescription_CameraRTPStreamManagement,
    .name = "Stream Manager",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &streamingStatusCharacteristic,
                                                            &selectedRTPStreamConfigCharacteristic,
                                                            &setupEndpointsCharacteristic,
                                                            &supportedAudioStreamConfigurationCharacteristic,
                                                            &supportedRTPConfigurationCharacteristic,
                                                            &supportedVideoStreamConfigurationCharacteristic,
                                                            NULL }
};

const HAPService microphoneService = {
    .iid = kIID_Microphone,
    .serviceType = &kHAPServiceType_Microphone,
    .debugDescription = kHAPServiceDebugDescription_Microphone,
    .name = "Microphone",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &microphoneMuteCharacteristic,
                                                            NULL }
};