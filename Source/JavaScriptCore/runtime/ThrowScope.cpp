/*
 * Copyright (C) 2016-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThrowScope.h"

#include "Exception.h"
#include "JSCInlines.h"
#include "VM.h"

namespace JSC {

#if ENABLE(EXCEPTION_SCOPE_VERIFICATION)

void ThrowScope::throwException(ExecState* exec, Exception* exception)
{
#ifndef NDEBUG
    if (m_vm.exception() && m_vm.exception() != exception)
        m_vm.verifyExceptionCheckNeedIsSatisfied(m_recursionDepth, m_location);
#endif // NDEBUG

    m_vm.throwException(exec, exception);
}

JSValue ThrowScope::throwException(ExecState* exec, JSValue error)
{
#ifndef NDEBUG
    if (!error.isCell() || !jsDynamicCast<Exception*>(m_vm, error.asCell()))
        m_vm.verifyExceptionCheckNeedIsSatisfied(m_recursionDepth, m_location);
#endif // NDEBUG

    return m_vm.throwException(exec, error);
}

JSObject* ThrowScope::throwException(ExecState* exec, JSObject* obj)
{
#ifndef NDEBUG
    if (!jsDynamicCast<Exception*>(m_vm, obj))
        m_vm.verifyExceptionCheckNeedIsSatisfied(m_recursionDepth, m_location);
#endif // NDEBUG

    return m_vm.throwException(exec, obj);
}

#ifndef NDEBUG
void ThrowScope::simulateThrow()
{
    RELEASE_ASSERT(m_vm.m_topExceptionScope);
    m_vm.m_simulatedThrowPointLocation = m_location;
    m_vm.m_simulatedThrowPointRecursionDepth = m_recursionDepth;
    m_vm.m_needExceptionCheck = true;
    if (UNLIKELY(Options::dumpSimulatedThrows()))
        m_vm.m_nativeStackTraceOfLastSimulatedThrow = StackTrace::captureStackTrace(Options::unexpectedExceptionStackTraceLimit());
}
#endif // NDEBUG

#endif // ENABLE(EXCEPTION_SCOPE_VERIFICATION)

} // namespace JSC
