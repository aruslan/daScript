#pragma once

#include "daScript/ast/ast.h"
#include "daScript/simulate/interop.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/simulate_nodes.h"

#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    #define DAS_BIND_MANAGED_FIELD(FIELDNAME)   DAS_BIND_FIELD(ManagedType,FIELDNAME)
    #define DAS_BIND_MANAGED_PROP(FIELDNAME)    DAS_BIND_PROP(ManagedType,FIELDNAME)

    struct DasStringTypeAnnotation : TypeAnnotation {
        DasStringTypeAnnotation() : TypeAnnotation("das_string","das::string") {}
        virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
        virtual bool isRefType() const override { return true; }
        virtual bool isLocal() const override { return false; }
        virtual void walk ( DataWalker & dw, void * p ) override {
            auto pstr = (string *)p;
            if (dw.reading) {
                char * pss = nullptr;
                dw.String(pss);
                *pstr = pss;
            } else {
                char * pss = (char *) pstr->c_str();
                dw.String(pss);
            }
        }
        virtual bool canCopy() const override { return false; }
        virtual bool canMove() const override { return false; }
        virtual bool canClone() const override { return true; }
        virtual size_t getAlignOf() const override { return alignof(string);}
        virtual size_t getSizeOf() const override { return sizeof(string);}
        virtual SimNode * simulateClone ( Context & context, const LineInfo & at, SimNode * l, SimNode * r ) const override {
            return context.code->makeNode<SimNode_CloneRefValueT<string>>(at, l, r);
        }
    };

    template <typename OT>
    struct ManagedStructureAlignof {static constexpr size_t alignment = alignof(OT); } ;//we use due to MSVC inability to work with abstarct classes

    template <typename OT, bool canNewAndDelete = true>
    struct ManagedStructureAnnotation ;

    template <typename OT>
    struct ManagedStructureAnnotation<OT,false>  : TypeAnnotation {
        typedef OT ManagedType;
        enum class FactoryNodeType {
            getField
        ,   getFieldR2V
        ,   safeGetField
        ,   safeGetFieldPtr
        };
        struct StructureField {
            string      name;
            string      cppName;
            TypeDeclPtr decl;
            uint32_t    offset;
            function<SimNode * (FactoryNodeType,Context &,const LineInfo &, const ExpressionPtr &)>   factory;
        };
        ManagedStructureAnnotation (const string & n, ModuleLibrary & ml, const string & cpn = "" )
            : TypeAnnotation(n,cpn), mlib(&ml) { }
        virtual void seal( Module * m ) override {
            TypeAnnotation::seal(m);
            mlib = nullptr;
        }
        virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
        virtual size_t getSizeOf() const override { return sizeof(ManagedType); }
        virtual size_t getAlignOf() const override { return ManagedStructureAlignof<ManagedType>::alignment; }
        virtual bool isRefType() const override { return true; }
        virtual bool canMove() const override { return false; }
        virtual bool canCopy() const override { return false; }
        virtual bool isLocal() const override { return false; }
        virtual TypeDeclPtr makeFieldType ( const string & na ) const override {
            auto it = fields.find(na);
            if ( it!=fields.end() ) {
                auto t = make_shared<TypeDecl>(*it->second.decl);
                if ( it->second.offset != -1U ) {
                    t->ref = true;
                }
                return t;
            } else {
                return nullptr;
            }
        }
        virtual TypeDeclPtr makeSafeFieldType ( const string & na ) const override {
            auto it = fields.find(na);
            if ( it!=fields.end() && it->second.offset!=-1U ) {
                return make_shared<TypeDecl>(*it->second.decl);
            } else {
                return nullptr;
            }
        }
        virtual SimNode * simulateGetField ( const string & na, Context & context,
                                            const LineInfo & at, const ExpressionPtr & value ) const override {
            auto it = fields.find(na);
            if ( it!=fields.end() ) {
                return it->second.factory(FactoryNodeType::getField,context,at,value);
            } else {
                return nullptr;
            }
        }
        virtual SimNode * simulateGetFieldR2V ( const string & na, Context & context,
                                               const LineInfo & at, const ExpressionPtr & value ) const override {
            auto it = fields.find(na);
            if ( it!=fields.end() ) {
                auto itT = it->second.decl;
                return it->second.factory(FactoryNodeType::getFieldR2V,context,at,value);
            } else {
                return nullptr;
            }
        }
        virtual SimNode * simulateSafeGetField ( const string & na, Context & context,
                                                const LineInfo & at, const ExpressionPtr & value ) const override {
            auto it = fields.find(na);
            if ( it!=fields.end() ) {
                return it->second.factory(FactoryNodeType::safeGetField,context,at,value);
            } else {
                return nullptr;
            }
        };
        virtual SimNode * simulateSafeGetFieldPtr ( const string & na, Context & context,
                                                   const LineInfo & at, const ExpressionPtr & value ) const override {
            auto it = fields.find(na);
            if ( it!=fields.end() ) {
                return it->second.factory(FactoryNodeType::safeGetFieldPtr,context,at,value);
            } else {
                return nullptr;
            }
        };
        template <typename FunT, FunT PROP>
        void addProperty ( const string & na, const string & cppNa="" ) {
            auto & field = fields[na];
            if ( field.decl ) {
                DAS_FATAL_LOG("structure field %s already exist in structure %s\n", na.c_str(), name.c_str() );
                DAS_FATAL_ERROR;
            }
            using resultType = decltype((((ManagedType *)0)->*PROP)());
            field.cppName = (cppNa.empty() ? na : cppNa) + "()";
            field.decl = makeType<resultType>(*mlib);
            DAS_ASSERTF ( !(field.decl->isRefType() && !field.decl->ref), "property can't be CMRES, in %s", field.decl->describe().c_str() );
            field.offset = -1U;
            field.factory = [](FactoryNodeType nt,Context & context,const LineInfo & at, const ExpressionPtr & value) -> SimNode * {
                switch ( nt ) {
                    case FactoryNodeType::getField:
                        return context.code->makeNode<SimNode_Property<ManagedType,FunT,PROP,false>>(at, value->simulate(context));
                    case FactoryNodeType::safeGetField:
                    case FactoryNodeType::safeGetFieldPtr:
                    case FactoryNodeType::getFieldR2V:
                        DAS_ASSERTF(0, "property requested property type, which is meaningless for the non-ref"
                                    "we should not be here, since property can't have ref type"
                                    "daScript compiler will later report missing node error");
                    default:
                        return nullptr;
                }
            };
        }
        virtual void aotVisitGetField ( TextWriter & ss, const string & fieldName ) override { 
            auto it = fields.find(fieldName);
            if (it != fields.end()) {
                ss << "." << it->second.cppName;
            } else {
                ss << "." << fieldName << " /*undefined */";
            }
        }
        virtual void aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName ) override {
            auto it = fields.find(fieldName);
            if (it != fields.end()) {
                ss << "->" << it->second.cppName;
            } else {
                ss << "->" << fieldName << " /*undefined */";
            }
        }
        void addFieldEx ( const string & na, const string & cppNa, off_t offset, TypeDeclPtr pT ) {
            auto & field = fields[na];
            if ( field.decl ) {
                DAS_FATAL_LOG("structure field %s already exist in structure %s\n", na.c_str(), name.c_str() );
                DAS_FATAL_ERROR;
            }
            field.cppName = cppNa;
            field.decl = pT;
            field.offset = offset;
            auto baseType = field.decl->baseType;
            field.factory = [offset,baseType](FactoryNodeType nt,Context & context,const LineInfo & at, const ExpressionPtr & value) -> SimNode * {
                if ( !value->type->isPointer() ) {
                    if ( nt==FactoryNodeType::getField || nt==FactoryNodeType::getFieldR2V ) {
                        auto r2vType = (nt==FactoryNodeType::getField) ? Type::none : baseType;
                        auto tnode = value->trySimulate(context, offset, r2vType);
                        if ( tnode ) {
                            return tnode;
                        }
                    }
                }
                auto simV = value->simulate(context);
                switch ( nt ) {
                    case FactoryNodeType::getField:
                        return context.code->makeNode<SimNode_FieldDeref>(at,simV,offset);
                    case FactoryNodeType::getFieldR2V:
                        return context.code->makeValueNode<SimNode_FieldDerefR2V>(baseType,at,simV,offset);
                    case FactoryNodeType::safeGetField:
                        return context.code->makeNode<SimNode_SafeFieldDeref>(at,simV,offset);
                    case FactoryNodeType::safeGetFieldPtr:
                        return context.code->makeNode<SimNode_SafeFieldDerefPtr>(at,simV,offset);
                    default:
                        return nullptr;
                }
            };
        }
        template <typename TT, off_t off>
        __forceinline void addField ( const string & na, const string & cppNa = "" ) {
            addFieldEx ( na, cppNa.empty() ? na : cppNa, off, makeType<TT>(*mlib) );
        }
        virtual void walk ( DataWalker & walker, void * data ) override {
            if ( !sti ) {
                auto debugInfo = helpA.debugInfo;
                sti = debugInfo->template makeNode<StructInfo>();
                sti->name = debugInfo->allocateName(name);
                // count fields
                sti->count = 0;
                for ( auto & fi : fields ) {
                    auto & var = fi.second;
                    if ( var.offset != -1U ) {
                        sti->count ++;
                    }
                }
                // and allocate
                sti->size = (uint32_t) getSizeOf();
                sti->fields = (VarInfo **) debugInfo->allocate( sizeof(VarInfo *) * sti->count );
                int i = 0;
                for ( auto & fi : fields ) {
                    auto & var = fi.second;
                    if ( var.offset != -1U ) {
                        VarInfo * vi = debugInfo->template makeNode<VarInfo>();
                        helpA.makeTypeInfo(vi, var.decl);
                        vi->name = debugInfo->allocateName(fi.first);
                        vi->offset = var.offset;
                        sti->fields[i++] = vi;
                    }
                }
            }
            walker.walk_struct((char *)data, sti);
        }
        map<string,StructureField> fields;
        DebugInfoHelper            helpA;
        StructInfo *               sti = nullptr;
        ModuleLibrary *            mlib = nullptr;
    };

    template <typename OT>
    struct ManagedStructureAnnotation<OT,true> : ManagedStructureAnnotation<OT,false> {
        typedef OT ManagedType;
        ManagedStructureAnnotation (const string & n, ModuleLibrary & ml, const string & cpn = "" )
            : ManagedStructureAnnotation<OT,false>(n,ml,cpn) { }
        virtual bool canNew() const override { return true; }
        virtual bool canDeletePtr() const override { return true; }
        virtual SimNode * simulateGetNew ( Context & context, const LineInfo & at ) const override {
            return context.code->makeNode<SimNode_NewHandle<ManagedType>>(at);
        }
        virtual SimNode * simulateDeletePtr ( Context & context, const LineInfo & at, SimNode * sube, uint32_t count ) const override {
            return context.code->makeNode<SimNode_DeleteHandlePtr<ManagedType>>(at,sube,count);
        }
    };

    template <typename OT, bool r2v=false>
    struct ManagedVectorAnnotation;

    template <typename OT>
    struct ManagedVectorAnnotation<OT,false> : TypeAnnotation {
        typedef vector<OT> VectorType;
        struct SimNode_VectorLength : SimNode {
            DAS_INT_NODE;
            SimNode_VectorLength ( const LineInfo & at, SimNode * rv )
                : SimNode(at), value(rv) {}
            __forceinline int32_t compute ( Context & context ) {
                auto pValue = (VectorType *) value->evalPtr(context);
                return int32_t(pValue->size());
            }
            SimNode * value;
        };
        struct SimNode_AtStdVector : SimNode_At {
            using TT = OT;
            DAS_PTR_NODE;
            SimNode_AtStdVector ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t ofs )
                : SimNode_At(at, rv, idx, 0, ofs, 0) {}
            virtual SimNode * visit ( SimVisitor & vis ) override {
                V_BEGIN();
                V_OP_TT(AtStdVector);
                V_SUB(value);
                V_SUB(index);
                V_ARG(stride);
                V_ARG(offset);
                V_ARG(range);
                V_END();
            }
            __forceinline char * compute ( Context & context ) {
                auto pValue = (VectorType *) value->evalPtr(context);
                uint32_t idx = cast<uint32_t>::to(index->eval(context));
                if ( idx >= pValue->size() ) {
                    context.throw_error_at(debugInfo,"std::vector index out of range");
                    return nullptr;
                } else {
                    return ((char *)(pValue->data() + idx)) + offset;
                }
            }
        };
        struct VectorIterator : Iterator {
            VectorIterator  ( VectorType * ar ) : array(ar) {}
            virtual bool first ( Context &, char * _value ) override {
                char ** value = (char **) _value;
                char * data     = (char *) array->data();
                uint32_t size   = (uint32_t) array->size();
                *value          = data;
                array_end       = data + size * sizeof(OT);
                return (bool) size;
            }
            virtual bool next  ( Context &, char * _value ) override {
                char ** value = (char **) _value;
                char * data = *value + sizeof(OT);
                *value = data;
                return data != array_end;
            }
            virtual void close ( Context &, char * _value ) override {
                char ** value = (char **) _value;
                *value = nullptr;
            }
            VectorType * array;
            char * array_end = nullptr;
        };
        ManagedVectorAnnotation(const string & n, ModuleLibrary & lib)
            : TypeAnnotation(n) {
                vecType = makeType<OT>(lib);
                vecType->ref = true;
        }
        virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
        virtual size_t getSizeOf() const override { return sizeof(VectorType); }
        virtual size_t getAlignOf() const override { return alignof(VectorType); }
        virtual bool isRefType() const override { return true; }
        virtual bool isIndexable ( const TypeDeclPtr & indexType ) const override { return indexType->isIndex(); }
        virtual bool isIterable ( ) const override { return true; }
        virtual bool canMove() const override { return false; }
        virtual bool canCopy() const override { return false; }
        virtual bool isLocal() const override { return false; }
        virtual TypeDeclPtr makeFieldType ( const string & na ) const override {
            if ( na=="length" ) return make_shared<TypeDecl>(Type::tInt);
            return nullptr;
        }
        virtual void aotVisitGetField ( TextWriter & ss, const string & fieldName ) override { 
            if ( fieldName=="length" ) {
                ss << ".size()";
            } else {
                ss << "." << fieldName << " /*undefined */";
            }
        }
        virtual void aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName ) override {
            if ( fieldName=="length" ) {
                ss << "->size()";
            } else {
                ss << "." << fieldName << " /*undefined */";
            }
        }
        virtual TypeDeclPtr makeIndexType ( const ExpressionPtr &, const ExpressionPtr & ) const override {
            return make_shared<TypeDecl>(*vecType);
        }
        virtual TypeDeclPtr makeIteratorType ( const ExpressionPtr & ) const override {
            return make_shared<TypeDecl>(*vecType);
        }
        virtual SimNode * simulateGetAt ( Context & context, const LineInfo & at, const TypeDeclPtr &,
                                         const ExpressionPtr & rv, const ExpressionPtr & idx, uint32_t ofs ) const override {
            return context.code->makeNode<SimNode_AtStdVector>(at,
                                                               rv->simulate(context),
                                                               idx->simulate(context),
                                                               ofs);
        }
        virtual SimNode * simulateGetIterator ( Context & context, const LineInfo & at, const ExpressionPtr & src ) const override {
            auto rv = src->simulate(context);
            return context.code->makeNode<SimNode_AnyIterator<VectorType,VectorIterator>>(at, rv);
        }
        virtual SimNode * simulateGetField ( const string & na, Context & context,
                                            const LineInfo & at, const ExpressionPtr & value ) const override {
            if ( na=="length" ) return context.code->makeNode<SimNode_VectorLength>(at,value->simulate(context));
            return nullptr;
        }
        TypeDeclPtr vecType;
    };

    template <typename OT>
    struct ManagedVectorAnnotation<OT,true> : ManagedVectorAnnotation<OT,false> {
        struct SimNode_AtStdVectorR2V : ManagedVectorAnnotation<OT,false>::SimNode_AtStdVector {
            SimNode_AtStdVectorR2V ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t ofs )
                : ManagedVectorAnnotation<OT,false>::SimNode_AtStdVector(at, rv, idx, ofs) {}
            virtual vec4f eval ( Context & context ) override {
                OT * pR = (OT *) ManagedVectorAnnotation<OT,false>::SimNode_AtStdVector::compute(context);
                return cast<OT>::from(*pR);
            }
#define EVAL_NODE(TYPE,CTYPE)                                           \
            virtual CTYPE eval##TYPE ( Context & context ) override {   \
                return *(CTYPE *)ManagedVectorAnnotation<OT,false>::SimNode_AtStdVector::compute(context);    \
            }
            DAS_EVAL_NODE
