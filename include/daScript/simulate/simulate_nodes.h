#pragma once

#include "simulate.h"

#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    // Delete structures
    struct SimNode_DeleteStructPtr : SimNode_Delete {
        SimNode_DeleteStructPtr ( const LineInfo & a, SimNode * s, uint32_t t, uint32_t ss )
            : SimNode_Delete(a,s,t), structSize(ss) {}
        virtual vec4f eval ( Context & context ) override;
        virtual SimNode * visit ( SimVisitor & vis ) override;
        uint32_t    structSize;
    };

    // New handle, default
    template <typename TT>
    struct SimNode_NewHandle : SimNode {
        DAS_PTR_NODE;
        SimNode_NewHandle ( const LineInfo & a ) : SimNode(a) {}
        __forceinline char * compute ( Context & ) {
            return (char *) new TT();
        }
        virtual SimNode * visit ( SimVisitor & vis ) override;
    };

    // Delete handle, default
    template <typename TT>
    struct SimNode_DeleteHandlePtr : SimNode_Delete {
        SimNode_DeleteHandlePtr ( const LineInfo & a, SimNode * s, uint32_t t )
            : SimNode_Delete(a,s,t) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto pH = (TT **) subexpr->evalPtr(context);
            for ( uint32_t i=0; i!=total; ++i, pH++ ) {
                if ( *pH ) {
                    delete * pH;
                    *pH = nullptr;
                }
            }
            return v_zero();
        }
    };

    // MakeBlock
    struct SimNode_MakeBlock : SimNode {
        SimNode_MakeBlock ( const LineInfo & at, SimNode * s, uint32_t a, uint32_t spp )
            : SimNode(at), subexpr(s), argStackTop(a), stackTop(spp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode *       subexpr;
        uint32_t        argStackTop;
        uint32_t        stackTop;
    };

    // ASSERT
    struct SimNode_Assert : SimNode {
        SimNode_Assert ( const LineInfo & at, SimNode * s, const char * m )
            : SimNode(at), subexpr(s), message(m) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode *       subexpr;
        const char *    message;
    };

    // VECTOR SWIZZLE
    // TODO: make at least 3 different versions
    struct SimNode_Swizzle : SimNode {
        SimNode_Swizzle ( const LineInfo & at, SimNode * rv, uint8_t * fi )
            : SimNode(at), value(rv) {
                fields[0] = fi[0];
                fields[1] = fi[1];
                fields[2] = fi[2];
                fields[3] = fi[3];
            }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode *   value;
        uint8_t     fields[4];
    };

    // FIELD .
    struct SimNode_FieldDeref : SimNode {
        DAS_PTR_NODE;
        SimNode_FieldDeref ( const LineInfo & at, SimNode * rv, uint32_t of )
            : SimNode(at), value(rv), offset(of) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            return value->evalPtr(context) + offset;
        }
        SimNode *   value;
        uint32_t    offset;
    };

    template <typename TT>
    struct SimNode_FieldDerefR2V : SimNode_FieldDeref {
        SimNode_FieldDerefR2V ( const LineInfo & at, SimNode * rv, uint32_t of )
            : SimNode_FieldDeref(at,rv,of) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto prv = value->evalPtr(context);
            TT * pR = (TT *)( prv + offset );
            return cast<TT>::from(*pR);

        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            auto prv = value->evalPtr(context);                     \
            return * (CTYPE *)( prv + offset );                     \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // PTR FIELD .
    struct SimNode_PtrFieldDeref : SimNode {
        DAS_PTR_NODE;
        SimNode_PtrFieldDeref(const LineInfo & at, SimNode * rv, uint32_t of)
            : SimNode(at), value(rv), offset(of) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute(Context & context) {
            auto prv = value->evalPtr(context);
            if (prv) {
                return prv + offset;
            }
            else {
                context.throw_error_at(debugInfo,"dereferencing null pointer");
                return nullptr;
            }
        }
        SimNode *   value;
        uint32_t    offset;
    };

    template <typename TT>
    struct SimNode_PtrFieldDerefR2V : SimNode_PtrFieldDeref {
        SimNode_PtrFieldDerefR2V(const LineInfo & at, SimNode * rv, uint32_t of)
            : SimNode_PtrFieldDeref(at, rv, of) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval(Context & context) override {
            auto prv = value->evalPtr(context);
            if (prv) {
                TT * pR = (TT *)(prv + offset);
                return cast<TT>::from(*pR);
            }
            else {
                context.throw_error_at(debugInfo,"dereferencing null pointer");
                return v_zero();
            }
        }
        virtual char * evalPtr(Context & context) override {
            auto prv = value->evalPtr(context);
            if (prv) {
                return *(char **)(prv + offset);
            }
            else {
                context.throw_error_at(debugInfo,"dereferencing null pointer");
                return nullptr;
            }
        }
    };

    // FIELD ?.
    struct SimNode_SafeFieldDeref : SimNode_FieldDeref {
        DAS_PTR_NODE;
        SimNode_SafeFieldDeref ( const LineInfo & at, SimNode * rv, uint32_t of )
            : SimNode_FieldDeref(at,rv,of) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            auto prv = value->evalPtr(context);
            return prv ? prv + offset : nullptr;
        }
    };

    // FIELD ?.->
    struct SimNode_SafeFieldDerefPtr : SimNode_FieldDeref {
        DAS_PTR_NODE;
        SimNode_SafeFieldDerefPtr ( const LineInfo & at, SimNode * rv, uint32_t of )
            : SimNode_FieldDeref(at,rv,of) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            char ** prv = (char **) value->evalPtr(context);
            return prv ? *(prv + offset) : nullptr;
        }
    };

    // AT (INDEX)
    struct SimNode_At : SimNode {
        DAS_PTR_NODE;
        SimNode_At ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t strd, uint32_t o, uint32_t rng )
            : SimNode(at), value(rv), index(idx), stride(strd), offset(o), range(rng) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute (Context & context) {
            auto pValue = value->evalPtr(context);
            uint32_t idx = uint32_t(index->evalInt(context));
            if (idx >= range) {
                context.throw_error_at(debugInfo,"index out of range");
                return nullptr;
            } else {
                return pValue + idx*stride + offset;
            }
        }
        SimNode * value, * index;
        uint32_t  stride, offset, range;
    };

    template <typename TT>
    struct SimNode_AtR2V : SimNode_At {
        SimNode_AtR2V ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t strd, uint32_t o, uint32_t rng )
            : SimNode_At(at,rv,idx,strd,o,rng) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *) compute(context);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)compute(context);                      \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // AT (INDEX)
    template <typename TT>
    struct SimNode_AtVector;

#define SIM_NODE_AT_VECTOR(TYPE,CTYPE)                                                          \
    template <>                                                                                 \
    struct SimNode_AtVector<CTYPE> : SimNode {                                                  \
        SimNode_AtVector ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t rng )     \
            : SimNode(at), value(rv), index(idx), range(rng) {}                                 \
        virtual SimNode * visit ( SimVisitor & vis ) override {                                 \
            V_BEGIN();                                                                          \
            V_OP(AtVector "_" #TYPE);                                                           \
            V_SUB(value);                                                                       \
            V_SUB(index);                                                                       \
            V_ARG(range);                                                                       \
            V_END();                                                                            \
        }                                                                                       \
        __forceinline CTYPE compute ( Context & context ) {                                     \
            auto vec = value->eval(context);                                                    \
            uint32_t idx = uint32_t(index->evalInt(context));                                   \
            if (idx >= range) {                                                                 \
                context.throw_error_at(debugInfo,"index out of range");                         \
                return (CTYPE) 0;                                                               \
            } else {                                                                            \
                CTYPE * pv = (CTYPE *) &vec;                                                    \
                return pv[idx];                                                                 \
            }                                                                                   \
        }                                                                                       \
        DAS_NODE(TYPE, CTYPE)                                                                   \
        SimNode * value, * index;                                                               \
        uint32_t  range;                                                                        \
    };

SIM_NODE_AT_VECTOR(Int,   int32_t)
SIM_NODE_AT_VECTOR(UInt,  uint32_t)
SIM_NODE_AT_VECTOR(Float, float)

    template <int nElem>
    struct EvalBlock { static __forceinline void eval(Context & context, SimNode ** arguments, vec4f * argValues) {
            for (int i = 0; i != nElem && !context.stopFlags; ++i)
                argValues[i] = arguments[i]->eval(context);
    }};
    template <>    struct EvalBlock<0> { static __forceinline void eval(Context &, SimNode **, vec4f *) {} };
    template <> struct EvalBlock<1> {    static __forceinline void eval(Context & context, SimNode ** arguments, vec4f * argValues) {
                                    argValues[0] = arguments[0]->eval(context);
    }};
    template <> struct EvalBlock<2> {    static __forceinline void eval(Context & context, SimNode ** arguments, vec4f * argValues) {
                                    argValues[0] = arguments[0]->eval(context);
            if (!context.stopFlags) argValues[1] = arguments[1]->eval(context);
    }};
    template <> struct EvalBlock<3> {    static __forceinline void eval(Context & context, SimNode ** arguments, vec4f * argValues) {
                                    argValues[0] = arguments[0]->eval(context);
            if (!context.stopFlags) argValues[1] = arguments[1]->eval(context);
            if (!context.stopFlags) argValues[2] = arguments[2]->eval(context);
    }};
    template <> struct EvalBlock<4> {    static __forceinline void eval(Context & context, SimNode ** arguments, vec4f * argValues) {
                                    argValues[0] = arguments[0]->eval(context);
            if (!context.stopFlags) argValues[1] = arguments[1]->eval(context);
            if (!context.stopFlags) argValues[2] = arguments[2]->eval(context);
            if (!context.stopFlags) argValues[3] = arguments[3]->eval(context);
    }};

    // FUNCTION CALL via FASTCALL convention
    template <int argCount>
    struct SimNode_FastCall : SimNode_CallBase {
        SimNode_FastCall ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            auto aa = context.abiArg;
            context.abiArg = argValues;
            auto res = fnPtr->code->eval(context);
            context.stopFlags &= ~(EvalFlags::stopForReturn | EvalFlags::stopForBreak | EvalFlags::stopForContinue);
            context.abiArg = aa;
            return res;
        }
