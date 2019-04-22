#include "UnormNormalize.h"

#if PLATFORM(IOS) && !USE(APPLE_INTERNAL_SDK)

#include <CoreFoundation/CFString.h>
#include <wtf/RetainPtr.h>

U_STABLE int32_t U_EXPORT2
ios_specific_unorm2_normalize(const UNormalizer2 *norm2, const UChar *source, int32_t sourceLength, UChar *resultBuffer, int32_t resultLength, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return 0;
    }

    if ( (source == nullptr ? sourceLength != 0 : sourceLength < -1) ||
        (resultBuffer == nullptr ? resultLength != 0 : resultLength < 0) ||
        (source == resultBuffer && source != nullptr)
        ) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    WTF::RetainPtr<CFMutableStringRef> normalizedString = WTF::adoptCF(CFStringCreateMutable(kCFAllocatorDefault, 0));
    CFStringAppendCharacters(normalizedString.get(), source, sourceLength);
    if (norm2 == nullptr) {
        norm2 = reinterpret_cast<UNormalizer2*>(kCFStringNormalizationFormC);
    }
    CFStringNormalizationForm normalizationForm = (CFStringNormalizationForm)(reinterpret_cast<long>(norm2) - 1);

    CFStringNormalize(normalizedString.get(), normalizationForm);

    CFIndex normalizedLength = CFStringGetLength(normalizedString.get());
    if (normalizedLength > resultLength) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    if (resultLength > 0 && resultBuffer != nullptr) {
        CFStringGetCharacters(normalizedString.get(), CFRangeMake(0, resultLength), resultBuffer);
    }
    return normalizedLength;
}

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFCInstance(UErrorCode */*pErrorCode*/){
    return reinterpret_cast<UNormalizer2*>(kCFStringNormalizationFormC + 1);
}

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFDInstance(UErrorCode */*pErrorCode*/){
    return reinterpret_cast<UNormalizer2*>(kCFStringNormalizationFormD + 1);
}

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFKCInstance(UErrorCode */*pErrorCode*/){
    return reinterpret_cast<UNormalizer2*>(kCFStringNormalizationFormKC + 1);
}

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFKDInstance(UErrorCode */*pErrorCode*/){
    return reinterpret_cast<UNormalizer2*>(kCFStringNormalizationFormKD + 1);
}

#endif
