
//
//  CallSite.cpp
//  JavaScriptCore
//
//  Created by Vasil Trifonov on 5.07.19.
//

#include "CallSite.h"
#include "JSString.h"

namespace JSC {
    
    CallSite::CallSite(ExecState* exec, Structure* structure, const StackFrame* stackFrame)
        : Base(exec->vm(), structure)
        , m_stackFrame(stackFrame)
    {

    }
    
    void CallSite::finishCreation(ExecState* exec) {
        auto& vm = exec->vm();
        Base::finishCreation(vm);
        // Implement the CallSite methods https://v8.dev/docs/stack-trace-api
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getThis"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getTypeName"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getFunction"), 0, &CallSite::getFunction, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getFunctionName"), 0, &CallSite::getFunctionName, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getMethodName"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getFileName"), 0, &CallSite::getFileName, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "toString"), 0, &CallSite::toString, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getLineNumber"), 0, &CallSite::getLineNumber, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getColumnNumber"), 0, &CallSite::getColumnNumber, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getEvalOrigin"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "isToplevel"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "isEval"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "isNative"), 0, &CallSite::isNative, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "isConstructor"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "isAsync"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "isPromiseAll"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
        this->putDirectNativeFunction(vm, exec->lexicalGlobalObject(), Identifier::fromString(exec, "getPromiseIndex"), 0, &CallSite::getUndefined, NoIntrinsic, static_cast<unsigned>(PropertyAttribute::DontEnum));
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::getFunctionName(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        auto functionName = callSite->m_stackFrame->functionName(vm);
        return JSValue::encode(jsString(&vm, functionName));
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::getFunction(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        auto callee = callSite->m_stackFrame->callee();
        return JSValue::encode(callee);
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::getUndefined(ExecState* execState) {
        VM& vm = execState->vm();
        return JSValue::encode(jsUndefined());
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::getFileName(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        auto sourceUrl = callSite->m_stackFrame->sourceURL();
        return JSValue::encode(jsString(&vm, sourceUrl));
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::isNative(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        auto sourceUrl = callSite->m_stackFrame->sourceURL();
        return JSValue::encode(jsBoolean(sourceUrl == "[native code]"));
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::toString(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        auto stringValue = callSite->m_stackFrame->toString(vm);
        return JSValue::encode(jsString(&vm, stringValue));
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::getLineNumber(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        unsigned lineNumber;
        unsigned unusedColumn;
        callSite->m_stackFrame->computeLineAndColumn(lineNumber, unusedColumn);
        return JSValue::encode(JSValue(lineNumber));
    }
    
    EncodedJSValue JSC_HOST_CALL CallSite::getColumnNumber(ExecState* execState) {
        VM& vm = execState->vm();
        auto callSite = jsCast<CallSite*>(execState->thisValue());
        unsigned unusedLineNumber;
        unsigned column;
        callSite->m_stackFrame->computeLineAndColumn(unusedLineNumber, column);
        return JSValue::encode(JSValue(column));
    }
    
} // namespace JSC