#define EVAL_NODE(TYPE,CTYPE)\
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
                vec4f argValues[argCount ? argCount : 1];                                       \
                EvalBlock<argCount>::eval(context, arguments, argValues);                       \
                auto aa = context.abiArg;                                                       \
                context.abiArg = argValues;                                                     \
                auto res = EvalTT<CTYPE>::eval(context, fnPtr->code);                           \
                context.stopFlags &= ~(EvalFlags::stopForReturn | EvalFlags::stopForBreak | EvalFlags::stopForContinue); \
                context.abiArg = aa;                                                            \
                return res;                                                                     \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // FUNCTION CALL
    template <int argCount>
    struct SimNode_Call : SimNode_CallBase {
        SimNode_Call ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            return context.call(fnPtr, argValues, debugInfo.line);
        }
#define EVAL_NODE(TYPE,CTYPE)\
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
                vec4f argValues[argCount ? argCount : 1];                                       \
                EvalBlock<argCount>::eval(context, arguments, argValues);                       \
                return cast<CTYPE>::to(context.call(fnPtr, argValues, debugInfo.line));    \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // FUNCTION CALL with copy-or-move-on-return
    template <int argCount>
    struct SimNode_CallAndCopyOrMove : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_CallAndCopyOrMove ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
                auto cmres = cmresEval->evalPtr(context);
                vec4f argValues[argCount ? argCount : 1];
                EvalBlock<argCount>::eval(context, arguments, argValues);
                return cast<char *>::to(context.callWithCopyOnReturn(fnPtr, argValues, cmres, debugInfo.line));
        }
    };

    // Invoke
    template <int argCount>
    struct SimNode_Invoke : SimNode_CallBase {
        SimNode_Invoke ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            Block * block = *(Block **)(argValues);
            if ( argCount>1 ) {
                return context.invoke(*block, argValues + 1, nullptr);
            } else {
                return context.invoke(*block, nullptr, nullptr);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                                                   \
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
            vec4f argValues[argCount ? argCount : 1];                                           \
            EvalBlock<argCount>::eval(context, arguments, argValues);                           \
            Block * block = *(Block **)(argValues);                                             \
            if ( argCount>1 ) {                                                                 \
                return cast<CTYPE>::to(context.invoke(*block, argValues + 1, nullptr));         \
            } else {                                                                            \
                return cast<CTYPE>::to(context.invoke(*block, nullptr, nullptr));               \
            }                                                                                   \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // Invoke with copy-or-move-on-return
    template <int argCount>
    struct SimNode_InvokeAndCopyOrMove : SimNode_CallBase {
        SimNode_InvokeAndCopyOrMove ( const LineInfo & at, SimNode * spCMRES )
            : SimNode_CallBase(at) { cmresEval = spCMRES; }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto cmres = cmresEval->evalPtr(context);
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            Block * block = *(Block **)(argValues);
            if ( argCount>1 ) {
                return context.invoke(*block, argValues + 1, cmres);
            } else {
                return context.invoke(*block, nullptr, cmres);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                                                   \
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
            auto cmres = cmresEval->evalPtr(context);                                           \
            vec4f argValues[argCount ? argCount : 1];                                           \
            EvalBlock<argCount>::eval(context, arguments, argValues);                           \
            Block * block = *(Block **)(argValues);                                             \
            if ( argCount>1 ) {                                                                 \
                return cast<CTYPE>::to(context.invoke(*block, argValues + 1, cmres));           \
            } else {                                                                            \
                return cast<CTYPE>::to(context.invoke(*block, nullptr, cmres));                 \
            }                                                                                   \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // Invoke function
    template <int argCount>
    struct SimNode_InvokeFn : SimNode_CallBase {
        SimNode_InvokeFn ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            SimFunction * simFunc = context.getFunction(cast<Func>::to(argValues[0]).index-1);
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");
            if ( argCount>1 ) {
                return context.call(simFunc, argValues + 1, debugInfo.line);
            } else {
                return context.call(simFunc, nullptr, debugInfo.line);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                                                   \
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
            vec4f argValues[argCount ? argCount : 1];                                           \
            EvalBlock<argCount>::eval(context, arguments, argValues);                           \
            SimFunction * simFunc = context.getFunction(cast<Func>::to(argValues[0]).index-1);  \
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");             \
            if ( argCount>1 ) {                                                                 \
                return cast<CTYPE>::to(context.call(simFunc, argValues + 1, debugInfo.line));   \
            } else {                                                                            \
                return cast<CTYPE>::to(context.call(simFunc, nullptr, debugInfo.line));         \
            }                                                                                   \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // Invoke function
    template <int argCount>
    struct SimNode_InvokeLambda : SimNode_CallBase {
        SimNode_InvokeLambda ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            int32_t * fnIndex = *(int32_t **)(argValues);
            if (!fnIndex) context.throw_error_at(debugInfo,"invoke null lambda");
            SimFunction * simFunc = context.getFunction(*fnIndex-1);
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");
            return context.call(simFunc, argValues, debugInfo.line);
        }
#define EVAL_NODE(TYPE,CTYPE)                                                                   \
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
            vec4f argValues[argCount ? argCount : 1];                                           \
            EvalBlock<argCount>::eval(context, arguments, argValues);                           \
            int32_t * fnIndex = *(int32_t **)(argValues);                                       \
            if (!fnIndex) context.throw_error_at(debugInfo,"invoke null lambda");               \
            SimFunction * simFunc = context.getFunction(*fnIndex-1);                            \
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");             \
            return cast<CTYPE>::to(context.call(simFunc, argValues, debugInfo.line));           \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // Invoke function with copy-or-move-on-return
    template <int argCount>
    struct SimNode_InvokeAndCopyOrMoveFn : SimNode_CallBase {
        SimNode_InvokeAndCopyOrMoveFn ( const LineInfo & at, SimNode * spEval )
            : SimNode_CallBase(at) { cmresEval = spEval; }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto cmres = cmresEval->evalPtr(context);
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            SimFunction * simFunc = context.getFunction(cast<Func>::to(argValues[0]).index-1);
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");
            if ( argCount>1 ) {
                return context.callWithCopyOnReturn(simFunc, argValues + 1, cmres, debugInfo.line);
            } else {
                return context.callWithCopyOnReturn(simFunc, nullptr, cmres, debugInfo.line);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {                                   \
            auto cmres = cmresEval->evalPtr(context);                                               \
            vec4f argValues[argCount ? argCount : 1];                                               \
            EvalBlock<argCount>::eval(context, arguments, argValues);                               \
            SimFunction * simFunc = context.getFunction(cast<Func>::to(argValues[0]).index-1);      \
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");                 \
            if ( argCount>1 ) {                                                                     \
                return cast<CTYPE>::to(context.callWithCopyOnReturn(simFunc, argValues + 1, cmres, debugInfo.line)); \
            } else {                                                                                \
                return cast<CTYPE>::to(context.callWithCopyOnReturn(simFunc, nullptr, cmres, debugInfo.line)); \
            }                                                                                       \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // Invoke function
    template <int argCount>
    struct SimNode_InvokeAndCopyOrMoveLambda : SimNode_CallBase {
        SimNode_InvokeAndCopyOrMoveLambda ( const LineInfo & at, SimNode * spEval )
            : SimNode_CallBase(at) { cmresEval = spEval; }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto cmres = cmresEval->evalPtr(context);
            vec4f argValues[argCount ? argCount : 1];
            EvalBlock<argCount>::eval(context, arguments, argValues);
            int32_t * fnIndex = *(int32_t **) (argValues);
            if (!fnIndex) context.throw_error_at(debugInfo,"invoke null lambda");
            SimFunction * simFunc = context.getFunction(*fnIndex-1);
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");
            return context.callWithCopyOnReturn(simFunc, argValues, cmres, debugInfo.line);
        }
#define EVAL_NODE(TYPE,CTYPE)                                                                   \
        virtual CTYPE eval##TYPE ( Context & context ) override {                               \
            auto cmres = cmresEval->evalPtr(context);                                           \
            vec4f argValues[argCount ? argCount : 1];                                           \
            EvalBlock<argCount>::eval(context, arguments, argValues);                           \
            int32_t * fnIndex = *(int32_t **) (argValues);                                      \
            if (!fnIndex) context.throw_error_at(debugInfo,"invoke null lambda");               \
            SimFunction * simFunc = context.getFunction(*fnIndex-1);                            \
            if (!simFunc) context.throw_error_at(debugInfo,"invoke null function");             \
            return cast<CTYPE>::to(context.callWithCopyOnReturn(simFunc, argValues, cmres, debugInfo.line));    \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
    };

    // StringBuilder
    struct SimNode_StringBuilder : SimNode_CallBase {
        SimNode_StringBuilder ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
    };

    // CAST
    template <typename CastTo, typename CastFrom>
    struct SimNode_Cast : SimNode_CallBase {
        SimNode_Cast ( const LineInfo & at ) : SimNode_CallBase(at) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f res = arguments[0]->eval(context);
            CastTo value = (CastTo) cast<CastFrom>::to(res);
            return cast<CastTo>::from(value);
        }
    };

    // LEXICAL CAST
    template <typename CastFrom>
    struct SimNode_LexicalCast : SimNode_CallBase {
        SimNode_LexicalCast ( const LineInfo & at ) : SimNode_CallBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f res = arguments[0]->eval(context);
            auto str = to_string ( cast<CastFrom>::to(res) );
            auto cpy = context.heap.allocateString(str);
            if ( !cpy ) {
                context.throw_error_at(debugInfo,"can't cast to string, out of heap");
                return v_zero();
            }
            return cast<char *>::from(cpy);
        }
    };

    // "DEBUG"
    struct SimNode_Debug : SimNode {
        SimNode_Debug ( const LineInfo & at, SimNode * s, TypeInfo * ti, char * msg )
            : SimNode(at), subexpr(s), typeInfo(ti), message(msg) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode *       subexpr;
        TypeInfo *      typeInfo;
        const char *    message;
    };

    // CMRES "GET" + OFFSET
    struct SimNode_GetCMResOfs : SimNode {
        DAS_PTR_NODE;
        SimNode_GetCMResOfs(const LineInfo & at, uint32_t o)
            : SimNode(at), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            return context.abiCopyOrMoveResult() + offset;
        }
        uint32_t offset;
    };

    template <typename TT>
    struct SimNode_GetCMResOfsR2V : SimNode_GetCMResOfs {
        SimNode_GetCMResOfsR2V(const LineInfo & at, uint32_t o)
            : SimNode_GetCMResOfs(at,o)  {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *)compute(context);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)compute(context);                      \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // BLOCK CMRES "GET" + OFFSET
    struct SimNode_GetBlockCMResOfs : SimNode {
        DAS_PTR_NODE;
        SimNode_GetBlockCMResOfs(const LineInfo & at, uint32_t o, uint32_t asp)
            : SimNode(at), offset(o), argStackTop(asp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            auto ba = (BlockArguments *) ( context.stack.sp() + argStackTop );
            return ba->copyOrMoveResult + offset;
        }
        uint32_t offset, argStackTop;
    };

    // LOCAL VARIABLE "GET"
    struct SimNode_GetLocal : SimNode {
        DAS_PTR_NODE;
        SimNode_GetLocal(const LineInfo & at, uint32_t sp)
            : SimNode(at), stackTop(sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            return context.stack.sp() + stackTop;
        }
        uint32_t stackTop;
    };

    template <typename TT>
    struct SimNode_GetLocalR2V : SimNode_GetLocal {
        SimNode_GetLocalR2V(const LineInfo & at, uint32_t sp)
            : SimNode_GetLocal(at,sp)  {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *)(context.stack.sp() + stackTop);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)(context.stack.sp() + stackTop);       \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // WHEN LOCAL VARIABLE STORES REFERENCE
    struct SimNode_GetLocalRef : SimNode_GetLocal {
        DAS_PTR_NODE;
        SimNode_GetLocalRef(const LineInfo & at, uint32_t sp)
            : SimNode_GetLocal(at,sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            return *(char **)(context.stack.sp() + stackTop);
        }
    };

    template <typename TT>
    struct SimNode_GetLocalRefR2V : SimNode_GetLocalRef {
        SimNode_GetLocalRefR2V(const LineInfo & at, uint32_t sp)
            : SimNode_GetLocalRef(at,sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = *(TT **)(context.stack.sp() + stackTop);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return **(CTYPE **)(context.stack.sp() + stackTop);     \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // WHEN LOCAL VARIABLE STORES REFERENCE
    struct SimNode_GetLocalRefOff : SimNode_GetLocal {
        DAS_PTR_NODE;
        SimNode_GetLocalRefOff(const LineInfo & at, uint32_t sp, uint32_t o)
            : SimNode_GetLocal(at,sp), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            return (*(char **)(context.stack.sp() + stackTop)) + offset;
        }
        uint32_t offset;
    };

    template <typename TT>
    struct SimNode_GetLocalRefR2VOff : SimNode_GetLocalRef {
        SimNode_GetLocalRefR2VOff(const LineInfo & at, uint32_t sp, uint32_t o)
            : SimNode_GetLocalRef(at,sp), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *)((*(char **)(context.stack.sp() + stackTop)) + offset);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)((*(char **)(context.stack.sp() + stackTop)) + offset); \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        uint32_t offset;
    };

    // AT (INDEX)
    struct SimNode_SetLocalRefAndEval : SimNode {
        DAS_PTR_NODE;
        SimNode_SetLocalRefAndEval ( const LineInfo & at, SimNode * rv, SimNode * ev, uint32_t sp )
            : SimNode(at), refValue(rv), evalValue(ev), stackTop(sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute (Context & context) {
            auto refV = refValue->evalPtr(context);
            auto pR = (char **)(context.stack.sp() + stackTop);
            *pR = refV;
            evalValue->eval(context);
            return refV;
        }
        SimNode * refValue, * evalValue;
        uint32_t  stackTop;
    };

    // ZERO MEMORY OF UNITIALIZED LOCAL VARIABLE
    struct SimNode_InitLocal : SimNode {
        SimNode_InitLocal(const LineInfo & at, uint32_t sp, uint32_t sz)
            : SimNode(at), stackTop(sp), size(sz) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            memset(context.stack.sp() + stackTop, 0, size);
            return v_zero();
        }
        uint32_t stackTop, size;
    };

    // ZERO MEMORY OF UNITIALIZED CMRES LOCAL VARIABLE
    struct SimNode_InitLocalCMRes : SimNode {
        SimNode_InitLocalCMRes(const LineInfo & at, uint32_t o, uint32_t sz)
            : SimNode(at), offset(o), size(sz) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            memset(context.abiCopyOrMoveResult() + offset, 0, size);
            return v_zero();
        }
        uint32_t offset, size;
    };

    // ZERO MEMORY OF UNITIALIZED LOCAL VARIABLE VIA REFERENCE AND OFFSET
    struct SimNode_InitLocalRef : SimNode {
        SimNode_InitLocalRef(const LineInfo & at, uint32_t sp, uint32_t o, uint32_t sz)
            : SimNode(at), stackTop(sp), offset(o), size(sz) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            char * pI = *((char **)(context.stack.sp() + stackTop)) + offset;
            memset(pI, 0, size);
            return v_zero();
        }
        uint32_t stackTop, offset, size;
    };

    // ARGUMENT VARIABLE "GET"
    struct SimNode_GetArgument : SimNode {
        SimNode_GetArgument ( const LineInfo & at, int32_t i )
            : SimNode(at), index(i) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            return context.abiArguments()[index];
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)(context.abiArguments()+index);  \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        int32_t index;
    };

    struct SimNode_GetArgumentRef : SimNode_GetArgument {
        DAS_PTR_NODE;
        SimNode_GetArgumentRef(const LineInfo & at, int32_t i)
            : SimNode_GetArgument(at,i) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute(Context & context) {
            return (char *)(context.abiArguments()+index);
        }
    };

    template <typename TT>
    struct SimNode_GetArgumentR2V : SimNode_GetArgument {
        SimNode_GetArgumentR2V ( const LineInfo & at, int32_t i )
            : SimNode_GetArgument(at,i) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = *(TT **)(context.abiArguments()+index);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                               \
        virtual CTYPE eval##TYPE ( Context & context ) override {           \
            return **(CTYPE **)(context.abiArguments()+index);              \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    struct SimNode_GetArgumentOff : SimNode_GetArgument {
        DAS_PTR_NODE;
        SimNode_GetArgumentOff(const LineInfo & at, int32_t i, uint32_t o)
            : SimNode_GetArgument(at,i), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute(Context & context) {
            char * pR = *(char **)(context.abiArguments()+index);
            return pR + offset;
        }
        uint32_t offset;
    };

    template <typename TT>
    struct SimNode_GetArgumentR2VOff : SimNode_GetArgument {
        SimNode_GetArgumentR2VOff ( const LineInfo & at, int32_t i, uint32_t o )
            : SimNode_GetArgument(at,i), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            char * pR = *(char **)(context.abiArguments()+index);
            return cast<TT>::from(*((TT *)(pR+offset)));
        }
#define EVAL_NODE(TYPE,CTYPE)                                               \
        virtual CTYPE eval##TYPE ( Context & context ) override {           \
            char * pR = *(char **)(context.abiArguments()+index);           \
            return *(CTYPE *)(pR + offset);                                 \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        uint32_t offset;
    };

    // BLOCK VARIABLE "GET"
    struct SimNode_GetBlockArgument : SimNode {
        SimNode_GetBlockArgument ( const LineInfo & at, int32_t i, uint32_t sp )
            : SimNode(at), index(i), stackTop(sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f * args = *((vec4f **)(context.stack.sp() + stackTop));
            return args[index];
        }
#define EVAL_NODE(TYPE,CTYPE)                                               \
        virtual CTYPE eval##TYPE ( Context & context ) override {           \
            vec4f * args = *((vec4f **)(context.stack.sp() + stackTop));    \
            return *(CTYPE *)(args+index);                                  \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        int32_t     index;
        uint32_t    stackTop;
    };

    template <typename TT>
    struct SimNode_GetBlockArgumentR2V : SimNode_GetBlockArgument {
        SimNode_GetBlockArgumentR2V ( const LineInfo & at, int32_t i, uint32_t sp )
            : SimNode_GetBlockArgument(at,i,sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            vec4f * args = *((vec4f **)(context.stack.sp() + stackTop));
            TT * pR = *(TT **)(args+index);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                               \
        virtual CTYPE eval##TYPE ( Context & context ) override {           \
            vec4f * args = *((vec4f **)(context.stack.sp() + stackTop));    \
            return **(CTYPE **)(args+index);                                \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    struct SimNode_GetBlockArgumentRef : SimNode_GetBlockArgument {
        DAS_PTR_NODE;
        SimNode_GetBlockArgumentRef(const LineInfo & at, int32_t i, uint32_t sp)
            : SimNode_GetBlockArgument(at,i,sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute(Context & context) {
            vec4f * args = *((vec4f **)(context.stack.sp() + stackTop));
            return (char *)(args+index);
        }
    };

    // THIS BLOCK VARIABLE "GET"
    struct SimNode_GetThisBlockArgument : SimNode {
        SimNode_GetThisBlockArgument ( const LineInfo & at, int32_t i )
            : SimNode(at), index(i) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            return context.abiThisBlockArguments()[index];
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)(context.abiThisBlockArguments()+index);  \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        int32_t index;
    };

    template <typename TT>
    struct SimNode_GetThisBlockArgumentR2V : SimNode_GetThisBlockArgument {
        SimNode_GetThisBlockArgumentR2V ( const LineInfo & at, int32_t i )
            : SimNode_GetThisBlockArgument(at,i) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = *(TT **)(context.abiThisBlockArguments()+index);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                               \
        virtual CTYPE eval##TYPE ( Context & context ) override {           \
            return **(CTYPE **)(context.abiThisBlockArguments()+index);     \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    struct SimNode_GetThisBlockArgumentRef : SimNode_GetThisBlockArgument {
        DAS_PTR_NODE;
        SimNode_GetThisBlockArgumentRef(const LineInfo & at, int32_t i)
            : SimNode_GetThisBlockArgument(at,i) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute(Context & context) {
            return (char *)(context.abiThisBlockArguments()+index);
        }
    };

    // GLOBAL VARIABLE "GET"
    struct SimNode_GetGlobal : SimNode {
        DAS_PTR_NODE;
        SimNode_GetGlobal ( const LineInfo & at, uint32_t o )
            : SimNode(at), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute (Context & context) {
            return context.globals + offset;
        }
        uint32_t    offset;
    };

    template <typename TT>
    struct SimNode_GetGlobalR2V : SimNode_GetGlobal {
        SimNode_GetGlobalR2V ( const LineInfo & at, uint32_t o )
            : SimNode_GetGlobal(at,o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *)(context.globals + offset);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return *(CTYPE *)(context.globals + offset);            \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // TRY-CATCH
    struct SimNode_TryCatch : SimNode {
        SimNode_TryCatch ( const LineInfo & at, SimNode * t, SimNode * c )
            : SimNode(at), try_block(t), catch_block(c) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode * try_block, * catch_block;
    };

    // RETURN
    struct SimNode_Return : SimNode {
        SimNode_Return ( const LineInfo & at, SimNode * s )
            : SimNode(at), subexpr(s) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode * subexpr;
    };

    struct SimNode_ReturnConst : SimNode {
        SimNode_ReturnConst ( const LineInfo & at, vec4f v )
            : SimNode(at), value(v) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        vec4f value;
    };

    struct SimNode_ReturnConstString : SimNode {
        SimNode_ReturnConstString ( const LineInfo & at, char * v )
            : SimNode(at), value(v) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        char * value;
    };

    struct SimNode_ReturnAndCopy : SimNode_Return {
        SimNode_ReturnAndCopy ( const LineInfo & at, SimNode * s, uint32_t sz )
            : SimNode_Return(at,s), size(sz) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        uint32_t size;
    };

    struct SimNode_ReturnRefAndEval : SimNode_Return {
        SimNode_ReturnRefAndEval ( const LineInfo & at, SimNode * s, uint32_t sp )
            : SimNode_Return(at,s), stackTop(sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        uint32_t stackTop;
    };

    struct SimNode_ReturnAndMove : SimNode_ReturnAndCopy {
        SimNode_ReturnAndMove ( const LineInfo & at, SimNode * s, uint32_t sz )
            : SimNode_ReturnAndCopy(at,s,sz) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
    };

    struct SimNode_ReturnReference : SimNode_Return {
        SimNode_ReturnReference ( const LineInfo & at, SimNode * s )
            : SimNode_Return(at,s) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
    };

    struct SimNode_ReturnRefAndEvalFromBlock : SimNode_Return {
        SimNode_ReturnRefAndEvalFromBlock ( const LineInfo & at, SimNode * s, uint32_t sp, uint32_t asp )
            : SimNode_Return(at,s), stackTop(sp), argStackTop(asp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        uint32_t stackTop, argStackTop;
    };

    struct SimNode_ReturnAndCopyFromBlock : SimNode_ReturnAndCopy {
        SimNode_ReturnAndCopyFromBlock ( const LineInfo & at, SimNode * s, uint32_t sz, uint32_t asp )
            : SimNode_ReturnAndCopy(at,s,sz), argStackTop(asp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        uint32_t argStackTop;
    };

    struct SimNode_ReturnAndMoveFromBlock : SimNode_ReturnAndCopyFromBlock {
        SimNode_ReturnAndMoveFromBlock ( const LineInfo & at, SimNode * s, uint32_t sz, uint32_t asp )
            : SimNode_ReturnAndCopyFromBlock(at,s,sz, asp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
    };

    struct SimNode_ReturnReferenceFromBlock : SimNode_Return {
        SimNode_ReturnReferenceFromBlock ( const LineInfo & at, SimNode * s )
            : SimNode_Return(at,s) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
    };

    // BREAK
    struct SimNode_Break : SimNode {
        SimNode_Break ( const LineInfo & at ) : SimNode(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            context.stopFlags |= EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    // CONTINUE
    struct SimNode_Continue : SimNode {
        SimNode_Continue ( const LineInfo & at ) : SimNode(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            context.stopFlags |= EvalFlags::stopForContinue;
            return v_zero();
        }
    };

    // DEREFERENCE
    template <typename TT>
    struct SimNode_Ref2Value : SimNode {      // &value -> value
        SimNode_Ref2Value ( const LineInfo & at, SimNode * s )
            : SimNode(at), subexpr(s) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *) subexpr->evalPtr(context);
            return cast<TT>::from(*pR);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE (Context & context) override {     \
            auto pR = (CTYPE *)subexpr->evalPtr(context);           \
            return *pR;                                             \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        SimNode * subexpr;
    };

    // POINTER TO REFERENCE (CAST)
    struct SimNode_Ptr2Ref : SimNode {      // ptr -> &value
        DAS_PTR_NODE;
        SimNode_Ptr2Ref ( const LineInfo & at, SimNode * s )
            : SimNode(at), subexpr(s) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            auto ptr = subexpr->evalPtr(context);
            if ( !ptr ) {
                context.throw_error_at(debugInfo,"dereferencing null pointer");
            }
            return ptr;
        }
        SimNode * subexpr;
    };

    // let(a:int?) x = a && 0
    template <typename TT>
    struct SimNode_NullCoalescing : SimNode_Ptr2Ref {
        SimNode_NullCoalescing ( const LineInfo & at, SimNode * s, SimNode * dv )
            : SimNode_Ptr2Ref(at,s), value(dv) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pR = (TT *) subexpr->evalPtr(context);
            return pR ? cast<TT>::from(*pR) : value->eval(context);
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            auto pR = (CTYPE *) subexpr->evalPtr(context);          \
            return pR ? *pR : value->eval##TYPE(context);           \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
        SimNode * value;
    };

    // let(a:int?) x = a && default_a
    struct SimNode_NullCoalescingRef : SimNode_Ptr2Ref {
        DAS_PTR_NODE;
        SimNode_NullCoalescingRef ( const LineInfo & at, SimNode * s, SimNode * dv )
            : SimNode_Ptr2Ref(at,s), value(dv) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            auto ptr = subexpr->evalPtr(context);
            return ptr ? ptr : value->evalPtr(context);
        }
        SimNode * value;
    };

    // NEW
    struct SimNode_New : SimNode {
        DAS_PTR_NODE;
        SimNode_New ( const LineInfo & at, int32_t b )
            : SimNode(at), bytes(b) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            if ( char * ptr = (char *) context.heap.allocate(bytes) ) {
                memset ( ptr, 0, bytes );
                return ptr;
            } else {
                context.throw_error_at(debugInfo,"out of heap");
                return nullptr;
            }
        }
        int32_t     bytes;
    };

    template <bool move>
    struct SimNode_Ascend : SimNode {
        DAS_PTR_NODE;
        SimNode_Ascend ( const LineInfo & at, SimNode * se, uint32_t b, TypeInfo * ti )
            : SimNode(at), subexpr(se), bytes(b), typeInfo(ti) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            if ( char * ptr = (char *) context.heap.allocate(bytes + (typeInfo ? 16 : 0)) ) {
                if ( typeInfo ) {
                    *((TypeInfo **)ptr) = typeInfo;
                    ptr += 16;
                }
                auto src = subexpr->evalPtr(context);
                memcpy ( ptr, src, bytes );
                if ( move ) {
                    memset ( src, 0, bytes );
                }
                return ptr;
            } else {
                context.throw_error_at(debugInfo,"out of heap");
                return nullptr;
            }
        }
        SimNode *   subexpr;
        uint32_t    bytes;
        TypeInfo *  typeInfo;
    };

    template <bool move>
    struct SimNode_AscendAndRef : SimNode {
        DAS_PTR_NODE;
        SimNode_AscendAndRef ( const LineInfo & at, SimNode * se, uint32_t b, uint32_t sp, TypeInfo * ti )
            : SimNode(at), subexpr(se), bytes(b), stackTop(sp), typeInfo(ti) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            if ( char * ptr = (char *) context.heap.allocate(bytes + (typeInfo ? 16 : 0)) ) {
                if ( typeInfo ) {
                    *((TypeInfo **)ptr) = typeInfo;
                    ptr += 16;
                }
                memset ( ptr, 0, bytes );
                char ** pRef = (char **)(context.stack.sp()+stackTop);
                *pRef = ptr;
                subexpr->evalPtr(context);
                return ptr;
            } else {
                context.throw_error_at(debugInfo,"out of heap");
                return nullptr;
            }
        }
        SimNode *   subexpr;
        uint32_t    bytes;
        uint32_t    stackTop;
        TypeInfo *  typeInfo;
    };

    template <int argCount>
    struct SimNode_NewWithInitializer : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_NewWithInitializer ( const LineInfo & at, uint32_t b )
            : SimNode_CallBase(at), bytes(b) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            if ( char * ptr = (char *) context.heap.allocate(bytes) ) {
                vec4f argValues[argCount ? argCount : 1];
                EvalBlock<argCount>::eval(context, arguments, argValues);
                context.callWithCopyOnReturn(fnPtr, argValues, ptr, debugInfo.line);
                return ptr;
            } else {
                context.throw_error_at(debugInfo,"out of heap");
                return nullptr;
            }
        }
        uint32_t     bytes;
    };

    struct SimNode_NewArray : SimNode {
        DAS_PTR_NODE;
        SimNode_NewArray ( const LineInfo & a, SimNode * nn, uint32_t sp, uint32_t c )
            : SimNode(a), newNode(nn), stackTop(sp), count(c) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            auto nodes = (char **)(context.stack.sp() + stackTop);
            for ( uint32_t i=0; i!=count; ++i ) {
                nodes[i] = newNode->evalPtr(context);
            }
            return (char *) nodes;
        }
        SimNode *   newNode;
        uint32_t    stackTop;
        uint32_t    count;
    };

    // CONST-VALUE
    struct SimNode_ConstString : SimNode {
        SimNode_ConstString(const LineInfo & at, char * c)
            : SimNode(at), value(c) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f       eval ( Context & )      override { return cast<char *>::from(value); }
        virtual char *      evalPtr(Context &)      override { return value; }
        char * value;
    };

    // CONST-VALUE
    struct SimNode_ConstValue : SimNode {
        SimNode_ConstValue(const LineInfo & at, vec4f c)
            : SimNode(at), value(c) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f       eval ( Context & )      override { return value; }
        virtual char *      evalPtr(Context &)      override { return valuePtr; }
        virtual int32_t     evalInt(Context &)      override { return valueI; }
        virtual uint32_t    evalUInt(Context &)     override { return valueU; }
        virtual int64_t     evalInt64(Context &)    override { return valueI64; }
        virtual uint64_t    evalUInt64(Context &)   override { return valueU64; }
        virtual float       evalFloat(Context &)    override { return valueF; }
        virtual double      evalDouble(Context &)   override { return valueLF; }
        virtual bool        evalBool(Context &)     override { return valueB; }
        union {
            vec4f       value;
            char *      valuePtr;
            int32_t     valueI;
            uint32_t    valueU;
            int64_t     valueI64;
            uint64_t    valueU64;
            float       valueF;
            double      valueLF;
            bool        valueB;
        };
    };

    struct SimNode_Zero : SimNode_CallBase {
        SimNode_Zero(const LineInfo & at) : SimNode_CallBase(at) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & ) override {
            return v_zero();
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & ) override {            \
            return 0;                                               \
        }
        DAS_EVAL_NODE
#undef EVAL_NODE
    };

    // COPY REFERENCE (int & a = b)
    struct SimNode_CopyReference : SimNode {
        SimNode_CopyReference(const LineInfo & at, SimNode * ll, SimNode * rr)
            : SimNode(at), l(ll), r(rr) {};
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            char  ** pl = (char **) l->evalPtr(context);
            char * pr = r->evalPtr(context);
            *pl = pr;
            return v_zero();
        }
        SimNode * l, * r;
    };

    // COPY VALUE
    template <typename TT>
    struct SimNode_CopyValue : SimNode {
        SimNode_CopyValue(const LineInfo & at, SimNode * ll, SimNode * rr)
            : SimNode(at), l(ll), r(rr) {};
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pl = (TT *) l->evalPtr(context);
            vec4f rr = r->eval(context);
            TT * pr = (TT *) &rr;
            *pl = *pr;
            return v_zero();
        }
        SimNode * l, * r;
    };

    // COPY REFERENCE VALUE
    struct SimNode_CopyRefValue : SimNode {
        SimNode_CopyRefValue(const LineInfo & at, SimNode * ll, SimNode * rr, uint32_t sz)
            : SimNode(at), l(ll), r(rr), size(sz) {};
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode * l, * r;
        uint32_t size;
    };

    template <typename TT>
    struct SimNode_CloneRefValueT : SimNode {
        SimNode_CloneRefValueT(const LineInfo & at, SimNode * ll, SimNode * rr)
            : SimNode(at), l(ll), r(rr) {};
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            TT * pl = (TT *) l->evalPtr(context);
            TT * pr = (TT *) r->evalPtr(context);
            *pl = *pr;
            return v_zero();
        }
        SimNode * l, * r;
    };

    // MOVE REFERENCE VALUE
    struct SimNode_MoveRefValue : SimNode {
        SimNode_MoveRefValue(const LineInfo & at, SimNode * ll, SimNode * rr, uint32_t sz)
            : SimNode(at), l(ll), r(rr), size(sz) {};
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode * l, * r;
        uint32_t size;
    };

    struct SimNode_MakeLocal : SimNode_Block {
        DAS_PTR_NODE;
        SimNode_MakeLocal ( const LineInfo & at, uint32_t sp )
            : SimNode_Block(at), stackTop(sp) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            for ( uint32_t i = 0; i!=total && !context.stopFlags; ) {
                list[i++]->eval(context);
            }
            return context.stack.sp() + stackTop;
        }
        uint32_t stackTop;
    };

    struct SimNode_MakeLocalCMRes : SimNode_Block {
        DAS_PTR_NODE;
        SimNode_MakeLocalCMRes ( const LineInfo & at )
            : SimNode_Block(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            for ( uint32_t i = 0; i!=total && !context.stopFlags; ) {
                list[i++]->eval(context);
            }
            return context.abiCopyOrMoveResult();
        }
    };

    // LET
    struct SimNode_Let : SimNode_Block {
        SimNode_Let ( const LineInfo & at ) : SimNode_Block(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
    };

    // IF-THEN-ELSE (also Cond)
    struct SimNode_IfThenElse : SimNode {
        SimNode_IfThenElse ( const LineInfo & at, SimNode * c, SimNode * t, SimNode * f )
            : SimNode(at), cond(c), if_true(t), if_false(f) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            bool cmp = cond->evalBool(context);
            if ( cmp ) {
                return if_true->eval(context);
            } else {
                return if_false->eval(context);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
                bool cmp = cond->evalBool(context);                 \
                if ( cmp ) {                                        \
                    return if_true->eval##TYPE(context);            \
                } else {                                            \
                    return if_false->eval##TYPE(context);           \
                }                                                   \
            }
        DAS_EVAL_NODE
#undef EVAL_NODE
        SimNode * cond, * if_true, * if_false;
    };

    template <typename TT>
    struct SimNode_IfZeroThenElse : SimNode {
        SimNode_IfZeroThenElse ( const LineInfo & at, SimNode * c, SimNode * t, SimNode * f )
            : SimNode(at), cond(c), if_true(t), if_false(f) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto res = EvalTT<TT>::eval(context,cond);
            if ( res == 0 ) {
                return if_true->eval(context);
            } else {
                return if_false->eval(context);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
                auto res = EvalTT<TT>::eval(context,cond);          \
                if ( res==0 ) {                                     \
                    return if_true->eval##TYPE(context);            \
                } else {                                            \
                    return if_false->eval##TYPE(context);           \
                }                                                   \
            }
        DAS_EVAL_NODE
#undef EVAL_NODE
        SimNode * cond, * if_true, * if_false;
    };

    template <typename TT>
    struct SimNode_IfNotZeroThenElse : SimNode {
        SimNode_IfNotZeroThenElse ( const LineInfo & at, SimNode * c, SimNode * t, SimNode * f )
            : SimNode(at), cond(c), if_true(t), if_false(f) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto res = EvalTT<TT>::eval(context,cond);
            if ( res != 0 ) {
                return if_true->eval(context);
            } else {
                return if_false->eval(context);
            }
        }
#define EVAL_NODE(TYPE,CTYPE)                                       \
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
                auto res = EvalTT<TT>::eval(context,cond);          \
                if ( res!=0 ) {                                     \
                    return if_true->eval##TYPE(context);            \
                } else {                                            \
                    return if_false->eval##TYPE(context);           \
                }                                                   \
            }
        DAS_EVAL_NODE
