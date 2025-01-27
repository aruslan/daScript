#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/interop.h"

namespace das
{
    template  <typename FuncT, FuncT fn, typename SimNodeT, typename FuncArgT>
    class ExternalFn : public BuiltInFunction {

        static_assert ( is_base_of<SimNode_CallBase, SimNodeT>::value, "only call-based nodes allowed" );

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif
        template <typename ArgumentsType, size_t... I>
        inline vector<TypeDeclPtr> makeArgs ( const ModuleLibrary & lib, index_sequence<I...> ) {
            return { makeType< typename tuple_element<I, ArgumentsType>::type>(lib)... };
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    public:
        ExternalFn(const string & name, const ModuleLibrary & lib, const string & cppName = string())
        : BuiltInFunction(name,cppName) {
            callBased = true;
            using FunctionTrait = function_traits<FuncArgT>;
            const int nargs = tuple_size<typename FunctionTrait::arguments>::value;
            using Indices = make_index_sequence<nargs>;
            using Arguments = typename FunctionTrait::arguments;
            using Result  = typename FunctionTrait::return_type;
            auto args = makeArgs<Arguments>(lib, Indices());
            for ( int argi=0; argi!=nargs; ++argi ) {
                auto arg = make_shared<Variable>();
                arg->name = "arg" + to_string(argi);
                arg->type = args[argi];
                if ( arg->type->baseType==Type::fakeContext ) {
                    arg->init = make_shared<ExprFakeContext>(at);
                }
                this->arguments.push_back(arg);
            }
            this->result = makeType<Result>(lib);
            this->totalStackSize = sizeof(Prologue);
            if ( result->isRefType() ) {
                if ( result->canCopy() ) {
                    copyOnReturn = true;
                    moveOnReturn = false;
                } else if ( result->canMove() ) {
                    copyOnReturn = false;
                    moveOnReturn = true;
                } else if ( !result->isRef() ) {
                    DAS_FATAL_LOG("ExternalFn %s can't be bound. It returns values which can't be copied or moved\n", name.c_str());
                    DAS_FATAL_ERROR;
                }
            }
        }
        virtual SimNode * makeSimNode ( Context & context ) override {
            const char * fnName = context.code->allocateName(this->name);
            return context.code->makeNode<SimNodeT>(at, fnName);
        }
    };

    template  <typename RetT, typename ...Args>
    class InteropFnBase : public BuiltInFunction {
    public:
        InteropFnBase(const string & name, const ModuleLibrary & lib, const string & cppName = string())
            : BuiltInFunction(name,cppName) {
            vector<TypeDeclPtr> args = { makeType<Args>(lib)... };
            for ( size_t argi=0; argi!=args.size(); ++argi ) {
                auto arg = make_shared<Variable>();
                arg->name = "arg" + to_string(argi);
                arg->type = args[argi];
                if ( arg->type->baseType==Type::fakeContext ) {
                    arg->init = make_shared<ExprConstPtr>(at);
                }
                this->arguments.push_back(arg);
            }
            this->result = makeType<RetT>(lib);
            this->totalStackSize = sizeof(Prologue);
        }
    };

    template  <InteropFunction func, typename RetT, typename ...Args>
    class InteropFn : public InteropFnBase<RetT,Args...> {
    public:
        InteropFn(const string & name, const ModuleLibrary & lib, const string & cppName = string())
            : InteropFnBase<RetT,Args...>(name,lib,cppName) {
            this->callBased = true;
            this->interopFn = true;
        }
        virtual SimNode * makeSimNode ( Context & context ) override {
            const char * fnName = context.code->allocateName(this->name);
            return context.code->makeNode<SimNode_InteropFuncCall<func>>(BuiltInFunction::at,fnName);
        }
    };

    template <typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall>
    inline auto addExtern ( Module & mod, const ModuleLibrary & lib, const string & name, SideEffects seFlags,
                                  const string & cppName = string()) {
        auto fnX = make_shared<ExternalFn<FuncT, fn, SimNodeT<FuncT, fn>, FuncT>>(name, lib, cppName);
        fnX->setSideEffects(seFlags);
        if ( !mod.addFunction(fnX) ) {
            DAS_FATAL_LOG("addExtern(%s) failed in module %s\n", name.c_str(), mod.name.c_str());
            DAS_FATAL_ERROR;
        }
        return fnX;
    }

    template <typename FuncArgT, typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall>
    inline auto addExternEx ( Module & mod, const ModuleLibrary & lib, const string & name, SideEffects seFlags,
                                  const string & cppName = string()) {
        auto fnX = make_shared<ExternalFn<FuncT, fn, SimNodeT<FuncT, fn>, FuncArgT>>(name, lib, cppName);
        fnX->setSideEffects(seFlags);
        if ( !mod.addFunction(fnX) ) {
            DAS_FATAL_LOG("addExternEx(%s) failed in module %s\n", name.c_str(), mod.name.c_str());
            DAS_FATAL_ERROR;
        }
        return fnX;
    }

    template <InteropFunction func, typename RetT, typename ...Args>
    inline auto addInterop ( Module & mod, const ModuleLibrary & lib, const string & name, SideEffects seFlags,
                                   const string & cppName = string() ) {
        auto fnX = make_shared<InteropFn<func, RetT, Args...>>(name, lib, cppName);
        fnX->setSideEffects(seFlags);
        if ( !mod.addFunction(fnX) ) {
            DAS_FATAL_LOG("addInterop(%s) failed in module %s\n", name.c_str(), mod.name.c_str());
            DAS_FATAL_ERROR;
        }
        return fnX;
    }
}

