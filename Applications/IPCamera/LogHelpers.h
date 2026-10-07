//
// Created by william on 2026-10-07.
//

#ifndef HOMEKITADK_LOGHELPERS_H
#define HOMEKITADK_LOGHELPERS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

void BufferToString(char * str, int strLength, const uint8_t * buffer, int bufferLength);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif //HOMEKITADK_LOGHELPERS_H
