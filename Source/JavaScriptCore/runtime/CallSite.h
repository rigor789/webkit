#pragma once

#include "JSDestructibleObject.h"
#include "StackFrame.h"

namespace JSC {

class CallSite : public JSDestructibleObject {
public:
    typedef JSDestructibleObject Base;
    CallSite(ExecState*, Structure*, const StackFrame*);
    static JSC_HOST_CALL EncodedJSValue getFunctionName(ExecState*);
    static JSC_HOST_CALL EncodedJSValue getFunction(ExecState*);
    static JSC_HOST_CALL EncodedJSValue getUndefined(ExecState*);
    static JSC_HOST_CALL EncodedJSValue getFileName(ExecState*);
    static JSC_HOST_CALL EncodedJSValue toString(ExecState*);
    static JSC_HOST_CALL EncodedJSValue getLineNumber(ExecState*);
    static JSC_HOST_CALL EncodedJSValue getColumnNumber(ExecState*);
    static JSC_HOST_CALL EncodedJSValue isNative(ExecState*);

    static CallSite* create(ExecState* exec, Structure* structure, const StackFrame* stackFrame) {
        CallSite* callSite = new (NotNull, allocateCell<CallSite>(exec->vm().heap)) CallSite(exec, structure, stackFrame);
        callSite->finishCreation(exec);
        return callSite;
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

    void finishCreation(ExecState*);
private:
    const StackFrame* m_stackFrame;
};
} // namespace JSC