#undef EVAL_NODE
        SimNode * cond, * if_true, * if_false;
    };

    // IF-THEN
    struct SimNode_IfThen : SimNode {
        SimNode_IfThen ( const LineInfo & at, SimNode * c, SimNode * t )
            : SimNode(at), cond(c), if_true(t) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            bool cmp = cond->evalBool(context);
            if ( cmp ) {
                return if_true->eval(context);
            } else {
                return v_zero();
            }
        }
        SimNode * cond, * if_true;
    };

    template <typename TT>
    struct SimNode_IfZeroThen : SimNode {
        SimNode_IfZeroThen ( const LineInfo & at, SimNode * c, SimNode * t )
            : SimNode(at), cond(c), if_true(t) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto res = EvalTT<TT>::eval(context,cond);
            if ( res==0 ) {
                return if_true->eval(context);
            } else {
                return v_zero();
            }
        }
        SimNode * cond, * if_true;
    };

    template <typename TT>
    struct SimNode_IfNotZeroThen : SimNode {
        SimNode_IfNotZeroThen ( const LineInfo & at, SimNode * c, SimNode * t )
            : SimNode(at), cond(c), if_true(t) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto res = EvalTT<TT>::eval(context,cond);
            if ( res != 0 ) {
                return if_true->eval(context);
            } else {
                return v_zero();
            }
        }
        SimNode * cond, * if_true;
    };


    // WHILE
    struct SimNode_While : SimNode_Block {
        SimNode_While ( const LineInfo & at, SimNode * c )
            : SimNode_Block(at), cond(c) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode * cond;
    };

    template <typename OT, typename Fun, Fun PROP, bool SAFE, typename CTYPE>
    struct SimNode_PropertyImpl : SimNode {
        SimNode_PropertyImpl(const LineInfo & a, SimNode * se)
            : SimNode(a), subexpr(se) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override {
            auto pv = (OT *) subexpr->evalPtr(context);
            if ( !pv ) {
                if ( !SAFE ) {
                    context.throw_error_at(debugInfo,"Property, dereferencing null pointer");
                }
                return v_zero();
            }
            return cast<CTYPE>::from((pv->*PROP)());
        }
        SimNode * subexpr;
    };

    template <typename OT, typename Fun, Fun PROP, bool SAFE, typename CTYPE>
    struct SimNode_PropertyImpl<OT,Fun,PROP,SAFE,CTYPE *> : SimNode {
        DAS_PTR_NODE;
        SimNode_PropertyImpl(const LineInfo & a, SimNode * se) : SimNode(a), subexpr(se) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP("PropertyImpl_Ptr");
            V_SUB_OPT(subexpr);
            V_END();
        }
        __forceinline char * compute(Context & context) {
            auto pv = (OT *) subexpr->evalPtr(context);
            if ( !pv ) {
                if ( !SAFE ) {
                    context.throw_error_at(debugInfo,"Property, dereferencing null pointer");
                }
                return nullptr;
            }
            return (char *) (pv->*PROP)();
        }
        SimNode * subexpr;
    };

    template <typename OT, typename Fun, Fun PROP, bool SAFE, typename CTYPE>
    struct SimNode_PropertyImpl<OT,Fun,PROP,SAFE,CTYPE &> : SimNode {
        DAS_PTR_NODE;
        SimNode_PropertyImpl(const LineInfo & a, SimNode * se) : SimNode(a), subexpr(se) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP("PropertyImpl_Ref");
            V_SUB_OPT(subexpr);
            V_END();
        }
        __forceinline char * compute(Context & context) {
            auto pv = (OT *) subexpr->evalPtr(context);
            if ( !pv ) {
                if ( !SAFE ) {
                    context.throw_error_at(debugInfo,"Property, dereferencing null pointer");
                }
                return nullptr;
            }
            return (char *) &((pv->*PROP)());
        }
        SimNode * subexpr;
    };

