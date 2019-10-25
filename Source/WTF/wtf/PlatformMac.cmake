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
    WeakObjCPtr.h

    cf/CFURLExtras.h
    cf/TypeCastsCF.h

    cocoa/Entitlements.h
    cocoa/NSURLExtras.h
    cocoa/SoftLinking.h

    darwin/WeakLinking.h

    mac/AppKitCompatibilityDeclarations.h

    spi/cf/CFBundleSPI.h
    spi/cf/CFStringSPI.h

    spi/cocoa/CFXPCBridgeSPI.h
    spi/cocoa/SecuritySPI.h
    spi/cocoa/objcSPI.h

    spi/darwin/DataVaultSPI.h
    spi/darwin/SandboxSPI.h
    spi/darwin/XPCSPI.h
    spi/darwin/dyldSPI.h

    spi/mac/MetadataSPI.h

    text/ASCIILiteral.h
    text/cf/TextBreakIteratorCF.h
)

list(APPEND WTF_SOURCES
    BlockObjCExceptions.mm

    cf/CFURLExtras.cpp
    cf/FileSystemCF.cpp
    cf/LanguageCF.cpp
    cf/RunLoopCF.cpp
    cf/RunLoopTimerCF.cpp
    cf/SchedulePairCF.cpp
    cf/URLCF.cpp

    cocoa/AutodrainedPool.cpp
    cocoa/CPUTimeCocoa.cpp
    cocoa/Entitlements.mm
    cocoa/FileSystemCocoa.mm
    cocoa/MachSendRight.cpp
    cocoa/MainThreadCocoa.mm
    cocoa/MemoryFootprintCocoa.cpp
    cocoa/MemoryPressureHandlerCocoa.mm
    cocoa/NSURLExtras.mm
    cocoa/URLCocoa.mm
    cocoa/WorkQueueCocoa.cpp

    mac/DeprecatedSymbolsUsedBySafari.mm
    mac/FileSystemMac.mm
    mac/SchedulePairMac.mm

    posix/FileSystemPOSIX.cpp
    posix/OSAllocatorPOSIX.cpp
    posix/ThreadingPOSIX.cpp

    ios/WebCoreThread.cpp

    text/cf/AtomStringImplCF.cpp
    text/cf/StringCF.cpp
    text/cf/StringImplCF.cpp
    text/cf/StringViewCF.cpp

    text/cocoa/StringCocoa.mm
    text/cocoa/StringImplCocoa.mm
    text/cocoa/StringViewCocoa.mm
    text/cocoa/TextBreakIteratorInternalICUCocoa.cpp
)

# TODO Check if we need to remove this
#list(APPEND WTF_PRIVATE_INCLUDE_DIRECTORIES
#    ${WTF_DERIVED_SOURCES_DIR}
#)
#
#file(COPY mac/MachExceptions.defs DESTINATION ${WTF_DERIVED_SOURCES_DIR})
#file(COPY "${WTF_DIR}/icu/unicode" DESTINATION ${WTF_DERIVED_SOURCES_DIR})

add_custom_command(
    OUTPUT
        ${WTF_DERIVED_SOURCES_DIR}/MachExceptionsServer.h
        ${WTF_DERIVED_SOURCES_DIR}/mach_exc.h
        ${WTF_DERIVED_SOURCES_DIR}/mach_excServer.c
        ${WTF_DERIVED_SOURCES_DIR}/mach_excUser.c
    MAIN_DEPENDENCY mac/MachExceptions.defs
    WORKING_DIRECTORY ${WTF_DERIVED_SOURCES_DIR}
    # hardcode /usr/bin because Xcode 10.2's toolchain doesn't contain mac_exc.defs
    COMMAND env -i /usr/bin/mig -sheader MachExceptionsServer.h MachExceptions.defs
    VERBATIM)
list(APPEND WTF_SOURCES
    ${WTF_DERIVED_SOURCES_DIR}/mach_excServer.c
    ${WTF_DERIVED_SOURCES_DIR}/mach_excUser.c
)
# /TODO Check if we need to remove this

WEBKIT_CREATE_FORWARDING_HEADERS(WebKitLegacy DIRECTORIES ${WebKitLegacy_FORWARDING_HEADERS_DIRECTORIES} FILES ${WebKitLegacy_FORWARDING_HEADERS_FILES})
WEBKIT_CREATE_FORWARDING_HEADERS(WebKit DIRECTORIES ${FORWARDING_HEADERS_DIR}/WebKitLegacy)