#undef EVAL_NODE
        };
        virtual SimNode * simulateGetAtR2V ( Context & context, const LineInfo & at, const TypeDeclPtr &,
                                            const ExpressionPtr & rv, const ExpressionPtr & idx, uint32_t ofs ) const override {
            return context.code->makeNode<SimNode_AtStdVectorR2V>(at,
                                                                  rv->simulate(context),
                                                                  idx->simulate(context),
                                                                  ofs);
        }
    };

    template <typename OT>
    struct ManagedValueAnnotation : TypeAnnotation {
        static_assert(sizeof(OT)<=sizeof(vec4f), "value types have to fit in ABI");
        ManagedValueAnnotation(const string & n, const string & cpn = string()) : TypeAnnotation(n,cpn) {}
        virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
        virtual bool canMove() const override { return true; }
        virtual bool canCopy() const override { return true; }
        virtual bool isLocal() const override { return true; }
        virtual bool isPod() const override { return true; }
        virtual bool isRawPod() const override { return true; }
        virtual size_t getSizeOf() const override { return sizeof(OT); }
        virtual size_t getAlignOf() const override { return alignof(OT); }
        virtual bool isRefType() const override { return false; }
        virtual SimNode * simulateCopy ( Context & context, const LineInfo & at, SimNode * l, SimNode * r ) const override {
            return context.code->makeNode<SimNode_CopyValue<OT>>(at, l, r);
        }
        virtual SimNode * simulateRef2Value ( Context & context, const LineInfo & at, SimNode * l ) const override {
            return context.code->makeNode<SimNode_Ref2Value<OT>>(at, l);
        }
    };

    template <typename TT>
    void addConstant ( Module & mod, const string & name, const TT & value ) {
        VariablePtr pVar = make_shared<Variable>();
        pVar->name = name;
        pVar->type = make_shared<TypeDecl>((Type)ToBasicType<TT>::type);
        pVar->type->constant = true;
        pVar->init = Program::makeConst(LineInfo(),pVar->type,cast<TT>::from(value));
        pVar->init->type = make_shared<TypeDecl>(*pVar->type);
        if ( !mod.addVariable(pVar) ) {
            DAS_FATAL_LOG("addVariable(%s) failed in module %s\n", name.c_str(), mod.name.c_str());
            DAS_FATAL_ERROR;
        }
    }

    template <typename TT>
    void addEquNeq(Module & mod, const ModuleLibrary & lib) {
        addExtern<decltype(&das_equ<TT>),  das_equ<TT>> (mod, lib, "==", SideEffects::none, "das_equ");
        addExtern<decltype(&das_nequ<TT>), das_nequ<TT>>(mod, lib, "!=", SideEffects::none, "das_nequ");
    }

    template <typename TT>
    void addEquNeqVal(Module & mod, const ModuleLibrary & lib) {
        addExtern<decltype(&das_equ_val<TT>),  das_equ<TT>> (mod, lib, "==", SideEffects::none, "das_equ_val");
        addExtern<decltype(&das_nequ_val<TT>), das_nequ<TT>>(mod, lib, "!=", SideEffects::none, "das_nequ_val");
    }
}

MAKE_TYPE_FACTORY(das_string, das::string);

#include "daScript/simulate/simulate_visit_op_undef.h"