#define IMPLEMENT_PROPERTY(TYPE,CTYPE) \
    template <typename OT, typename Fun, Fun PROP, bool SAFE> \
    struct SimNode_PropertyImpl<OT,Fun,PROP,SAFE,CTYPE> : SimNode { \
        DAS_NODE(TYPE,CTYPE); \
        SimNode_PropertyImpl(const LineInfo & a, SimNode * se) : SimNode(a), subexpr(se) {} \
        virtual SimNode * visit ( SimVisitor & vis ) override { \
            V_BEGIN(); \
            V_OP("PropertyImpl_" #TYPE); \
            V_SUB_OPT(subexpr); \
            V_END(); \
        } \
        __forceinline CTYPE compute(Context & context) { \
            auto pv = (OT *) subexpr->evalPtr(context); \
            if ( !pv ) { \
                if ( !SAFE ) { \
                    context.throw_error_at(debugInfo,"Property, dereferencing null pointer"); \
                } \
                return (CTYPE) 0; \
            } \
            return (pv->*PROP)(); \
        } \
        SimNode * subexpr; \
    };

    IMPLEMENT_PROPERTY(Bool,    bool);
    IMPLEMENT_PROPERTY(Int,     int32_t);
    IMPLEMENT_PROPERTY(UInt,    uint32_t);
    IMPLEMENT_PROPERTY(Int64,   int64_t);
    IMPLEMENT_PROPERTY(UInt64,  uint64_t);
    IMPLEMENT_PROPERTY(Float,   float);

    template <typename OT, typename FunT, FunT PROPT, bool SAFET>
    struct SimNode_Property : SimNode_PropertyImpl<OT, FunT, PROPT, SAFET, decltype((((OT *)0)->*PROPT)())> {
        using returnType = decltype((((OT *)0)->*PROPT)());
        SimNode_Property(const LineInfo & a, SimNode * se) :
            SimNode_PropertyImpl<OT,FunT,PROPT,SAFET,returnType>(a,se) {
        }
    };

    struct SimNode_ForWithIteratorBase : SimNode_Block {
        SimNode_ForWithIteratorBase ( const LineInfo & at )
            : SimNode_Block(at) {}
        SimNode * visitFor ( SimVisitor & vis, int total );
        SimNode *   source_iterators[MAX_FOR_ITERATORS];
        uint32_t    stackTop[MAX_FOR_ITERATORS];
    };

    template <int totalCount>
    struct SimNode_ForWithIterator : SimNode_ForWithIteratorBase {
        SimNode_ForWithIterator ( const LineInfo & at )
            : SimNode_ForWithIteratorBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount);
        }
        virtual vec4f eval ( Context & context ) override {
            vec4f * pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (vec4f *)(context.stack.sp() + stackTop[t]);
            }
            Iterator * sources[totalCount] = {};
            for ( int t=0; t!=totalCount; ++t ) {
                vec4f ll = source_iterators[t]->eval(context);
                sources[t] = cast<Iterator *>::to(ll);
            }
            vec4f ph[totalCount];
            bool needLoop = true;
            SimNode ** __restrict tail = list + total;
            for ( int t=0; t!=totalCount; ++t ) {
                needLoop = sources[t]->first(context, (char *)(ph+t)) && needLoop;
                if ( context.stopFlags ) goto loopend;
            }
            if ( !needLoop ) goto loopend;
            for ( int i=0; !context.stopFlags; ++i ) {
                for ( int t=0; t!=totalCount; ++t ){
                    *pi[t] = ph[t];
                }
                for (SimNode ** __restrict body = list; body!=tail; ++body) {
                    (*body)->eval(context);
                    context.stopFlags &= ~EvalFlags::stopForContinue;
                    if (context.stopFlags) goto loopend;
                }
                for ( int t=0; t!=totalCount; ++t ){
                    if ( !sources[t]->next(context, (char *)(ph+t)) ) goto loopend;
                    if ( context.stopFlags ) goto loopend;
                }
            }
        loopend:
            evalFinal(context);
            for ( int t=0; t!=totalCount; ++t ) {
                sources[t]->close(context, (char *)(ph+t));
            }
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForWithIterator<0> : SimNode_ForWithIteratorBase {
        SimNode_ForWithIterator ( const LineInfo & at )
            : SimNode_ForWithIteratorBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForWithIterator_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f eval ( Context & context ) override {
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForWithIterator<1> : SimNode_ForWithIteratorBase {
        SimNode_ForWithIterator ( const LineInfo & at )
            : SimNode_ForWithIteratorBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1);
        }
        virtual vec4f eval ( Context & context ) override {
            vec4f * pi = (vec4f *)(context.stack.sp() + stackTop[0]);
            Iterator * sources;
            vec4f ll = source_iterators[0]->eval(context);
            sources = cast<Iterator *>::to(ll);
            vec4f ph;
            bool needLoop = true;
            SimNode ** __restrict tail = list + total;
            needLoop = sources->first(context, (char *)&ph) && needLoop;
            if ( context.stopFlags || !needLoop) goto loopend;
            for ( int i=0; !context.stopFlags; ++i ) {
                *pi = ph;
                for (SimNode ** __restrict body = list; body!=tail; ++body) {
                    (*body)->eval(context);
                    context.stopFlags &= ~EvalFlags::stopForContinue;
                    if (context.stopFlags) goto loopend;
                }
                if ( !sources->next(context, (char *)&ph) ) goto loopend;
                if ( context.stopFlags ) goto loopend;
            }
        loopend:
            evalFinal(context);
            sources->close(context, (char *)&ph);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <typename TT, typename IterT>
    struct SimNode_AnyIterator : SimNode {
        SimNode_AnyIterator ( const LineInfo & at, SimNode * s )
            : SimNode(at), source(s) { }
        virtual vec4f eval ( Context & context ) override {
            vec4f ll = source->eval(context);
            TT * array = cast<TT *>::to(ll);
            char * iter = context.heap.allocate(sizeof(IterT));
            new (iter) IterT(array);
            return cast<char *>::from(iter);
        }
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            V_OP_TT(AnyIterator);
            V_SUB(source);
            V_END();
        }
        SimNode *   source;
    };

    struct SimNode_Op1 : SimNode {
        SimNode_Op1 ( const LineInfo & at ) : SimNode(at) {}
        SimNode * visitOp1 ( SimVisitor & vis, const char * op, int typeSize, const string & typeName );
        SimNode * x = nullptr;
    };

    struct SimNode_Op2 : SimNode {
        SimNode_Op2 ( const LineInfo & at ) : SimNode(at) {}
        SimNode * visitOp2 ( SimVisitor & vis, const char * op, int typeSize, const string & typeName );
        SimNode * l = nullptr;
        SimNode * r = nullptr;
    };

    struct Sim_BoolAnd : SimNode_Op2 {
        DAS_BOOL_NODE;
        Sim_BoolAnd ( const LineInfo & at ) : SimNode_Op2(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitOp2(vis, "BoolAnd", sizeof(bool), "bool");
        }
        __forceinline bool compute ( Context & context ) {
            if ( !l->evalBool(context) ) {      // if not left, then false
                return false;
            } else {
                return r->evalBool(context);    // if left, then right
            }
        }
    };

    struct Sim_BoolOr : SimNode_Op2 {
        DAS_BOOL_NODE;
        Sim_BoolOr ( const LineInfo & at ) : SimNode_Op2(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitOp2(vis, "BoolOr", sizeof(bool), "bool");
        }
        __forceinline bool compute ( Context & context )  {
            if ( l->evalBool(context) ) {       // if left, then true
                return true;
            } else {
                return r->evalBool(context);    // if not left, then right
            }
        }
    };

#if !_TARGET_64BIT && !defined(__clang__) && (_MSC_VER <= 1900)
#define _msc_inline_bug __declspec(noinline)
#else
#define _msc_inline_bug __forceinline
#endif

#define DEFINE_POLICY(CALL) template <typename SimPolicy> struct Sim_##CALL;

#define IMPLEMENT_OP1_POLICY_BASE(CALL,TYPE,CTYPE,INLINE)               \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op1 {                           \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op1(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp1(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        INLINE CTYPE compute ( Context & context ) {                    \
            auto val = x->eval##TYPE(context);                          \
            return SimPolicy<CTYPE>::CALL(val,context);                 \
        }                                                               \
    };

