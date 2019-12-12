add_definitions(-DBPLATFORM_IOS=1)

list(APPEND bmalloc_SOURCES
    bmalloc/ProcessCheck.mm
)

list(APPEND bmalloc_PUBLIC_HEADERS
    bmalloc/darwin/BSoftLinking.h
    bmalloc/darwin/MemoryStatusSPI.h
)
