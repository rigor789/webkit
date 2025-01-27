/*
 * Copyright (C) 2015-2016 Apple Inc. All rights reserved.
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
#include "ModuleAnalyzer.h"

#include "ConsoleClient.h"
#include "JSCInlines.h"
#include "JSGlobalObject.h"
#include "JSModuleRecord.h"
#include "ModuleScopeData.h"
#include "ScriptArguments.h"
#include "StrongInlines.h"

namespace JSC {


ModuleAnalyzer::ModuleAnalyzer(ExecState* exec, const Identifier& moduleKey, const SourceCode& sourceCode, const VariableEnvironment& declaredVariables, const VariableEnvironment& lexicalVariables)
    : m_vm(&exec->vm())
    , m_exec(exec)
    , m_moduleRecord(*m_vm, JSModuleRecord::create(exec, *m_vm, exec->lexicalGlobalObject()->moduleRecordStructure(), moduleKey, sourceCode, declaredVariables, lexicalVariables))
{
}

void ModuleAnalyzer::exportVariable(ModuleProgramNode& moduleProgramNode, const RefPtr<UniquedStringImpl>& localName, const VariableEnvironmentEntry& variable)
{
    // In the parser, we already marked the variables as Exported and Imported.
    // By leveraging this information, we collect the information that is needed
    // to construct the module environment.
    //
    // I E
    //   * = exported module local variable
    // *   = imported binding
    //     = non-exported module local variable
    // * * = indirect exported binding
    //
    // One exception is namespace binding (like import * as ns from "mod").
    // This is annotated as an imported, but the actual binding is locate in the
    // current module.

    if (!variable.isExported())
        return;

    // Exported module local variable.
    if (!variable.isImported()) {
        for (auto& exportName : moduleProgramNode.moduleScopeData().exportedBindings().get(localName.get()))
            moduleRecord()->addExportEntry(JSModuleRecord::ExportEntry::createLocal(Identifier::fromUid(m_vm, exportName.get()), Identifier::fromUid(m_vm, localName.get())));
        return;
    }

    if (variable.isImportedNamespace()) {
        // Exported namespace binding.
        // import * as namespace from "mod"
        // export { namespace }
        //
        // Sec 15.2.1.16.1 step 11-a-ii-2-b https://tc39.github.io/ecma262/#sec-parsemodule
        // Namespace export is handled as local export since a namespace object binding itself is implemented as a local binding.
        for (auto& exportName : moduleProgramNode.moduleScopeData().exportedBindings().get(localName.get()))
            moduleRecord()->addExportEntry(JSModuleRecord::ExportEntry::createLocal(Identifier::fromUid(m_vm, exportName.get()), Identifier::fromUid(m_vm, localName.get())));
        return;
    }

    // Indirectly exported binding.
    // import a from "mod"
    // export { a }
    Optional<JSModuleRecord::ImportEntry> optionalImportEntry = moduleRecord()->tryGetImportEntry(localName.get());
    ASSERT(optionalImportEntry);
    const JSModuleRecord::ImportEntry& importEntry = *optionalImportEntry;
    for (auto& exportName : moduleProgramNode.moduleScopeData().exportedBindings().get(localName.get()))
        moduleRecord()->addExportEntry(JSModuleRecord::ExportEntry::createIndirect(Identifier::fromUid(m_vm, exportName.get()), importEntry.importName, importEntry.moduleRequest));
}



JSModuleRecord* ModuleAnalyzer::analyze(ModuleProgramNode& moduleProgramNode)
{
    // Traverse the module AST and collect
    // * Import entries
    // * Export entries that have FromClause (e.g. export { a } from "mod")
    // * Export entries that have star (e.g. export * from "mod")
    // * Aliased export names (e.g. export { a as b })
    moduleProgramNode.analyzeModule(*this);

    // Based on the collected information, categorize export entries into 3 types.
    // 1. Local export entries
    //     This references the local variable in the current module.
    //     This variable should be allocated in the current module environment as a heap variable.
    //
    //     const variable = 20
    //     export { variable }
    //
    // 2. Namespace export entries
    //     This references the namespace object imported by some import entries.
    //     This variable itself should be allocated in the current module environment as a heap variable.
    //     But when the other modules attempt to resolve this export name in this module, this module
    //     should tell the link to the original module.
    //
    //     import * as namespace from "mod"
    //     export { namespace as mod }
    //
    // 3. Indirect export entries
    //     This references the imported binding name from the other module.
    //     This module environment itself should hold the pointer to (1) the original module and
    //     (2) the binding in the original module. The variable itself is allocated in the original
    //     module. This indirect binding is resolved when the CodeBlock resolves the references.
    //
    //     import mod from "mod"
    //     export { mod }
    //
    //     export { a } from "mod"
    //
    // And separeted from the above 3 types, we also collect the star export entries.
    //
    // 4. Star export entries
    //     This exports all the names from the specified external module as the current module's name.
    //
    //     export * from "mod"
    for (const auto& pair : m_moduleRecord->declaredVariables())
        exportVariable(moduleProgramNode, pair.key, pair.value);

    for (const auto& pair : m_moduleRecord->lexicalVariables())
        exportVariable(moduleProgramNode, pair.key, pair.value);

    ensureDefaultExportIfNothingExported();
    if (Options::dumpModuleRecord())
        m_moduleRecord->dump();

    return m_moduleRecord.get();
}

void ModuleAnalyzer::ensureDefaultExportIfNothingExported() {

    WTF::String source = m_moduleRecord->sourceCode().view().toString();
    SourceCode sourceC;

    if (m_moduleRecord->requestedModules().isEmpty() && m_moduleRecord->exportEntries().isEmpty() && m_moduleRecord->starExportEntries().isEmpty()) {

        ParserError error;
        const char *defaultExportSrc = "export default undefined;";
        const URL& moduleUrl = m_moduleRecord->sourceCode().provider()->url();
        sourceC = makeSource(defaultExportSrc, SourceOrigin(), URL(), WTF::TextPosition(), SourceProviderSourceType::Module);

        std::unique_ptr<ModuleProgramNode> moduleProgramNode = parse<ModuleProgramNode>(
                                                                                        m_vm, sourceC, Identifier(), JSParserBuiltinMode::NotBuiltin,
                                                                                        JSParserStrictMode::NotStrict, JSParserScriptMode::Module, SourceParseMode::ModuleAnalyzeMode, SuperBinding::NotNeeded, error);
        if (error.isValid()) {
            auto consoleMsgArgs = Inspector::ScriptArguments::create(*m_exec,
                                                                     {
                                                                         Strong<JSC::Unknown>(*m_vm, jsString(m_exec, makeString("Error parsing default export for key: ", m_moduleRecord->moduleKey().utf8().data(), ". Source: '", defaultExportSrc,"'"))),
                                                                         Strong<JSC::Unknown>(*m_vm, jsString(m_exec, error.message())),
                                                                     });
            ConsoleClient::printConsoleMessageWithArguments(MessageSource::JS,
                                                            MessageType::Assert,
                                                            MessageLevel::Error,
                                                            m_exec,
                                                            WTFMove(consoleMsgArgs));
            return;
        }

        m_moduleRecord = Strong<JSModuleRecord>(*m_vm, JSModuleRecord::create(m_exec, *m_vm, m_exec->lexicalGlobalObject()->moduleRecordStructure(),  m_moduleRecord->moduleKey(), sourceC, moduleProgramNode->varDeclarations(), moduleProgramNode->lexicalVariables()));

        parseModule(moduleProgramNode.get());
        WTF::StringBuilder moduleFunctionSource;
        moduleFunctionSource.append(COMMONJS_FUNCTION_PROLOGUE);
        moduleFunctionSource.append(source);
        moduleFunctionSource.append(COMMONJS_FUNCTION_EPILOGUE);

        JSObject* exception = nullptr;

        SourceCode functionSource = makeSource(moduleFunctionSource.toString(), SourceOrigin(), URL(moduleUrl), WTF::TextPosition(), SourceProviderSourceType::Module);
        FunctionExecutable* moduleFunctionExecutable = FunctionExecutable::fromGlobalCode(Identifier::fromString(m_exec, "anonymous"), *m_exec, functionSource, exception, -1, WTF::nullopt);
        if (!moduleFunctionExecutable) {
            ASSERT(exception);

            auto consoleMsgArgs = Inspector::ScriptArguments::create(*m_exec,
                                                                     {
                                                                         Strong<JSC::Unknown>(*m_vm, jsString(m_exec, "Error defining CommonJS function: "_s)),
                                                                         Strong<JSC::Unknown>(*m_vm, jsString(m_exec, moduleFunctionSource.toString())),
                                                                         Strong<JSC::Unknown>(*m_vm, exception),
                                                                         Strong<JSC::Unknown>(*m_vm, exception->getDirect(*m_vm, m_vm->propertyNames->stack)),
                                                                     });
            ConsoleClient::printConsoleMessageWithArguments(MessageSource::JS,
                                                            MessageType::Assert,
                                                            MessageLevel::Error,
                                                            m_exec,
                                                            WTFMove(consoleMsgArgs));
            return;
        }

        JSFunction* moduleFunction = JSFunction::create(*m_vm, moduleFunctionExecutable, m_exec->lexicalGlobalObject());
        m_moduleRecord->putDirect(*m_vm, Identifier::fromString(m_vm, "CommonJSModuleFunction"), moduleFunction);

    }
}

JSModuleRecord* ModuleAnalyzer::parseModule(ModuleProgramNode* moduleProgramNode) {

    if (!moduleProgramNode) {
        return nullptr;
    }

    return analyze(*moduleProgramNode);
}

} // namespace JSC