#define IMPLEMENT_OP1_POLICY(CALL,TYPE,CTYPE) IMPLEMENT_OP1_POLICY_BASE(CALL,TYPE,CTYPE,__forceinline)

#define IMPLEMENT_OP1_FUNCTION_POLICY(CALL,TYPE,CTYPE)                  \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_CallBase {                      \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}    \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp1(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        __forceinline CTYPE compute ( Context & context ) {             \
            auto val = arguments[0]->eval##TYPE(context);               \
            return SimPolicy<CTYPE>::CALL(val,context);                 \
        }                                                               \
    };

#define IMPLEMENT_OP1_FUNCTION_POLICY_EX(CALL,TYPE,CTYPE,ATYPE,ACTYPE)  \
    template <>                                                         \
    struct Sim_##CALL <ACTYPE> : SimNode_CallBase {                     \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}    \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp1(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        __forceinline CTYPE compute ( Context & context ) {             \
            auto val = arguments[0]->eval##ATYPE(context);              \
            return SimPolicy<ACTYPE>::CALL(val,context);                \
        }                                                               \
    };

#define IMPLEMENT_OP1_SET_POLICY_BASE(CALL,TYPE,CTYPE,INLINE)           \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op1 {                           \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op1(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp1(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        INLINE CTYPE compute ( Context & context ) {                    \
            auto val = (CTYPE *) x->evalPtr(context);                   \
            return SimPolicy<CTYPE>::CALL(*val,context);                \
        }                                                               \
    };

