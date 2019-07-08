#ifndef CallSite_h
#define CallSite_h

#include "config.h"
#include "JSDestructibleObject.h"
#include "StackFrame.h"

namespace JSC {

    class CallSite : public JSDestructibleObject {
    public:
        typedef JSDestructibleObject Base;
        CallSite(ExecState*, Structure*, const StackFrame*);
        static EncodedJSValue getFunctionName(ExecState*);
        static EncodedJSValue getFunction(ExecState*);
        static EncodedJSValue getUndefined(ExecState*);
        static EncodedJSValue getFileName(ExecState*);
        static EncodedJSValue toString(ExecState*);
        static EncodedJSValue getLineNumber(ExecState*);
        static EncodedJSValue getColumnNumber(ExecState*);
        static EncodedJSValue isNative(ExecState*);
        
        
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
}


#endif /* CallSite_h */
