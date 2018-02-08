//
//  UnormNormalize.h
//  WTF

#ifndef UnormNormalize_h
#define UnormNormalize_h

#include <wtf/ExportMacros.h>
#include <unicode/unorm.h>

#if PLATFORM(IOS) && !USE(APPLE_INTERNAL_SDK)

U_STABLE int32_t U_EXPORT2
ios_specific_unorm2_normalize(const UNormalizer2 *norm2, const UChar *src,
                         int32_t length, UChar *dest, int32_t capacity,
                              UErrorCode *pErrorCode);

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFCInstance(UErrorCode *pErrorCode);

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFDInstance(UErrorCode *pErrorCode);

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFKInstance(UErrorCode *pErrorCode);

U_STABLE const UNormalizer2 * U_EXPORT2
ios_specific_unorm2_getNFKDInstance(UErrorCode *pErrorCode);

#endif

#endif /* UnormNormalize_h */