#define IMPLEMENT_OP1_SET_POLICY(CALL,TYPE,CTYPE) IMPLEMENT_OP1_SET_POLICY_BASE(CALL,TYPE,CTYPE, __forceinline)

#define IMPLEMENT_OP1_EVAL_POLICY(CALL,CTYPE)                           \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op1 {                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op1(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp1(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        virtual vec4f eval ( Context & context ) override {             \
            auto val = x->eval(context);                                \
            return cast_result(SimPolicy<CTYPE>::CALL(val,context));    \
        }                                                               \
    };

#define IMPLEMENT_OP1_EVAL_FUNCTION_POLICY(CALL,CTYPE)                  \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_CallBase {                      \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}    \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp1(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        virtual vec4f eval ( Context & context ) override {             \
            auto val = arguments[0]->eval(context);                     \
            return cast_result(SimPolicy<CTYPE>::CALL(val,context));    \
        }                                                               \
    };

#define DEFINE_OP1_NUMERIC_INTEGER(CALL)                \
    IMPLEMENT_OP1_POLICY(CALL,Int,int32_t);             \
    IMPLEMENT_OP1_POLICY(CALL,UInt,uint32_t);           \
    IMPLEMENT_OP1_POLICY_BASE(CALL,Int64,int64_t,_msc_inline_bug);           \
    IMPLEMENT_OP1_POLICY_BASE(CALL,UInt64,uint64_t,_msc_inline_bug);         \

