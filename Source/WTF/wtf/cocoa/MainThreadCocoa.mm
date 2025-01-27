/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 2016 Telerik AD. All rights reserved. (as modified)
 */

#import "config.h"
#import <wtf/MainThread.h>

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/NSThread.h>
#import <dispatch/dispatch.h>
#import <stdio.h>
#import <wtf/Assertions.h>
#import <wtf/HashSet.h>
#import <wtf/RetainPtr.h>
#import <wtf/SchedulePair.h>
#import <wtf/Threading.h>

#if USE(WEB_THREAD)
#include <wtf/ios/WebCoreThread.h>
#endif

namespace WTF {

static CFRunLoopRef mainRunLoop;
static bool isTimerPosted; // This is only accessed on the main thread.
static bool mainThreadEstablishedAsPthreadMain { false };
static pthread_t mainThreadPthread { nullptr };
static NSThread* mainThreadNSThread { nullptr };

#if USE(WEB_THREAD)
static Thread* sApplicationUIThread;
static Thread* sWebThread;
#endif

void initializeMainThreadPlatform()
{
    ASSERT(!mainRunLoop);
    mainRunLoop = CFRunLoopGetMain();

#if !USE(WEB_THREAD)
    mainThreadEstablishedAsPthreadMain = false;
    mainThreadPthread = pthread_self();
    mainThreadNSThread = [NSThread currentThread];
#else
    mainThreadEstablishedAsPthreadMain = true;
    ASSERT(!mainThreadPthread);
    ASSERT(!mainThreadNSThread);
#endif
}

#if !USE(WEB_THREAD)
void initializeMainThreadToProcessMainThreadPlatform()
{
    if (!pthread_main_np())
        NSLog(@"WebKit Threading Violation - initial use of WebKit from a secondary thread.");

    ASSERT(!mainRunLoop);
    mainRunLoop = CFRunLoopGetMain();

    mainThreadEstablishedAsPthreadMain = true;
    mainThreadPthread = 0;
    mainThreadNSThread = nil;
}
#endif // !USE(WEB_THREAD)

static void timerFired(CFRunLoopTimerRef timer, void*)
{
    CFRelease(timer);
    isTimerPosted = false;

    @autoreleasepool {
        WTF::dispatchFunctionsFromMainThread();
    }
}

static void postTimer()
{
    ASSERT(isMainThread());

    if (isTimerPosted)
        return;

    isTimerPosted = true;
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), CFRunLoopTimerCreate(0, 0, 0, 0, 0, timerFired, 0), kCFRunLoopCommonModes);
}

void scheduleDispatchFunctionsOnMainThread()
{
    ASSERT(mainRunLoop);

    if (isWebThread()) {
        postTimer();
        return;
    }
    
    if (mainThreadEstablishedAsPthreadMain) {
        ASSERT(!mainThreadNSThread);
        CFRunLoopPerformBlock(mainRunLoop, kCFRunLoopDefaultMode, ^{
            WTF::dispatchFunctionsFromMainThread();
        });
        return;
    }

    ASSERT(mainThreadNSThread);
    CFRunLoopPerformBlock(mainRunLoop, kCFRunLoopDefaultMode, ^{
        WTF::dispatchFunctionsFromMainThread();
    });
}

void dispatchAsyncOnMainThreadWithWebThreadLockIfNeeded(void (^block)())
{
#if USE(WEB_THREAD)
    if (WebCoreWebThreadIsEnabled && WebCoreWebThreadIsEnabled()) {
        dispatch_async(dispatch_get_main_queue(), ^{
            WebCoreWebThreadLock();
            block();
        });
        return;
    }
#endif
    dispatch_async(dispatch_get_main_queue(), block);
}

void callOnWebThreadOrDispatchAsyncOnMainThread(void (^block)())
{
#if USE(WEB_THREAD)
    if (WebCoreWebThreadIsEnabled && WebCoreWebThreadIsEnabled()) {
        WebCoreWebThreadRun(block);
        return;
    }
#endif
    dispatch_async(dispatch_get_main_queue(), block);
}

#if USE(WEB_THREAD)
static bool webThreadIsUninitializedOrLockedOrDisabled()
{
    return !WebCoreWebThreadIsLockedOrDisabled || WebCoreWebThreadIsLockedOrDisabled();
}

bool isMainThread()
{
    return (isWebThread() || pthread_main_np()) && webThreadIsUninitializedOrLockedOrDisabled();
}

bool isMainThreadIfInitialized()
{
    return isMainThread();
}

bool isUIThread()
{
    return pthread_main_np();
}

// Keep in mind that isWebThread can be called even when destroying the current thread.
bool isWebThread()
{
    return pthread_equal(pthread_self(), mainThreadPthread);
}

void initializeApplicationUIThread()
{
    ASSERT(pthread_main_np());
    sApplicationUIThread = &Thread::current();
}

void initializeWebThreadPlatform()
{
    ASSERT(!pthread_main_np());

    mainThreadEstablishedAsPthreadMain = false;
    mainThreadPthread = pthread_self();
    mainThreadNSThread = [NSThread currentThread];

    sWebThread = &Thread::current();
}

bool canAccessThreadLocalDataForThread(Thread& thread)
{
    Thread& currentThread = Thread::current();
    if (&thread == &currentThread)
        return true;

    if (&thread == sWebThread || &thread == sApplicationUIThread)
        return (&currentThread == sWebThread || &currentThread == sApplicationUIThread) && webThreadIsUninitializedOrLockedOrDisabled();

    return false;
}
#else
bool isMainThread()
{
    if (mainThreadEstablishedAsPthreadMain) {
        ASSERT(!mainThreadPthread);
        return pthread_main_np();
    }

    ASSERT(mainThreadPthread);
    return pthread_equal(pthread_self(), mainThreadPthread);
}

bool isMainThreadIfInitialized()
{
    if (mainThreadEstablishedAsPthreadMain)
        return pthread_main_np();
    return pthread_equal(pthread_self(), mainThreadPthread);
}

#endif // USE(WEB_THREAD)

} // namespace WTF
