//
// Created by william on 2026-08-07.
//

#ifndef HOMEKITADK_TLVHELPERS_H
#define HOMEKITADK_TLVHELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"


#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#define STRINGIFY(x) #x

#define APPEND_TLV_WRITER_VAL(writer_ptr, tag, val) \
    do { \
        HAPLogInfo(&kHAPLog_Default, "%s: Appending %s: %d", __func__, STRINGIFY(val), (int)val); \
        HAPError _err = AppendTLVWriterBuffer(writer_ptr, tag, &val, sizeof(val)); \
        if (_err) { \
            HAPLogError(&kHAPLog_Default, "%s: Out of resources when writing: %s", __func__, STRINGIFY(val)); \
            HAPAssert((_err) == kHAPError_OutOfResources); \
        return (_err); \
        } \
    } while (0)

#define APPEND_TLV_WRITER_ARR(writer_ptr, tag, arr) \
    do { \
        HAPLogInfo(&kHAPLog_Default, "%s: Appending %s", __func__, STRINGIFY(arr)); \
        HAPError _err = AppendTLVWriterBuffer(writer_ptr, tag, arr, sizeof(arr)); \
        if (_err) { \
            HAPLogError(&kHAPLog_Default, "%s: Out of resources when writing: %s", __func__, STRINGIFY(arr)); \
            HAPAssert((_err) == kHAPError_OutOfResources); \
        return (_err); \
        } \
    } while (0)

#define APPEND_TLV_WRITER_ARR_LOG(writer_ptr, tag, arr, bufferLog) \
    do { \
        HAPLogInfo(&kHAPLog_Default, "%s: Appending %s", __func__, STRINGIFY(arr)); \
        if(bufferLog) { \
            BufferToString(bufferLog, sizeof(bufferLog), arr, sizeof(arr)); \
            HAPLogInfo(&kHAPLog_Default, "%s", bufferLog); \
        } \
        HAPError _err = AppendTLVWriterBuffer(writer_ptr, tag, arr, sizeof(arr)); \
        if (_err) { \
            HAPLogError(&kHAPLog_Default, "%s: Out of resources when writing: %s", __func__, STRINGIFY(arr)); \
            HAPAssert((_err) == kHAPError_OutOfResources); \
        return (_err); \
        } \
    } while (0)

#define APPEND_TLV_WRITER_NESTED_TLV(writer_ptr, subwriter_ptr, tag) \
    do { \
        HAPLogInfo(&kHAPLog_Default, "%s: Appending %s %s", __func__, STRINGIFY(subwriter_ptr), STRINGIFY(tag)); \
        HAPError _err = AppendTLVWriterNestedTLV(writer_ptr, subwriter_ptr, tag); \
        if (_err) { \
        HAPLogError(&kHAPLog_Default, "%s: Out of resources appending %s: %s", __func__, STRINGIFY(subwriter_ptr), STRINGIFY(tag)); \
        HAPAssert((_err) == kHAPError_OutOfResources); \
        return (_err); \
        } \
    } while (0)

static void CreateNestedTLVWriter(HAPTLVWriterRef* writer, HAPTLVWriterRef* subwriter) {
    void * bytes;
    size_t maxBytes;
    // Get scratch bytes from original writer
    HAPTLVWriterGetScratchBytes(writer, &bytes, &maxBytes);
    // Use scratch bytes for sub writer
    HAPTLVWriterCreate(subwriter, bytes, maxBytes);
}

static HAPError AppendTLVWriterBuffer(HAPTLVWriterRef* writer, uint8_t tag, const void* buffer, size_t length) {
    return HAPTLVWriterAppend(writer, &(const HAPTLV) {
        .type = tag,
        .value = { .bytes = (void*)buffer, .numBytes = length }
    });
}

static HAPError AppendTLVWriterNestedTLV(HAPTLVWriterRef* writer, HAPTLVWriterRef* subwriter, uint8_t tag) {
    void * bytes;
    size_t maxBytes;
    // Obtain nested writer contents
    HAPTLVWriterGetBuffer(subwriter, &bytes, &maxBytes);

    // Write entire nested TLV as value of new outer TLV
    return AppendTLVWriterBuffer(writer, tag, bytes, maxBytes);
}


#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif //HOMEKITADK_TLVHELPERS_H