#define DEFINE_OP1_NUMERIC(CALL);                       \
    DEFINE_OP1_NUMERIC_INTEGER(CALL);                   \
    IMPLEMENT_OP1_POLICY(CALL,Double,double);           \
    IMPLEMENT_OP1_POLICY(CALL,Float,float);

#define DEFINE_OP1_SET_NUMERIC_INTEGER(CALL)            \
    IMPLEMENT_OP1_SET_POLICY(CALL,Int,int32_t);         \
    IMPLEMENT_OP1_SET_POLICY(CALL,UInt,uint32_t);       \
    IMPLEMENT_OP1_SET_POLICY_BASE(CALL,Int64,int64_t,_msc_inline_bug);       \
    IMPLEMENT_OP1_SET_POLICY_BASE(CALL,UInt64,uint64_t,_msc_inline_bug);     \

#define DEFINE_OP1_SET_NUMERIC(CALL);                   \
    DEFINE_OP1_SET_NUMERIC_INTEGER(CALL);               \
    IMPLEMENT_OP1_SET_POLICY(CALL,Double,double);       \
    IMPLEMENT_OP1_SET_POLICY(CALL,Float,float);

#define IMPLEMENT_OP2_POLICY_BASE(CALL,TYPE,CTYPE, INLINE)              \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op2 {                           \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op2(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        INLINE CTYPE compute ( Context & context ) {                    \
            auto lv = l->eval##TYPE(context);                           \
            auto rv = r->eval##TYPE(context);                           \
            return SimPolicy<CTYPE>::CALL(lv,rv,context);               \
        }                                                               \
    };

#define IMPLEMENT_OP2_POLICY(CALL,TYPE,CTYPE) IMPLEMENT_OP2_POLICY_BASE(CALL,TYPE,CTYPE, __forceinline)

#define IMPLEMENT_OP2_FUNCTION_POLICY(CALL,TYPE,CTYPE)                  \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_CallBase {                      \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}    \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        __forceinline CTYPE compute ( Context & context ) {             \
            auto lv = arguments[0]->eval##TYPE(context);                \
            auto rv = arguments[1]->eval##TYPE(context);                \
            return SimPolicy<CTYPE>::CALL(lv,rv,context);               \
        }                                                               \
    };

#define IMPLEMENT_OP2_SET_POLICY_BASE(CALL,TYPE,CTYPE, INLINE)          \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op2 {                           \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op2(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        INLINE CTYPE compute ( Context & context ) {                    \
            auto lv = (CTYPE *) l->evalPtr(context);                    \
            auto rv = r->eval##TYPE(context);                           \
            SimPolicy<CTYPE>::CALL(*lv,rv,context);                     \
            return CTYPE();                                             \
        }                                                               \
    };

#define IMPLEMENT_OP2_SET_POLICY(CALL,TYPE,CTYPE) IMPLEMENT_OP2_SET_POLICY_BASE(CALL,TYPE,CTYPE, __forceinline)

#define IMPLEMENT_OP2_BOOL_POLICY(CALL,TYPE,CTYPE)                      \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op2 {                           \
        DAS_BOOL_NODE;                                                  \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op2(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        __forceinline bool compute ( Context & context ) {              \
            auto lv = l->eval##TYPE(context);                           \
            auto rv = r->eval##TYPE(context);                           \
            return SimPolicy<CTYPE>::CALL(lv,rv,context);               \
        }                                                               \
    };

#define IMPLEMENT_OP2_EVAL_POLICY(CALL,CTYPE)                           \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op2 {                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op2(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        virtual vec4f eval ( Context & context ) override {             \
            auto lv = l->eval(context);                                 \
            auto rv = r->eval(context);                                 \
            return cast_result(SimPolicy<CTYPE>::CALL(lv,rv,context));  \
        }                                                               \
    };

#define IMPLEMENT_OP2_EVAL_FUNCTION_POLICY(CALL,CTYPE)                  \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_CallBase {                      \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}    \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        virtual vec4f eval ( Context & context ) override {             \
            auto lv = arguments[0]->eval(context);                      \
            auto rv = arguments[1]->eval(context);                      \
            return cast_result(SimPolicy<CTYPE>::CALL(lv,rv,context));  \
        }                                                               \
    };

#define IMPLEMENT_OP2_EVAL_SET_POLICY(CALL,CTYPE)                       \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op2 {                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op2(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        virtual vec4f eval ( Context & context ) override {             \
            auto lv = l->evalPtr(context);                              \
            auto rv = r->eval(context);                                 \
            SimPolicy<CTYPE>::CALL(lv,rv,context);                      \
            return v_zero();                                            \
        }                                                               \
    };

#define IMPLEMENT_OP2_EVAL_BOOL_POLICY(CALL,CTYPE)                      \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_Op2 {                           \
        DAS_BOOL_NODE;                                                  \
        Sim_##CALL ( const LineInfo & at ) : SimNode_Op2(at) {}         \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp2(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        __forceinline bool compute ( Context & context ) {              \
            auto lv = l->eval(context);                                 \
            auto rv = r->eval(context);                                 \
            return SimPolicy<CTYPE>::CALL(lv,rv,context);               \
        }                                                               \
    };

#define DEFINE_OP2_NUMERIC_INTEGER(CALL)                \
    IMPLEMENT_OP2_POLICY(CALL,Int,int32_t);             \
    IMPLEMENT_OP2_POLICY(CALL,UInt,uint32_t);           \
    IMPLEMENT_OP2_POLICY_BASE(CALL,Int64,int64_t, _msc_inline_bug);           \
    IMPLEMENT_OP2_POLICY_BASE(CALL,UInt64,uint64_t, _msc_inline_bug);         \

#define DEFINE_OP2_NUMERIC(CALL);                       \
    DEFINE_OP2_NUMERIC_INTEGER(CALL);                   \
    IMPLEMENT_OP2_POLICY(CALL,Double,double);           \
    IMPLEMENT_OP2_POLICY(CALL,Float,float);

