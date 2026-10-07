#include "LogHelpers.h"
#include "HAP.h"

void BufferToString(char * str, int strLength, const uint8_t * buffer, int bufferLength) {
    int pos = 0;
    for (int i = 0; i < bufferLength; i++) {
        // sprintf(temp, "%u ", buffer[i]);
        int addedLength = snprintf(&str[pos], strLength - pos, "%u ", buffer[i]);
        // addedLength = strlen(temp);
        if (addedLength < 0 || pos + addedLength >= strLength) {
            HAPLogError(&kHAPLog_Default, "String buffer is not big enough");
            if (strLength > 0) {
                str[0] = 0;
            }
            return;
        }
        pos = pos + addedLength;
    }
    if (strLength > 0 && pos < strLength) {
        str[pos] = 0;
    }
}