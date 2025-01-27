#pragma once

#include <climits>
#include "daScript/simulate/simulate.h"
#include "daScript/misc/arraytype.h"

#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    // AT (INDEX)
    struct SimNode_ArrayAt : SimNode {
        DAS_PTR_NODE;
        SimNode_ArrayAt ( const LineInfo & at, SimNode * ll, SimNode * rr, uint32_t sz, uint32_t o)
            : SimNode(at), l(ll), r(rr), stride(sz), offset(o) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline char * compute ( Context & context ) {
            Array * pA = (Array *) l->evalPtr(context);
            vec4f rr = r->eval(context);
            uint32_t idx = cast<uint32_t>::to(rr);
            if ( idx >= pA->size ) {
                context.throw_error_at(debugInfo,"array index out of range");
                return nullptr;
            } else {
                return pA->data + idx*stride + offset;
            }
        }
        SimNode * l, * r;
        uint32_t stride, offset;
    };

    template <typename TT>
    struct SimNode_ArrayAtR2V : SimNode_ArrayAt {
        SimNode_ArrayAtR2V ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t strd, uint32_t o )
            : SimNode_ArrayAt(at,rv,idx,strd,o) {}
        SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP_TT(ArrayAtR2V);
            V_SUB(l);
            V_SUB(r);
            V_ARG(stride);
            V_ARG(offset);
            V_END();
        }
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

    struct GoodArrayIterator : Iterator {
        GoodArrayIterator ( Array * arr, uint32_t st ) : array(arr), stride(st) {}
        virtual bool first ( Context & context, char * value ) override;
        virtual bool next  ( Context & context, char * value ) override;
        virtual void close ( Context & context, char * value ) override;
        Array *     array;
        uint32_t    stride;
        char *      array_end = nullptr;
    };

    struct SimNode_GoodArrayIterator : SimNode {
        SimNode_GoodArrayIterator ( const LineInfo & at, SimNode * s, uint32_t st )
            : SimNode(at), source(s), stride(st) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode *   source;
        uint32_t    stride;
    };

    struct FixedArrayIterator : Iterator {
        FixedArrayIterator ( char * d, uint32_t sz, uint32_t st ) : data(d), size(sz), stride(st) {}
        virtual bool first ( Context & context, char * value ) override;
        virtual bool next  ( Context & context, char * value ) override;
        virtual void close ( Context & context, char * value ) override;
        char *      data;
        uint32_t    size;
        uint32_t    stride;
        char *      fixed_array_end = nullptr;
    };

    struct SimNode_FixedArrayIterator : SimNode {
        SimNode_FixedArrayIterator ( const LineInfo & at, SimNode * s, uint32_t sz, uint32_t st )
            : SimNode(at), source(s), size(sz), stride(st) { }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        SimNode *   source;
        uint32_t    size;
        uint32_t    stride;
    };

    template <int totalCount>
    struct SimNode_ForGoodArray : public SimNode_ForBase {
        SimNode_ForGoodArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForGoodArray");
        }
        virtual vec4f eval ( Context & context ) override {
            Array * __restrict pha[totalCount];
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pha[t] = cast<Array *>::to(sources[t]->eval(context));
                array_lock(context, *pha[t]);
                ph[t]  = pha[t]->data;
            }
            char ** __restrict pi[totalCount];
            int szz = INT_MAX;
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
                szz = das::min(szz, int(pha[t]->size));
            }
            SimNode ** __restrict tail = list + total;
            for (int i = 0; i!=szz; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                for (SimNode ** __restrict body = list; body!=tail; ++body) {
                    (*body)->eval(context);
                    context.stopFlags &= ~EvalFlags::stopForContinue;
                    if (context.stopFlags) goto loopend;
                }
            }
        loopend:;
            for ( int t=0; t!=totalCount; ++t ) {
                array_unlock(context, *pha[t]);
            }
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray<0> : public SimNode_ForBase {
        SimNode_ForGoodArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForGoodArray_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f eval ( Context & context ) override {
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray<1> : public SimNode_ForBase {
        SimNode_ForGoodArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForGoodArray");
        }
        virtual vec4f eval ( Context & context ) override {
            Array * __restrict pha;
            char * __restrict ph;
            pha = cast<Array *>::to(sources[0]->eval(context));
            array_lock(context, *pha);
            ph = pha->data;
            char ** __restrict pi;
            int szz = int(pha->size);
            pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode ** __restrict tail = list + total;
            for (int i = 0; i!=szz; ++i) {
                *pi = ph;
                ph += stride;
                for (SimNode ** __restrict body = list; body!=tail; ++body) {
                    (*body)->eval(context);
                    context.stopFlags &= ~EvalFlags::stopForContinue;
                    if (context.stopFlags) goto loopend;
                }
            }
        loopend:;
            evalFinal(context);
            array_unlock(context, *pha);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <int totalCount>
    struct SimNode_ForGoodArray1 : public SimNode_ForBase {
        SimNode_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForGoodArray1");
        }
        virtual vec4f eval ( Context & context ) override {
            Array * __restrict pha[totalCount];
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pha[t] = cast<Array *>::to(sources[t]->eval(context));
                array_lock(context, *pha[t]);
                ph[t]  = pha[t]->data;
            }
            char ** __restrict pi[totalCount];
            int szz = INT_MAX;
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
                szz = das::min(szz, int(pha[t]->size));
            }
            SimNode * __restrict body = list[0];
            for (int i = 0; i!=szz && !context.stopFlags; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                body->eval(context);
                context.stopFlags &= ~EvalFlags::stopForContinue;
            }
            for ( int t=0; t!=totalCount; ++t ) {
                array_unlock(context, *pha[t]);
            }
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray1<0> : public SimNode_ForBase {
        SimNode_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForGoodArray1_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f eval ( Context & context ) override {
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForGoodArray1<1> : public SimNode_ForBase {
        SimNode_ForGoodArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForGoodArray1");
        }
        virtual vec4f eval ( Context & context ) override {
            Array * __restrict pha;
            char * __restrict ph;
            pha = cast<Array *>::to(sources[0]->eval(context));
            array_lock(context, *pha);
            ph = pha->data;
            char ** __restrict pi;
            int szz = int(pha->size);
            pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode * __restrict body = list[0];
            for (int i = 0; i!=szz && !context.stopFlags; ++i) {
                *pi = ph;
                ph += stride;
                body->eval(context);
                context.stopFlags &= ~EvalFlags::stopForContinue;
            }
            evalFinal(context);
            array_unlock(context, *pha);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    // FOR
    template <int totalCount>
    struct SimNode_ForFixedArray : SimNode_ForBase {
        SimNode_ForFixedArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForFixedArray");
        }
        virtual vec4f eval ( Context & context ) override {
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                ph[t] = cast<char *>::to(sources[t]->eval(context));
            }
            char ** __restrict pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
            }
            SimNode ** __restrict tail = list + total;
            for (uint32_t i = 0; i != size; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                for (SimNode ** __restrict body = list; body!=tail; ++body) {
                    (*body)->eval(context);
                    context.stopFlags &= ~EvalFlags::stopForContinue;
                    if (context.stopFlags) goto loopend;
                }
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray<0> : SimNode_ForBase {
        SimNode_ForFixedArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForFixedArray_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f eval ( Context & context ) override {
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray<1> : SimNode_ForBase {
        SimNode_ForFixedArray ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForFixedArray");
        }
        virtual vec4f eval ( Context & context ) override {
            char * __restrict ph = cast<char *>::to(sources[0]->eval(context));
            char ** __restrict pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode ** __restrict tail = list + total;
            for (uint32_t i = 0; i != size; ++i) {
                *pi = ph;
                ph += stride;
                for (SimNode ** __restrict body = list; body!=tail; ++body) {
                    (*body)->eval(context);
                    context.stopFlags &= ~EvalFlags::stopForContinue;
                    if (context.stopFlags) goto loopend;
                }
            }
        loopend:;
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    // FOR
    template <int totalCount>
    struct SimNode_ForFixedArray1 : SimNode_ForBase {
        SimNode_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, totalCount, "ForFixedArray1");
        }
        virtual vec4f eval ( Context & context ) override {
            char * __restrict ph[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                ph[t] = cast<char *>::to(sources[t]->eval(context));
            }
            char ** __restrict pi[totalCount];
            for ( int t=0; t!=totalCount; ++t ) {
                pi[t] = (char **)(context.stack.sp() + stackTop[t]);
            }
            SimNode * __restrict body = list[0];
            for (uint32_t i = 0; i != size && !context.stopFlags; ++i) {
                for (int t = 0; t != totalCount; ++t) {
                    *pi[t] = ph[t];
                    ph[t] += strides[t];
                }
                body->eval(context);
                context.stopFlags &= ~EvalFlags::stopForContinue;
            }
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray1<0> : SimNode_ForBase {
        SimNode_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN_CR();
            V_OP(ForFixedArray1_0);
            V_FINAL();
            V_END();
        }
        virtual vec4f eval ( Context & context ) override {
            evalFinal(context);
            return v_zero();
        }
    };

    template <>
    struct SimNode_ForFixedArray1<1> : SimNode_ForBase {
        SimNode_ForFixedArray1 ( const LineInfo & at ) : SimNode_ForBase(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override {
            return visitFor(vis, 1, "ForFixedArray1");
        }
        virtual vec4f eval ( Context & context ) override {
            char * __restrict ph = cast<char *>::to(sources[0]->eval(context));
            char ** __restrict pi = (char **)(context.stack.sp() + stackTop[0]);
            auto stride = strides[0];
            SimNode * __restrict body = list[0];
            for (uint32_t i = 0; i != size && !context.stopFlags; ++i) {
                *pi = ph;
                ph += stride;
                body->eval(context);
                context.stopFlags &= ~EvalFlags::stopForContinue;
            }
            evalFinal(context);
            context.stopFlags &= ~EvalFlags::stopForBreak;
            return v_zero();
        }
    };

    struct SimNode_DeleteArray : SimNode_Delete {
        SimNode_DeleteArray ( const LineInfo & a, SimNode * s, uint32_t t, uint32_t st )
            : SimNode_Delete(a,s,t), stride(st) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        virtual vec4f eval ( Context & context ) override;
        uint32_t stride;
    };
}

#include "daScript/simulate/simulate_visit_op_undef.h"
