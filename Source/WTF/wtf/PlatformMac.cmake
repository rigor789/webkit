set(WTF_LIBRARY_TYPE STATIC)

find_library(COCOA_LIBRARY Cocoa)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(FOUNDATION_LIBRARY Foundation)
find_library(SECURITY_LIBRARY Security)
list(APPEND WTF_LIBRARIES
    ${COREFOUNDATION_LIBRARY}
    ${FOUNDATION_LIBRARY}
    ${READLINE_LIBRARY}
)

list(APPEND WTF_PUBLIC_HEADERS
    cf/TypeCastsCF.h

    cocoa/Entitlements.h
    MachSendRight.h
    cocoa/SoftLinking.h

    darwin/WeakLinking.h

    mac/AppKitCompatibilityDeclarations.h

    spi/cf/CFBundleSPI.h
    spi/cf/CFStringSPI.h

    spi/cocoa/FoundationSPI.h
    spi/cocoa/NSMapTableSPI.h
    spi/cocoa/SecuritySPI.h

    spi/darwin/SandboxSPI.h
    spi/darwin/XPCSPI.h
    spi/darwin/dyldSPI.h

    text/ASCIILiteral.h
    text/cf/TextBreakIteratorCF.h
)

list(APPEND WTF_SOURCES
    BlockObjCExceptions.mm
    RunLoopTimerCF.cpp
    SchedulePairCF.cpp
    SchedulePairMac.mm

    cf/LanguageCF.cpp
    cf/RunLoopCF.cpp

    cocoa/AutodrainedPool.cpp
    cocoa/CPUTimeCocoa.cpp
    cocoa/Entitlements.mm
    cocoa/MachSendRight.cpp
    cocoa/MemoryFootprintCocoa.cpp
    cocoa/MemoryPressureHandlerCocoa.mm
    cocoa/WorkQueueCocoa.cpp

    mac/DeprecatedSymbolsUsedBySafari.mm
    mac/MainThreadMac.mm

    ios/WebCoreThread.cpp

    text/cf/AtomicStringImplCF.cpp
    text/cf/StringCF.cpp
    text/cf/StringImplCF.cpp
    text/cf/StringViewCF.cpp

    text/cocoa/StringImplCocoa.mm
    text/cocoa/StringViewCocoa.mm
    text/cocoa/TextBreakIteratorInternalICUCocoa.cpp
)

list(APPEND WTF_PRIVATE_INCLUDE_DIRECTORIES
    "${WTF_DIR}/icu"
    ${DERIVED_SOURCES_WTF_DIR}
)

WEBKIT_CREATE_FORWARDING_HEADERS(WebKitLegacy DIRECTORIES ${WebKitLegacy_FORWARDING_HEADERS_DIRECTORIES} FILES ${WebKitLegacy_FORWARDING_HEADERS_FILES})
WEBKIT_CREATE_FORWARDING_HEADERS(WebKit DIRECTORIES ${FORWARDING_HEADERS_DIR}/WebKitLegacy)