#define DEFINE_OP2_FUNCTION_NUMERIC_INTEGER(CALL)                \
    IMPLEMENT_OP2_FUNCTION_POLICY(CALL,Int,int32_t);             \
    IMPLEMENT_OP2_FUNCTION_POLICY(CALL,UInt,uint32_t);           \
    IMPLEMENT_OP2_FUNCTION_POLICY(CALL,Int64,int64_t);           \
    IMPLEMENT_OP2_FUNCTION_POLICY(CALL,UInt64,uint64_t);         \

#define DEFINE_OP2_FUNCTION_NUMERIC(CALL);                       \
    DEFINE_OP2_FUNCTION_NUMERIC_INTEGER(CALL);                   \
    IMPLEMENT_OP2_FUNCTION_POLICY(CALL,Double,double);           \
    IMPLEMENT_OP2_FUNCTION_POLICY(CALL,Float,float);

#define DEFINE_OP2_BOOL_NUMERIC_INTEGER(CALL)           \
    IMPLEMENT_OP2_BOOL_POLICY(CALL,Int,int32_t);        \
    IMPLEMENT_OP2_BOOL_POLICY(CALL,UInt,uint32_t);      \
    IMPLEMENT_OP2_BOOL_POLICY(CALL,Int64,int64_t);      \
    IMPLEMENT_OP2_BOOL_POLICY(CALL,UInt64,uint64_t);    \

#define DEFINE_OP2_BOOL_NUMERIC(CALL);                  \
    DEFINE_OP2_BOOL_NUMERIC_INTEGER(CALL);              \
    IMPLEMENT_OP2_BOOL_POLICY(CALL,Double,double);      \
    IMPLEMENT_OP2_BOOL_POLICY(CALL,Float,float);

#define DEFINE_OP2_SET_NUMERIC_INTEGER(CALL)            \
    IMPLEMENT_OP2_SET_POLICY(CALL,Int,int32_t);         \
    IMPLEMENT_OP2_SET_POLICY(CALL,UInt,uint32_t);       \
    IMPLEMENT_OP2_SET_POLICY_BASE(CALL,Int64,int64_t,_msc_inline_bug);       \
    IMPLEMENT_OP2_SET_POLICY_BASE(CALL,UInt64,uint64_t,_msc_inline_bug);     \

#define DEFINE_OP2_SET_NUMERIC(CALL);                   \
    DEFINE_OP2_SET_NUMERIC_INTEGER(CALL);               \
    IMPLEMENT_OP2_SET_POLICY(CALL,Double,double);       \
    IMPLEMENT_OP2_SET_POLICY(CALL,Float,float);

#define DEFINE_OP2_BASIC_POLICY(TYPE,CTYPE)             \
    IMPLEMENT_OP2_BOOL_POLICY(Equ, TYPE, CTYPE);        \
    IMPLEMENT_OP2_BOOL_POLICY(NotEqu, TYPE, CTYPE);

#define DEFINE_OP2_EVAL_BASIC_POLICY(CTYPE)             \
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ, CTYPE);         \
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu, CTYPE);

#define DEFINE_OP2_EVAL_ORDERED_POLICY(CTYPE)           \
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(LessEqu,CTYPE);      \
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(GtEqu,CTYPE);        \
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Less,CTYPE);         \
    IMPLEMENT_OP2_EVAL_BOOL_POLICY(Gt,CTYPE);

#define DEFINE_OP2_EVAL_GROUPBYADD_POLICY(CTYPE)        \
    IMPLEMENT_OP2_EVAL_POLICY(Add, CTYPE);              \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetAdd, CTYPE);

#define DEFINE_OP2_EVAL_NUMERIC_POLICY(CTYPE)           \
    DEFINE_OP2_EVAL_GROUPBYADD_POLICY(CTYPE);           \
    IMPLEMENT_OP1_EVAL_POLICY(Unp, CTYPE);              \
    IMPLEMENT_OP1_EVAL_POLICY(Unm, CTYPE);              \
    IMPLEMENT_OP2_EVAL_POLICY(Sub, CTYPE);              \
    IMPLEMENT_OP2_EVAL_POLICY(Div, CTYPE);              \
    IMPLEMENT_OP2_EVAL_POLICY(Mul, CTYPE);              \
    IMPLEMENT_OP2_EVAL_POLICY(Mod, CTYPE);              \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetSub, CTYPE);       \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetDiv, CTYPE);       \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetMul, CTYPE);       \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetMod, CTYPE);

#define DEFINE_OP2_EVAL_VECNUMERIC_POLICY(CTYPE)        \
    IMPLEMENT_OP2_EVAL_POLICY(DivVecScal, CTYPE);       \
    IMPLEMENT_OP2_EVAL_POLICY(MulVecScal, CTYPE);       \
    IMPLEMENT_OP2_EVAL_POLICY(DivScalVec, CTYPE);       \
    IMPLEMENT_OP2_EVAL_POLICY(MulScalVec, CTYPE);       \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetDivScal, CTYPE);   \
    IMPLEMENT_OP2_EVAL_SET_POLICY(SetMulScal, CTYPE);

#define DEFINE_VECTOR_POLICY(CTYPE)             \
    DEFINE_OP2_EVAL_BASIC_POLICY(CTYPE);        \
    DEFINE_OP2_EVAL_NUMERIC_POLICY(CTYPE);      \
    DEFINE_OP2_EVAL_VECNUMERIC_POLICY(CTYPE);

#define IMPLEMENT_OP3_FUNCTION_POLICY(CALL,TYPE,CTYPE)                  \
    template <>                                                         \
    struct Sim_##CALL <CTYPE> : SimNode_CallBase {                      \
        DAS_NODE(TYPE,CTYPE);                                           \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}    \
        virtual SimNode * visit ( SimVisitor & vis ) override {         \
            return visitOp3(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                               \
        __forceinline CTYPE compute ( Context & context ) {             \
            auto a0 = arguments[0]->eval##TYPE(context);                \
            auto a1 = arguments[1]->eval##TYPE(context);                \
            auto a2 = arguments[2]->eval##TYPE(context);                \
            return SimPolicy<CTYPE>::CALL(a0,a1,a2,context);            \
        }                                                               \
    };

#define IMPLEMENT_OP3_EVAL_FUNCTION_POLICY(CALL,CTYPE)                      \
    template <>                                                             \
    struct Sim_##CALL <CTYPE> : SimNode_CallBase {                          \
        Sim_##CALL ( const LineInfo & at ) : SimNode_CallBase(at) {}        \
        virtual SimNode * visit ( SimVisitor & vis ) override {             \
            return visitOp3(vis, #CALL, sizeof(CTYPE), typeName<CTYPE>::name());    \
        }                                                                   \
        virtual vec4f eval ( Context & context ) override {                 \
            auto a0 = arguments[0]->eval(context);                          \
            auto a1 = arguments[1]->eval(context);                          \
            auto a2 = arguments[2]->eval(context);                          \
            return cast_result(SimPolicy<CTYPE>::CALL(a0,a1,a2,context));   \
        }                                                                   \
    };

    // unary
    DEFINE_POLICY(Unp);
    DEFINE_POLICY(Unm);
    DEFINE_POLICY(Inc);
    DEFINE_POLICY(Dec);
    DEFINE_POLICY(IncPost);
    DEFINE_POLICY(DecPost);
    DEFINE_POLICY(BinNot);
    DEFINE_POLICY(BoolNot);
    // binary
    // +,-,*,/,%
    DEFINE_POLICY(Add);
    DEFINE_POLICY(Sub);
    DEFINE_POLICY(Mul);
    DEFINE_POLICY(Div);
    DEFINE_POLICY(Mod);
    DEFINE_POLICY(SetAdd);
    DEFINE_POLICY(SetSub);
    DEFINE_POLICY(SetMul);
    DEFINE_POLICY(SetDiv);
    DEFINE_POLICY(SetMod);
    // comparisons
    DEFINE_POLICY(Equ);
    DEFINE_POLICY(NotEqu);
    DEFINE_POLICY(LessEqu);
    DEFINE_POLICY(GtEqu);
    DEFINE_POLICY(Less);
    DEFINE_POLICY(Gt);
    DEFINE_POLICY(Min);
    DEFINE_POLICY(Max);
    // binary and, or, xor
    DEFINE_POLICY(BinAnd);
    DEFINE_POLICY(BinOr);
    DEFINE_POLICY(BinXor);
    DEFINE_POLICY(BinShl);
    DEFINE_POLICY(BinShr);
    DEFINE_POLICY(BinRotl);
    DEFINE_POLICY(BinRotr);
    DEFINE_POLICY(SetBinAnd);
    DEFINE_POLICY(SetBinOr);
    DEFINE_POLICY(SetBinXor);
    DEFINE_POLICY(SetBinShl);
    DEFINE_POLICY(SetBinShr);
    DEFINE_POLICY(SetBinRotl);
    DEFINE_POLICY(SetBinRotr);
    // boolean and, or, xor
    DEFINE_POLICY(SetBoolAnd);
    DEFINE_POLICY(SetBoolOr);
    DEFINE_POLICY(SetBoolXor);
    DEFINE_POLICY(BoolXor);
    // vector*scalar, scalar*vector
    DEFINE_POLICY(DivVecScal);
    DEFINE_POLICY(MulVecScal);
    DEFINE_POLICY(DivScalVec);
    DEFINE_POLICY(MulScalVec);
    DEFINE_POLICY(SetBoolAnd);
    DEFINE_POLICY(SetBoolOr);
    DEFINE_POLICY(SetBoolXor);
    DEFINE_POLICY(SetDivScal);
    DEFINE_POLICY(SetMulScal);
}

#include "daScript/simulate/simulate_visit.h"
#include "daScript/simulate/simulate_visit_op_undef.h"

