#include "precomp.h"

#include "ast.h"
#include "enums.h"

#include "runtime_array.h"
#include "runtime_table.h"
#include "runtime_range.h"
#include "hash.h"

void yybegin(const char * str);
int yyparse();
int yylex_destroy();

namespace yzg
{
    // TypeDecl
    
    ostream& operator<< (ostream& stream, const TypeDecl & decl) {
        if ( decl.baseType==Type::tHandle ) {
            stream << decl.annotation->name;
        } else if ( decl.baseType==Type::tArray ) {
            if ( decl.firstType ) {
                stream << "array<" << *decl.firstType << ">";
            } else {
                stream << "array";
            }
        } else if ( decl.baseType==Type::tTable ) {
            if ( decl.firstType && decl.secondType ) {
                stream << "table<" << *decl.firstType << "," << *decl.secondType << ">";
            } else {
                stream << "table";
            }
        } else if ( decl.baseType==Type::tStructure ) {
            if ( decl.structType ) {
                stream << decl.structType->name;
            } else {
                stream << "unspecified";
            }
        } else if ( decl.baseType==Type::tPointer ) {
            if ( decl.firstType ) {
                stream << *decl.firstType << "?";
            } else {
                stream << "void ?";
            }
        } else if ( decl.baseType==Type::tIterator ) {
            if ( decl.firstType ) {
                stream << "iterator<" << *decl.firstType << ">";
            } else {
                stream << "iterator";
            }
        } else if ( decl.baseType==Type::tBlock ) {
            if ( decl.firstType ) {
                stream << "block<" << *decl.firstType << ">";
            } else {
                stream << "block";
            }
        } else {
            stream << to_string(decl.baseType);
        }
        if ( decl.constant ) {
            stream << " const";
        }
        for ( auto d : decl.dim ) {
            stream << "[" << d << "]";
        }
        if ( decl.ref )
            stream << "&";
        return stream;
    }
    
    TypeDecl::TypeDecl(const TypeDecl & decl) {
        baseType = decl.baseType;
        structType = decl.structType;
        annotation = decl.annotation;
        dim = decl.dim;
        ref = decl.ref;
        constant = decl.constant;
        at = decl.at;
        if ( decl.firstType )
            firstType = make_shared<TypeDecl>(*decl.firstType);
        if ( decl.secondType )
            secondType = make_shared<TypeDecl>(*decl.secondType);
    }
    
    bool TypeDecl::canMove() const {
        if ( baseType==Type::tHandle )
            return annotation->canMove();
        if ( baseType==Type::tBlock )
            return false;
        return true;
    }
    
    bool TypeDecl::canCopy() const {
        if ( baseType==Type::tHandle )
            return annotation->canCopy();
        if ( baseType==Type::tArray || baseType==Type::tTable || baseType==Type::tBlock )
            return false;
        if ( baseType==Type::tStructure && structType )
            return structType->canCopy();
        return true;
    }
    
    bool TypeDecl::isPod() const {
        if ( baseType==Type::tArray || baseType==Type::tTable || baseType==Type::tString || baseType==Type::tBlock )
            return false;
        if ( baseType==Type::tStructure && structType )
            return structType->isPod();
        if ( baseType==Type::tHandle )
            return annotation->isPod();
        return true;
    }
    
    string TypeDecl::getMangledName() const {
        stringstream ss;
        if ( constant ) {
            ss << "#const";
        }
        if ( baseType==Type::tHandle ) {
            ss << "#handle#" << annotation->name;
        } else if ( baseType==Type::tArray ) {
            ss << "#array";
            if ( firstType ) {
                ss << "#" << firstType->getMangledName();
            }
        } else if ( baseType==Type::tTable ) {
            ss << "#table";
            if ( firstType ) {
                ss << "#" << firstType->getMangledName();
            }
            if ( secondType ) {
                ss << "#" << secondType->getMangledName();
            }
        } else if ( baseType==Type::tPointer ) {
            ss << "#ptr";
            if ( firstType ) {
                ss << "#" << firstType->getMangledName();
            }
        } else if ( baseType==Type::tIterator ) {
            ss << "#iterator";
            if ( firstType ) {
                ss << "#" << firstType->getMangledName();
            }
        } else if ( baseType==Type::tBlock ) {
            ss << "#block";
            if ( firstType ) {
                ss << "#" << firstType->getMangledName();
            }
        } else if ( baseType==Type::tStructure ) {
            if ( structType ) {
                ss << structType->name;
            } else {
                ss << "structue?";
            }
        } else {
            ss << to_string(baseType);
        }
        if ( ref )
            ss << "#ref";
        if ( dim.size() ) {
            for ( auto d : dim ) {
                ss << "#" << d;
            }
        }
        return ss.str();
    }
    
    bool TypeDecl::isConst() const
    {
        if ( constant )
            return true;
        if ( baseType==Type::tPointer )
            if ( firstType && firstType->constant )
                return true;
        return false;
    }
    
    bool TypeDecl::isSameType ( const TypeDecl & decl, bool refMatters, bool constMatters ) const {
        if ( baseType!=decl.baseType )
            return false;
        if ( baseType==Type::tHandle && annotation!=decl.annotation )
            return false;
        if ( baseType==Type::tStructure && structType!=decl.structType )
            return false;
        if ( baseType==Type::tPointer || baseType==Type::tIterator ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType) ) {
                return false;
            }
        }
        if ( baseType==Type::tArray ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType) ) {
                return false;
            }
        }
        if ( baseType==Type::tTable ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType) ) {
                return false;
            }
            if ( secondType && decl.secondType && !secondType->isSameType(*decl.secondType) ) {
                return false;
            }
        }
        if ( baseType==Type::tBlock ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType) ) {
                return false;
            }
        }
        if ( dim!=decl.dim )
            return false;
        if ( refMatters )
            if ( ref!=decl.ref )
                return false;
        if ( constMatters )
            if ( constant!=decl.constant )
                return false;
        return true;
    }
    
    bool TypeDecl::isGoodIteratorType() const {
        return baseType==Type::tIterator && dim.size()==0 && firstType;
    }
    
    bool TypeDecl::isGoodBlockType() const {
        return baseType==Type::tBlock && dim.size()==0;
    }
    
    bool TypeDecl::isGoodArrayType() const {
        return baseType==Type::tArray && dim.size()==0 && firstType;
    }
    
    bool TypeDecl::isGoodTableType() const {
        return baseType==Type::tTable && dim.size()==0 && firstType && secondType;
    }
    
    bool TypeDecl::isIteratorType ( const TypeDecl & decl ) const {
        if ( baseType!=decl.baseType )
            return false;
        if ( baseType==Type::tStructure && structType!=decl.structType )
            return false;
        if ( decl.dim.size() )
            return false;
        if ( !decl.isRef() )
            return false;
        return true;
    }
    
    bool TypeDecl::isVoid() const {
        return (baseType==Type::tVoid) && (dim.size()==0);
    }
    
    bool TypeDecl::isPointer() const {
        return (baseType==Type::tPointer) && (dim.size()==0);
    }
    
    bool TypeDecl::isHandle() const {
        return (baseType==Type::tHandle) && (dim.size()==0);
    }
    
    bool TypeDecl::isSimpleType() const {
        if (    baseType==Type::none
            ||  baseType==Type::tVoid
            ||  baseType==Type::tStructure
            ||  baseType==Type::tPointer )
            return false;
        if ( dim.size() )
            return false;
        return true;
    }
    
    bool TypeDecl::isWorkhorseType() const {
        if ( dim.size() )
            return false;
        switch ( baseType ) {
            case Type::tBool:
            case Type::tInt64:
            case Type::tUInt64:
            case Type::tInt:
            case Type::tInt2:
            case Type::tInt3:
            case Type::tInt4:
            case Type::tUInt:
            case Type::tUInt2:
            case Type::tUInt3:
            case Type::tUInt4:
            case Type::tFloat:
            case Type::tFloat2:
            case Type::tFloat3:
            case Type::tFloat4:
            case Type::tRange:
            case Type::tURange:
            case Type::tString:
            case Type::tPointer:
                return true;
            default:
                return false;
        }
    }
    
    Type TypeDecl::getRangeBaseType() const
    {
        switch ( baseType ) {
            case Type::tRange:  return Type::tInt;
            case Type::tURange: return Type::tUInt;
            default:
                assert(0 && "we should not even be here");
                return Type::none;
        }
    }
    
    bool TypeDecl::isRange() const
    {
        return (baseType==Type::tRange || baseType==Type::tURange) && dim.size()==0;
    }
    
    bool TypeDecl::isSimpleType(Type typ) const {
        return baseType==typ && isSimpleType();
    }
    
    bool TypeDecl::isArray() const {
        return dim.size() != 0;
    }
    
    bool TypeDecl::isRef() const {
        return ref || isRefType();
    }
    
    bool TypeDecl::isRefType() const {
        if ( baseType==Type::tHandle ) {
            return annotation->isRefType();
        }
        return baseType==Type::tStructure || baseType==Type::tArray || baseType==Type::tTable || dim.size();
    }
    
    bool TypeDecl::isIndex() const {
        return (baseType==Type::tInt || baseType==Type::tUInt) && dim.size()==0;
    }
    
    int TypeDecl::getBaseSizeOf() const {
        if ( baseType==Type::tHandle ) {
            return annotation->getSizeOf();
        }
        return baseType==Type::tStructure ? structType->getSizeOf() : getTypeBaseSize(baseType);
    }
    
    int TypeDecl::getSizeOf() const {
        int size = 1;
        for ( auto i : dim )
            size *= i;
        return getBaseSizeOf() * size;
    }
    
    int TypeDecl::getStride() const {
        int size = 1;
        if ( dim.size() > 1 ) {
            for ( size_t i=0; i!=dim.size()-1; ++i )
                size *= dim[i];
        }
        return getBaseSizeOf() * size;
    }
    
    // structure
    
    bool Structure::canCopy() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canCopy() )
                return false;
        }
        return true;
    }
    
    bool Structure::isPod() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isPod() )
                return false;
        }
        return true;
    }
    
    int Structure::getSizeOf() const {
        int size = 0;
        for ( const auto & fd : fields ) {
            size += fd.type->getSizeOf();
        }
        return size;
    }
    
    const Structure::FieldDeclaration * Structure::findField ( const string & name ) const {
        for ( const auto & fd : fields ) {
            if ( fd.name==name ) {
                return &fd;
            }
        }
        return nullptr;
    }
    
    ostream& operator<< (ostream& stream, const Structure & structure) {
        stream << "(struct " << structure.name << "\n";
        for ( auto & decl : structure.fields ) {
            stream << "\t(" << *decl.type << " " << decl.name << ")\n";
        }
        stream << ")";
        return stream;
    }

    // variable
    
    ostream& operator<< (ostream& stream, const Variable & var) {
        stream << *var.type << " " << var.name;
        return stream;
    }
    
    VariablePtr Variable::clone() const {
        auto pVar = make_shared<Variable>();
        pVar->name = name;
        pVar->type = make_shared<TypeDecl>(*type);
        if ( init )
            pVar->init = init->clone();
        pVar->at = at;
        return pVar;
    }
    
    // function
    
    string Function::getMangledName() const {
        stringstream ss;
        ss << name;
        for ( auto & arg : arguments ) {
            ss << " " << arg->type->getMangledName();
        }
        // ss << "->" << result->getMangledName();
        return ss.str();
    }
    
    VariablePtr Function::findArgument(const string & name) {
        for ( auto & arg : arguments ) {
            if ( arg->name==name ) {
                return arg;
            }
        }
        return nullptr;
    }
    
    SimNode * Function::simulate (Context & context) const {
        if ( builtIn ) {
            assert(0 && "can only simulate non built-in function");
            return nullptr;
        }
        return body->simulate(context);
    }
    
    FunctionPtr Function::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitArgument(this, arg, arg==arguments.back() );
            if ( arg->init ) {
                vis.preVisitArgumentInit(this, arg->init.get());
                arg->init = arg->init->visit(vis);
                arg->init = vis.visitArgumentInit(this, arg->init.get());
            }
            arg = vis.visitArgument(this, arg, arg==arguments.back() );
        }
        vis.preVisitFunctionBody(this, body.get());
        body = body->visit(vis);
        body = vis.visitFunctionBody(this, body.get());
        return vis.visit(this);
    }
    
    // built-in function
    
    BuiltInFunction::BuiltInFunction ( const string & fn ) {
        builtIn = true;
        name = fn;
    }
    
    // type annotation
    
    void debugType ( TypeAnnotation * ta, stringstream & ss , void * data ) {
        ta->debug(ss, data);
    }
    
    // expression
    
    void Expression::InferTypeContext::error ( const string & err, const LineInfo & at, CompilationError cerr ) {
        program->error(err,at,cerr);
    }
    
    ExpressionPtr Expression::clone( const ExpressionPtr & expr ) const {
        if ( !expr ) {
            assert(0 && "unsupported expression");
            return nullptr;
        }
        expr->at = at;
        expr->type = type ? make_shared<TypeDecl>(*type) : nullptr;
        return expr;
    }
    
    ExpressionPtr Expression::autoDereference ( const ExpressionPtr & expr ) {
        if ( expr->type && expr->type->isRef() ) {
            auto ar2l = make_shared<ExprRef2Value>();
            ar2l->subexpr = expr;
            ar2l->at = expr->at;
            ar2l->type = make_shared<TypeDecl>(*expr->type);
            ar2l->type->ref = false;
            ar2l->topLevel = expr->topLevel;
            ar2l->argLevel = expr->argLevel;
            expr->topLevel = expr->argLevel = false;
            return ar2l;
        } else {
            return expr;
        }
    }
    
    // ExprRef2Value
    
    ExpressionPtr ExprRef2Value::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprRef2Value::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprRef2Value>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }
    
    void ExprRef2Value::inferType(InferTypeContext & context) {
        type.reset();
        subexpr->inferType(context);
        if ( !subexpr->type ) return;
        // infer
        if ( !subexpr->type->isRef() ) {
            context.error("can only dereference ref", at);
        } else if ( !subexpr->type->isSimpleType() ) {
            context.error("can only dereference a simple type", at);
        } if ( !subexpr->type->canCopy() ) {
            context.error("can't dereference non-copyable type", at);
        } else {
            type = make_shared<TypeDecl>(*subexpr->type);
            type->ref = false;
        }
    }
    
    SimNode * ExprRef2Value::simulate (Context & context) const {
        if ( type->isHandle() ) {
            return type->annotation->simulateRef2Value(context, at, subexpr->simulate(context));
        } else {
            return context.makeValueNode<SimNode_Ref2Value>(type->baseType, at, subexpr->simulate(context));
        }
    }
    
    // ExprPtr2Ref
    
    ExpressionPtr ExprPtr2Ref::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprPtr2Ref::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprPtr2Ref>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }
    
    void ExprPtr2Ref::inferType(InferTypeContext & context) {
        type.reset();
        subexpr->inferType(context);
        if ( !subexpr->type ) return;
        // infer
        subexpr = autoDereference(subexpr);
        if ( !subexpr->type->isPointer() ) {
            context.error("can only dereference pointer", at, CompilationError::cant_dereference);
        } else if ( !subexpr->type->firstType || subexpr->type->firstType->isVoid() ) {
            context.error("can only dereference pointer to something", at, CompilationError::cant_dereference);
        } else {
            type = make_shared<TypeDecl>(*subexpr->type->firstType);
            type->ref = true;
            type->constant |= subexpr->type->constant;
        }
    }
    
    SimNode * ExprPtr2Ref::simulate (Context & context) const {
        return context.makeNode<SimNode_Ptr2Ref>(at,subexpr->simulate(context));
    }

    // ExprNullCoalescing
    
    ExpressionPtr ExprNullCoalescing::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitNullCoaelescingDefault(this, defaultValue.get());
        defaultValue = defaultValue->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprNullCoalescing::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNullCoalescing>(expr);
        ExprPtr2Ref::clone(cexpr);
        cexpr->defaultValue = defaultValue->clone();
        return cexpr;
    }
    
    void ExprNullCoalescing::inferType(InferTypeContext & context) {
        subexpr->inferType(context);
        defaultValue->inferType(context);
        if ( !subexpr->type || !defaultValue->type ) return;
        // infer
        subexpr = autoDereference(subexpr);
        auto seT = subexpr->type;
        auto dvT = defaultValue->type;
        if ( !seT->isPointer() ) {
            context.error("can only dereference pointer", at, CompilationError::cant_dereference);
        } else if ( !seT->firstType || seT->firstType->isVoid() ) {
            context.error("can only dereference pointer to something", at, CompilationError::cant_dereference);
        } else if ( !seT->firstType->isSameType(*dvT,false,false) ) {
            context.error("default value type mismatch in " + seT->firstType->describe() + " vs " + dvT->describe(),
                          at, CompilationError::cant_dereference);
        } else if ( !seT->isConst() && dvT->isConst() ) {
            context.error("default value type mismatch, constant matters in " + seT->describe() + " vs " + dvT->describe(),
                          at, CompilationError::cant_dereference);
        } else {
            type = make_shared<TypeDecl>(*dvT);
            type->constant |= subexpr->type->constant;
        }
    }
    
    SimNode * ExprNullCoalescing::simulate (Context & context) const {
        if ( type->ref ) {
            return context.makeNode<SimNode_NullCoalescingRef>(at,subexpr->simulate(context),defaultValue->simulate(context));
        } else {
            return context.makeValueNode<SimNode_NullCoalescing>(type->baseType,at,subexpr->simulate(context),defaultValue->simulate(context));
        }
    }

    
    // ExprAssert
    
    ExpressionPtr ExprAssert::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAssert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprAssert::inferType(InferTypeContext & context) {
        type.reset();
        ExprLooksLikeCall::inferType(context);
        if ( arguments.size()<1 || arguments.size()>2 ) {
            context.error("assert(expr) or assert(expr,string)", at, CompilationError::invalid_argument_count);
            return;
        }
        if ( argumentsFailedToInfer ) return;
        // infer
        autoDereference();
        if ( !arguments[0]->type->isSimpleType(Type::tBool) )
            context.error("assert condition must be boolean", at, CompilationError::invalid_argument_type);
        if ( arguments.size()==2 && !arguments[1]->isStringConstant() )
            context.error("assert comment must be string constant", at, CompilationError::invalid_argument_type);
        type = make_shared<TypeDecl>(Type::tVoid);
    }
    
    SimNode * ExprAssert::simulate (Context & context) const {
        string message;
        if ( arguments.size()==2 && arguments[1]->isStringConstant() )
            message = static_pointer_cast<ExprConstString>(arguments[1])->getValue();
        return context.makeNode<SimNode_Assert>(at,arguments[0]->simulate(context),context.allocateName(message));
    }
    
    // ExprDebug
    
    ExpressionPtr ExprDebug::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprDebug>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprDebug::inferType(InferTypeContext & context) {
        type.reset();
        if ( arguments.size()<1 || arguments.size()>2 ) {
            context.error("debug(expr) or debug(expr,string)", at, CompilationError::invalid_argument_count);
            return;
        }
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        if ( arguments.size()==2 && !arguments[1]->isStringConstant() )
            context.error("debug comment must be string constant", at, CompilationError::invalid_argument_type);
        type = make_shared<TypeDecl>(*arguments[0]->type);
    }
    
    SimNode * ExprDebug::simulate (Context & context) const {
        TypeInfo * pTypeInfo = context.makeNode<TypeInfo>();
        context.thisProgram->makeTypeInfo(pTypeInfo, context, arguments[0]->type);
        string message;
        if ( arguments.size()==2 && arguments[1]->isStringConstant() )
            message = static_pointer_cast<ExprConstString>(arguments[1])->getValue();
        return context.makeNode<SimNode_Debug>(at,
                                               arguments[0]->simulate(context),
                                               pTypeInfo,
                                               context.allocateName(message));
    }

    // ExprMakeBlock
    
    ExpressionPtr ExprMakeBlock::visit(Visitor & vis) {
        vis.preVisit(this);
        block = block->visit(vis);
        return vis.visit(this);
    }
    
    void ExprMakeBlock::inferType(InferTypeContext & context) {
        type.reset();
        static_pointer_cast<ExprBlock>(block)->closure = true;
        block->inferType(context);
        // infer
        type = make_shared<TypeDecl>(Type::tBlock);
        if ( block->type ) {
            type->firstType = make_shared<TypeDecl>(*block->type);
        }
    }
    
    SimNode * ExprMakeBlock::simulate (Context & context) const {
        return context.makeNode<SimNode_MakeBlock>(at,block->simulate(context));
    }
    
    ExpressionPtr ExprMakeBlock::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeBlock>(expr);
        cexpr->block = block->clone();
        return cexpr;
    }
    
    // ExprInvoke
    
    ExpressionPtr ExprInvoke::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprInvoke>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprInvoke::inferType(InferTypeContext & context) {
        type.reset();
        if ( arguments.size()!=1 ) {
            context.error("invoke(block)", at, CompilationError::invalid_argument_count);
            return;
        }
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        arguments[0] = Expression::autoDereference(arguments[0]);
        auto blockT = arguments[0]->type;
        if ( !blockT->isGoodBlockType() ) {
            context.error("expecting block", at, CompilationError::invalid_argument_type);
        }
        if ( blockT->firstType ) {
            type = make_shared<TypeDecl>(*blockT->firstType);
        } else {
            type = make_shared<TypeDecl>();
        }
    }
    
    SimNode * ExprInvoke::simulate (Context & context) const {
        return context.makeNode<SimNode_Invoke>(at,arguments[0]->simulate(context));
    }
    
    // ExprHash
    
    ExpressionPtr ExprHash::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprHash>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprHash::inferType(InferTypeContext & context) {
        type.reset();
        if ( arguments.size()!=1 ) {
            context.error("hash(expr)", at, CompilationError::invalid_argument_count);
            return;
        }
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        type = make_shared<TypeDecl>(Type::tUInt64);
    }
    
    SimNode * ExprHash::simulate (Context & context) const {
        auto val = arguments[0]->simulate(context);
        if ( !arguments[0]->type->isRef() ) {
            return context.makeValueNode<SimNode_HashOfValue>(arguments[0]->type->baseType, at, val);
        } else if ( arguments[0]->type->isPod() ) {
            return context.makeNode<SimNode_HashOfRef>(at, val, arguments[0]->type->getSizeOf());
        } else {
            auto typeInfo = context.makeNode<TypeInfo>();
            context.thisProgram->makeTypeInfo(typeInfo, context, arguments[0]->type);
            return context.makeNode<SimNode_HashOfMixedType>(at, val, typeInfo);
        }
    }

    // ExprArrayPush
    
    ExpressionPtr ExprArrayPush::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprArrayPush>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprArrayPush::inferType(InferTypeContext & context) {
        type.reset();
        if ( arguments.size()!=2 && arguments.size()!=3 ) {
            context.error("push(array,value) or push(array,value,at)", at, CompilationError::invalid_argument_count);
            return;
        }
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        auto arrayType = arguments[0]->type;
        auto valueType = arguments[1]->type;
        if ( !arrayType->isGoodArrayType() ) {
            context.error("push first argument must be fully qualified array", at, CompilationError::invalid_argument_type);
            return;
        }
        if ( !arrayType->firstType->isSameType(*valueType,false) )
            context.error("can't push value of different type", at, CompilationError::invalid_argument_type);
        if ( arguments.size()==3 && !arguments[2]->type->isIndex() )
            context.error("push at must be an index", at, CompilationError::invalid_argument_type);
        type = make_shared<TypeDecl>(Type::tVoid);
    }
    
    SimNode * ExprArrayPush::simulate (Context & context) const {
        auto arr = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        auto idx = arguments.size()==3 ? arguments[2]->simulate(context) : nullptr;
        if ( arguments[1]->type->isRef() ) {
            return context.makeNode<SimNode_ArrayPushRefValue>(at, arr, val, idx, arguments[0]->type->firstType->getSizeOf());
        } else {
            return context.makeValueNode<SimNode_ArrayPushValue>(arguments[1]->type->baseType, at, arr, val, idx);
        }
    }
    
    // ExprErase
    
    ExpressionPtr ExprErase::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprErase>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprErase::inferType(InferTypeContext & context) {
        type.reset();
        if ( arguments.size()!=2 ) {
            context.error("erase(table,key) or erase(array,index)", at, CompilationError::invalid_argument_count);
            return;
        }
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        arguments[1] = Expression::autoDereference(arguments[1]);
        auto containerType = arguments[0]->type;
        auto valueType = arguments[1]->type;
        if ( containerType->isGoodArrayType() ) {
            if ( !valueType->isIndex() )
                context.error("size must be int or uint", at, CompilationError::invalid_argument_type);
            type = make_shared<TypeDecl>(Type::tVoid);
        } else if ( containerType->isGoodTableType() ) {
            if ( !containerType->firstType->isSameType(*valueType,false) )
                context.error("key must be of the same type as table<key,...>", at, CompilationError::invalid_argument_type);
            type = make_shared<TypeDecl>(Type::tBool);
        } else {
            context.error("first argument must be fully qualified array or table", at, CompilationError::invalid_argument_type);
        }
    }
    
    SimNode * ExprErase::simulate (Context & context) const {
        auto cont = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        if ( arguments[0]->type->isGoodArrayType() ) {
            auto size = arguments[0]->type->firstType->getSizeOf();
            return context.makeNode<SimNode_ArrayErase>(at,cont,val,size);
        } else if ( arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = arguments[0]->type->secondType->getSizeOf();
            return context.makeValueNode<SimNode_TableErase>(arguments[0]->type->firstType->baseType, at, cont, val, valueTypeSize);
        } else {
            assert(0 && "we should not even be here");
            return nullptr;
        }
    }
    
    // ExprFind
    
    ExpressionPtr ExprFind::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprFind>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }
    
    void ExprFind::inferType(InferTypeContext & context) {
        type.reset();
        if ( arguments.size()!=2 ) {
            context.error("find(table,key) or find(array,value)", at, CompilationError::invalid_argument_count);
            return;
        }
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        arguments[1] = Expression::autoDereference(arguments[1]);
        auto containerType = arguments[0]->type;
        auto valueType = arguments[1]->type;
        if ( containerType->isGoodArrayType() ) {
            if ( !valueType->isSameType(*containerType->firstType) )
                context.error("value must be of the same type as array<value>", at, CompilationError::invalid_argument_type);
        } else if ( containerType->isGoodTableType() ) {
            if ( !containerType->firstType->isSameType(*valueType,false) )
                context.error("key must be of the same type as table<key,...>", at, CompilationError::invalid_argument_type);
            type = make_shared<TypeDecl>(Type::tPointer);
            type->firstType = make_shared<TypeDecl>(*containerType->secondType);
        } else {
            context.error("first argument must be fully qualified array or table", at, CompilationError::invalid_argument_type);
        }
    }
    
    SimNode * ExprFind::simulate (Context & context) const {
        auto cont = arguments[0]->simulate(context);
        auto val = arguments[1]->simulate(context);
        if ( arguments[0]->type->isGoodArrayType() ) {
            assert(0);
            return nullptr;
            // auto size = arguments[0]->type->firstType->getSizeOf();
            // return context.makeNode<SimNode_ArrayErase>(at,cont,val,size);
        } else if ( arguments[0]->type->isGoodTableType() ) {
            uint32_t valueTypeSize = arguments[0]->type->secondType->getSizeOf();
            return context.makeValueNode<SimNode_TableFind>(arguments[0]->type->firstType->baseType, at, cont, val, valueTypeSize);
        } else {
            assert(0 && "we should not even be here");
            return nullptr;
        }
    }

    // ExprSizeOf
    
    ExpressionPtr ExprSizeOf::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprSizeOf::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSizeOf>(expr);
        Expression::clone(cexpr);
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        if ( typeexpr )
            cexpr->typeexpr = typeexpr;
        return cexpr;
    }
    
    void ExprSizeOf::inferType(InferTypeContext & context) {
        type.reset();
        subexpr->inferType(context);
        if ( !subexpr->type ) return;
        // infer
        typeexpr = make_shared<TypeDecl>(*subexpr->type);
        type = make_shared<TypeDecl>(Type::tInt);
    }
    
    SimNode * ExprSizeOf::simulate (Context & context) const {
        int32_t size = typeexpr->getSizeOf();
        return context.makeNode<SimNode_ConstValue<int32_t>>(at,size);
    }
    
    // ExprNew
    
    ExpressionPtr ExprNew::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprNew::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNew>(expr);
        Expression::clone(cexpr);
        cexpr->typeexpr = typeexpr;
        return cexpr;
    }

    // TODO:
    //  this would need proper testing, but only afrer parser is modified
    //  curently none of the errors bellow can even be parsed
    void ExprNew::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        if ( typeexpr->ref ) {
            context.error("can't new a ref", typeexpr->at, CompilationError::invalid_new_type);
        } else if ( typeexpr->dim.size() ) {
            context.error("can only new single object", typeexpr->at, CompilationError::invalid_new_type);
        } else if ( typeexpr->baseType==Type::tStructure || typeexpr->isHandle() ) {
            type = make_shared<TypeDecl>(Type::tPointer);
            type->firstType = make_shared<TypeDecl>(*typeexpr);
        } else {
            context.error("can only new structures or handles", typeexpr->at, CompilationError::invalid_new_type);
        }
    }
    
    SimNode * ExprNew::simulate (Context & context) const {
        if ( typeexpr->isHandle() ) {
            return typeexpr->annotation->simulateGetNew(context, at);
        } else {
            int32_t bytes = typeexpr->getSizeOf();
            return context.makeNode<SimNode_New>(at,bytes);
        }
    }

    // ExprAt
    
    ExpressionPtr ExprAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitAtIndex(this, index.get());
        index = index->visit(vis);
        return vis.visit(this);
    }
    
    void ExprAt::inferType(InferTypeContext & context) {
        type.reset();
        subexpr->inferType(context);
        index->inferType(context);
        if ( !subexpr->type || !index->type ) return;
        // infer
        index = autoDereference(index);
        if ( subexpr->type->isGoodTableType() ) {
            if ( !subexpr->type->firstType->isSameType(*index->type) ) {
                context.error("table index type mismatch", index->at, CompilationError::invalid_index_type);
                return;
            }
            type = make_shared<TypeDecl>(*subexpr->type->secondType);
            type->ref = true;
            type->constant |= subexpr->type->constant;
        } else if ( subexpr->type->isHandle() ) {
            if ( !subexpr->type->annotation->isIndexable(index->type) ) {
                context.error("handle does not support this index type", index->at, CompilationError::invalid_index_type);
                return;
            }
            type = subexpr->type->annotation->makeIndexType(index->type);
            type->constant |= subexpr->type->constant;
        } else {
            if ( !index->type->isIndex() ) {
                context.error("index is int or uint", index->at, CompilationError::invalid_index_type);
                return;
            }
            if ( subexpr->type->isGoodArrayType() ) {
                type = make_shared<TypeDecl>(*subexpr->type->firstType);
                type->ref = true;
                type->constant |= subexpr->type->constant;
            } else if ( !subexpr->type->isRef() ) {
                context.error("can only index ref", subexpr->at, CompilationError::cant_index);
            } else if ( !subexpr->type->dim.size() ) {
                context.error("can only index arrays", subexpr->at, CompilationError::cant_index);
            } else {
                type = make_shared<TypeDecl>(*subexpr->type);
                type->ref = true;
                type->dim.pop_back();
            }
        }
    }
    
    ExpressionPtr ExprAt::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAt>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->index = index->clone();
        return cexpr;
    }

    SimNode * ExprAt::simulate (Context & context) const {
        auto prv = subexpr->simulate(context);
        auto pidx = index->simulate(context);
        if ( subexpr->type->isGoodTableType() ) {
            uint32_t valueTypeSize = subexpr->type->secondType->getSizeOf();
            return context.makeValueNode<SimNode_TableIndex>(subexpr->type->firstType->baseType, at, prv, pidx, valueTypeSize);
        } else if ( subexpr->type->isGoodArrayType() ) {
            uint32_t stride = subexpr->type->firstType->getSizeOf();
            return context.makeNode<SimNode_ArrayAt>(at, prv, pidx, stride);
        } else if ( subexpr->type->isHandle() ) {
            return subexpr->type->annotation->simulateGetAt(context, at, prv, pidx);
        } else {
            uint32_t stride = subexpr->type->getStride();
            uint32_t range = subexpr->type->dim.back();
            return context.makeNode<SimNode_At>(at, prv, pidx, stride, range);
        }
    }

    // ExprBlock
    
    ExpressionPtr ExprBlock::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & subexpr : list ) {
            vis.preVisitBlockExpression(this, subexpr.get());
            subexpr = subexpr->visit(vis);
            subexpr = vis.visitBlockExpression(this, subexpr.get());
        }
        return vis.visit(this);
    }
    
    ExpressionPtr ExprBlock::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprBlock>(expr);
        Expression::clone(cexpr);
        for ( auto & subexpr : list ) {
            cexpr->list.push_back(subexpr->clone());
        }
        return cexpr;
    }
    
    void ExprBlock::setBlockReturnsValue() {
        returnsValue = true;
        if ( list.size() ) {
            list.back()->setBlockReturnsValue();
        }
    }
    
    uint32_t ExprBlock::getEvalFlags() const {
        uint32_t flags = 0;
        for ( const auto & ex : list ) {
            flags |= ex->getEvalFlags();
        }
        return flags;
    }
    
    void ExprBlock::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        auto sz = context.local.size();
        for ( auto & ex : list ) {
            ex->topLevel = true;
            ex->inferType(context);
        }
        // block type
        if ( returnsValue && list.size() ) {
            uint32_t flags = getEvalFlags();
            if ( flags & EvalFlags::stopForReturn ) {
                context.error("captured block can't return outside of the block", at, CompilationError::invalid_block);
            } else if ( flags & EvalFlags::stopForBreak ) {
                context.error("captured block can't break outside of the block", at, CompilationError::invalid_block);
            } else {
                auto & tail = list.back();
                if ( tail->type ) {
                    tail = autoDereference(tail);
                    type = make_shared<TypeDecl>(*tail->type);
                }
            }
        }
        context.local.resize(sz);
    }
    
    SimNode * ExprBlock::simulate (Context & context) const {
        // TODO: what if list size is 0?
        if ( list.size()!=1 ) {
            auto block = context.makeNode<SimNode_Block>(at);
            block->total = int(list.size());
            block->list = (SimNode **) context.allocate(sizeof(SimNode *)*block->total);
            for ( int i = 0; i != block->total; ++i )
                block->list[i] = list[i]->simulate(context);
            return block;
        } else {
            return list[0]->simulate(context);
        }
    }
    
    // ExprField
    
    ExpressionPtr ExprField::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprField::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprField>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->value = value->clone();
        cexpr->field = field;
        return cexpr;
    }
    
    void ExprField::inferType(InferTypeContext & context) {
        type.reset();
        value->inferType(context);
        if ( !value->type ) return;
        // infer
        auto valT = value->type;
        if ( valT->isArray() ) {
            context.error("can't get field of array", at, CompilationError::cant_get_field);
            return;
        }
        if ( valT->isHandle() ) {
            annotation = valT->annotation;
            type = annotation->makeFieldType(name);
        } else if ( valT->baseType==Type::tStructure ) {
            field = valT->structType->findField(name);
        } else if ( valT->isPointer() ) {
            value = autoDereference(value);
            if ( valT->firstType->baseType==Type::tStructure ) {
                field = valT->firstType->structType->findField(name);
            } else if ( valT->firstType->isHandle() ) {
                annotation = valT->firstType->annotation;
                type = annotation->makeFieldType(name);
            }
        }
        // handle
        if ( field ) {
            type = make_shared<TypeDecl>(*field->type);
            type->ref = true;
            type->constant |= valT->constant;
        } else if ( !type ) {
            context.error("field " + name + " not found", at, CompilationError::cant_get_field);
        } else {
            type->constant |= valT->constant;
        }
    }
    
    SimNode * ExprField::simulate (Context & context) const {
        if ( !field ) {
            return annotation->simulateGetField(name, context, at, value->simulate(context));
        } else {
            return context.makeNode<SimNode_FieldDeref>(at,value->simulate(context),field->offset);
        }
    }
    
    // ExprSafeField
    
    ExpressionPtr ExprSafeField::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprSafeField::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSafeField>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }
    
    void ExprSafeField::inferType(InferTypeContext & context) {
        type.reset();
        value->inferType(context);
        if ( !value->type ) return;
        // infer
        auto valT = value->type;
        if ( !valT->isPointer() || !valT->firstType ) {
            context.error("can only safe dereference a pointer to a structure or handle " + valT->describe(),
                          at, CompilationError::cant_get_field);
            return;
        }
        value = autoDereference(value);
        if ( valT->firstType->structType ) {
            field = valT->firstType->structType->findField(name);
            if ( !field ) {
                context.error("can't get field " + name, at, CompilationError::cant_get_field);
                return;
            }
            type = make_shared<TypeDecl>(*field->type);
        } else if ( valT->firstType->isHandle() ) {
            annotation = valT->firstType->annotation;
            type = annotation->makeSafeFieldType(name);
            if ( !type ) {
                context.error("can't get field " + name, at, CompilationError::cant_get_field);
                return;
            }
        } else {
            context.error("can only safe dereference a pointer to a structure or handle " + valT->describe(),
                          at, CompilationError::cant_get_field);
            return;
        }
        skipQQ = type->isPointer();
        if ( !skipQQ ) {
            auto fieldType = type;
            type = make_shared<TypeDecl>(Type::tPointer);
            type->firstType = fieldType;
        }
        type->constant |= valT->constant;
    }
    
    SimNode * ExprSafeField::simulate (Context & context) const {
        if ( skipQQ ) {
            if ( annotation ) {
                return annotation->simulateSafeGetFieldPtr(name, context, at, value->simulate(context));
            } else {
                return context.makeNode<SimNode_SafeFieldDerefPtr>(at,value->simulate(context),field->offset);
            }
        } else {
            if ( annotation ) {
                return annotation->simulateSafeGetField(name, context, at, value->simulate(context));
            } else {
                return context.makeNode<SimNode_SafeFieldDeref>(at,value->simulate(context),field->offset);
            }
        }
    }
    
    // ExprVar
    
    ExpressionPtr ExprVar::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprVar::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprVar>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->variable = variable; // todo: lookup again?
        cexpr->local = local;
        cexpr->argument = argument;
        return cexpr;
    }
    
    void ExprVar::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        // local (that on the stack)
        for ( auto it = context.local.rbegin(); it!=context.local.rend(); ++it ) {
            auto var = *it;
            if ( var->name==name ) {
                variable = var;
                local = true;
                type = make_shared<TypeDecl>(*var->type);
                type->ref = true;
                return;
            }
        }
        // function argument
        argumentIndex = 0;
        for ( auto & arg : context.func->arguments ) {
            if ( arg->name==name ) {
                variable = arg;
                argument = true;
                type = make_shared<TypeDecl>(*arg->type);
                type->ref = arg->type->ref;
                return;
            }
            argumentIndex ++;
        }
        // global
        auto var = context.program->findVariable(name);
        if ( !var ) {
            context.error("can't locate variable " + name, at, CompilationError::variable_not_found);
        } else {
            variable = var;
            type = make_shared<TypeDecl>(*var->type);
            type->ref = true;
        }
    }
    
    SimNode * ExprVar::simulate (Context & context) const {
        if ( local ) {
            if ( variable->type->ref ) {
                return context.makeNode<SimNode_GetLocalRef>(at, variable->stackTop);
            } else {
                return context.makeNode<SimNode_GetLocal>(at, variable->stackTop);
            }
        } else if ( argument) {
            return context.makeNode<SimNode_GetArgument>(at, argumentIndex);
        } else {
            return context.makeNode<SimNode_GetGlobal>(at, variable->index);
        }
    }
    
    // ExprOp
    
    ExpressionPtr ExprOp::clone( const ExpressionPtr & expr ) const {
        if ( !expr ) {
            assert(0 && "can't clone ExprOp");
            return nullptr;
        }
        auto cexpr = static_pointer_cast<ExprOp>(expr);
        cexpr->op = op;
        cexpr->func = func;
        return cexpr;
    }
    
    // ExprOp1
    
    ExpressionPtr ExprOp1::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprOp1::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprOp1>(expr);
        ExprOp::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }
    
    void ExprOp1::inferType(InferTypeContext & context) {
        type.reset();
        subexpr->inferType(context);
        if ( !subexpr->type ) return;
        // infer
        vector<TypeDeclPtr> types = { subexpr->type };
        auto functions = context.program->findMatchingFunctions(op, types);
        if ( functions.size()==0 ) {
            string candidates = context.program->describeCandidates(context.program->findCandidates(op,types));
            context.error("no matching operator '" + op
                          + "' with argument (" + subexpr->type->describe() + ")", at, CompilationError::operator_not_found);
        } else if ( functions.size()>1 ) {
            string candidates = context.program->describeCandidates(functions);
            context.error("too many matching operators '" + op
                          + "' with argument (" + subexpr->type->describe() + ")", at, CompilationError::operator_not_found);
        } else {
            func = functions[0];
            type = make_shared<TypeDecl>(*func->result);
            if ( !func->arguments[0]->type->isRef() )
                subexpr = autoDereference(subexpr);
        }
        constexpression = subexpr->constexpression;
    }
    
    SimNode * ExprOp1::simulate (Context & context) const {
        if ( func->builtIn ) {
            auto pSimOp1 = static_cast<SimNode_Op1 *>(func->makeSimNode(context));
            pSimOp1->x = subexpr->simulate(context);
            return pSimOp1;
        } else {
            SimNode_Call * pCall = static_cast<SimNode_Call *>(func->makeSimNode(context));
            pCall->debug = at;
            pCall->fnIndex = func->index;
            pCall->arguments = (SimNode **) context.allocate(1 * sizeof(SimNode *));
            pCall->nArguments = 1;
            pCall->arguments[0] = subexpr->simulate(context);
            return pCall;
        }
    }
    
    // ExprOp2
    
    ExpressionPtr ExprOp2::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprOp2::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprOp2>(expr);
        ExprOp::clone(cexpr);
        cexpr->left = left->clone();
        cexpr->right = right->clone();
        return cexpr;
    }
    
    void ExprOp2::inferType(InferTypeContext & context) {
        type.reset();
        left->inferType(context);
        right->inferType(context);
        if ( !left->type || !right->type ) return;
        // infer
        if ( left->type->isPointer() && right->type->isPointer() )
            if ( !left->type->isSameType(*right->type,false) )
                context.error("operations on incompatible pointers are prohibited", at);
        vector<TypeDeclPtr> types = { left->type, right->type };
        auto functions = context.program->findMatchingFunctions(op, types);
        if ( functions.size()==0 ) {
            string candidates = context.program->describeCandidates(context.program->findCandidates(op,types));
            context.error("no matching operator '" + op
                + "' with arguments (" + left->type->describe() + ", " + right->type->describe()
                          + ")\n" + candidates, at, CompilationError::operator_not_found);
        } else if ( functions.size()>1 ) {
            string candidates = context.program->describeCandidates(functions);
            context.error("too many matching operators '" + op
                          + "' with arguments (" + left->type->describe() + ", " + right->type->describe()
                          + ")\n" + candidates, at, CompilationError::operator_not_found);
        } else {
            func = functions[0];
            type = make_shared<TypeDecl>(*func->result);
            if ( !func->arguments[0]->type->isRef() )
                left = autoDereference(left);
            if ( !func->arguments[1]->type->isRef() )
                right = autoDereference(right);
        }
        constexpression = left->constexpression && right->constexpression;
    }
    
    SimNode * ExprOp2::simulate (Context & context) const {
        if ( func->builtIn ) {
            auto pSimOp2 = static_cast<SimNode_Op2 *>(func->makeSimNode(context));
            pSimOp2->l = left->simulate(context);
            pSimOp2->r = right->simulate(context);
            return pSimOp2;
        } else {
            SimNode_Call * pCall = static_cast<SimNode_Call *>(func->makeSimNode(context));
            pCall->debug = at;
            pCall->fnIndex = func->index;
            pCall->arguments = (SimNode **) context.allocate(2 * sizeof(SimNode *));
            pCall->nArguments = 2;
            pCall->arguments[0] = left->simulate(context);
            pCall->arguments[1] = right->simulate(context);
            return pCall;
        }
    }

    // ExprOp3
    
    ExpressionPtr ExprOp3::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitLeft(this, left.get());
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprOp3::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprOp3>(expr);
        ExprOp::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->left = left->clone();
        cexpr->right = right->clone();
        return cexpr;
    }
    
    void ExprOp3::inferType(InferTypeContext & context) {
        type.reset();
        subexpr->inferType(context);
        left->inferType(context);
        right->inferType(context);
        if ( !subexpr->type || !left->type || !right->type ) return;
        // infer
        if ( op!="?" ) {
            context.error("Op3 currently only supports 'is'", at, CompilationError::operator_not_found);
            return;
        }
        subexpr = autoDereference(subexpr);
        if ( !subexpr->type->isSimpleType(Type::tBool) ) {
            context.error("cond operator condition must be boolean", at, CompilationError::condition_must_be_bool);
        } else if ( !left->type->isSameType(*right->type,false,false) ) {
            context.error("cond operator must return same types on both sides", at, CompilationError::operator_not_found);
        } else {
            if ( left->type->ref ^ right->type->ref ) { // if either one is not ref
                left = autoDereference(left);
                right = autoDereference(right);
            }
            type = make_shared<TypeDecl>(*left->type);
            type->constant |= right->type->constant;
        }
        constexpression = subexpr->constexpression && left->constexpression && right->constexpression;
    }
    
    SimNode * ExprOp3::simulate (Context & context) const {
        return context.makeNode<SimNode_IfThenElse>(at,
                                                    subexpr->simulate(context),
                                                    left->simulate(context),
                                                    right->simulate(context));
    }
    
    // common for move and copy
    
    SimNode * makeCopy(const LineInfo & at, Context & context, const TypeDecl & rightType, SimNode * left, SimNode * right ) {
        assert ( rightType.canCopy() && "should check above" );
        if ( rightType.isRef() ) {
            return context.makeNode<SimNode_CopyRefValue>(at, left, right, rightType.getSizeOf());
        } else if ( rightType.isHandle() ) {
            return rightType.annotation->simulateCopy(context, at, left, right);
        } else {
            return context.makeValueNode<SimNode_CopyValue>(rightType.baseType, at, left, right);
        }
    }
    
    SimNode * makeMove (const LineInfo & at, Context & context, const TypeDecl & rightType, SimNode * left, SimNode * right ) {
        assert ( !rightType.canCopy() && "should check above" );
        if ( rightType.ref ) {
            return context.makeNode<SimNode_MoveRefValue>(at, left, right, rightType.getSizeOf());
        } else if ( rightType.isGoodArrayType() ) {
            return context.makeNode<SimNode_CopyValue<Array>>(at, left, right);
        } else {
            assert(0 && "we should not be here");
            return nullptr;
        }
    }
    
    // ExprMove
    
    ExpressionPtr ExprMove::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }
    
    void ExprMove::inferType(InferTypeContext & context) {
        type.reset();
        left->inferType(context);
        right->inferType(context);
        if ( !left->type || !right->type ) return;
        // infer
        if ( !left->type->isSameType(*right->type,false,false) ) {
            context.error("can only move same type", at, CompilationError::operator_not_found);
        } else if ( !left->type->isRef() ) {
            context.error("can only move to reference", at, CompilationError::cant_write_to_non_reference);
        } else if ( left->type->constant ) {
            context.error("can't move to constant value", at, CompilationError::cant_move_to_const);
        } else if ( !left->type->canMove() ) {
            context.error("this type can't be moved", at, CompilationError::cant_move);
        } else if ( left->type->canCopy() ) {
            context.error("this type can be copied, use = instead", at, CompilationError::cant_move);
        }
        type = make_shared<TypeDecl>(*left->type);  // we return left
    }
    
    ExpressionPtr ExprMove::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMove>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }
    
    SimNode * ExprMove::simulate (Context & context) const {
        return makeMove(at,
                        context,
                        *right->type,
                        left->simulate(context),
                        right->simulate(context));
    }
    
    // ExprCopy
    
    ExpressionPtr ExprCopy::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }
    
    void ExprCopy::inferType(InferTypeContext & context) {
        type.reset();
        left->inferType(context);
        right->inferType(context);
        if ( !left->type || !right->type ) return;
        // infer
        if ( !left->type->isSameType(*right->type,false,false) ) {
            context.error("can only copy same type " + left->type->describe() + " vs " + right->type->describe(),
                          at, CompilationError::operator_not_found);
        } else if ( !left->type->isRef() ) {
            context.error("can only copy to reference", at, CompilationError::cant_write_to_non_reference);
        } else if ( left->type->constant ) {
            context.error("can't write to constant value", at, CompilationError::cant_write_to_const);
        }
        if ( !left->type->canCopy() ) {
            context.error("this type can't be copied, use <- instead", at, CompilationError::cant_copy);
        }
        type = make_shared<TypeDecl>(*left->type);  // we return left
    }
    
    ExpressionPtr ExprCopy::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCopy>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }
    
    SimNode * ExprCopy::simulate (Context & context) const {
        return makeCopy(at, context, *right->type, left->simulate(context), right->simulate(context));
    }
    
    // ExprTryCatch
    
    ExpressionPtr ExprTryCatch::visit(Visitor & vis) {
        vis.preVisit(this);
        try_block = try_block->visit(vis);
        vis.preVisitCatch(this,catch_block.get());
        catch_block = catch_block->visit(vis);
        return vis.visit(this);
    }
    
    uint32_t ExprTryCatch::getEvalFlags() const {
        return (try_block->getEvalFlags() | catch_block->getEvalFlags()) & ~EvalFlags::stopForThrow;
    }
    
    void ExprTryCatch::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        try_block->inferType(context);
        catch_block->inferType(context);
    }
    
    SimNode * ExprTryCatch::simulate (Context & context) const {
        return context.makeNode<SimNode_TryCatch>(at,
                                                  try_block->simulate(context),
                                                  catch_block->simulate(context));
    }
    
    ExpressionPtr ExprTryCatch::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTryCatch>(expr);
        Expression::clone(cexpr);
        cexpr->try_block = try_block->clone();
        cexpr->catch_block = catch_block->clone();
        return cexpr;
    }
    
    // ExprReturn
    
    ExpressionPtr ExprReturn::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprReturn::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprReturn>(expr);
        Expression::clone(cexpr);
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        return cexpr;
    }
    
    void ExprReturn::inferType(InferTypeContext & context) {
        type.reset();
        if ( subexpr ) {
            subexpr->argLevel = true;
            subexpr->inferType(context);
            if ( !subexpr->type ) return;
            subexpr = autoDereference(subexpr);
        }
        // infer
        auto resType = context.func->result;
        if ( resType->isVoid() ) {
            if ( subexpr ) {
                context.error("void function has no return", at);
            }
        } else {
            if ( !subexpr ) {
                context.error("must return value", at);
            } else {
                if ( !resType->isSameType(*subexpr->type,true,false) ) {
                    context.error("incompatible return type, expecting "
                                  + resType->describe() + " vs " + subexpr->type->describe(),
                                  at, CompilationError::invalid_return_type);
                }
                if ( resType->isPointer() && !resType->isConst() && subexpr->type->isConst() ) {
                    context.error("incompatible return type, constant matters. expecting "
                                  + resType->describe() + " vs " + subexpr->type->describe(),
                                  at, CompilationError::invalid_return_type);
                }
                type = make_shared<TypeDecl>(*context.func->result);
                type->ref = true;   // we return func-result &
            }
        }
    }
    
    SimNode * ExprReturn::simulate (Context & context) const {
        return context.makeNode<SimNode_Return>(at, subexpr ? subexpr->simulate(context) : nullptr);
    }
    
    // ExprBreak
    
    ExpressionPtr ExprBreak::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprBreak::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprBreak>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }
    
    void ExprBreak::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        if ( !context.loop.size() )
            context.error("break without loop", at);
    }
    
    SimNode * ExprBreak::simulate (Context & context) const {
        return context.makeNode<SimNode_Break>(at);
    }
    
    // CONSTANTS
    
    ExpressionPtr ExprConstPtr::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprConstInt::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprConstUInt::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprConstBool::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprConstFloat::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprConstString::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    // ExprIfThenElse
    
    ExpressionPtr ExprIfThenElse::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        vis.preVisitIfBlock(this, if_true.get());
        if_true = if_true->visit(vis);
        if ( if_false ) {
            vis.preVisitElseBlock(this, if_false.get());
            if_false = if_false->visit(vis);
        }
        return vis.visit(this);
    }
    
    ExpressionPtr ExprIfThenElse::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprIfThenElse>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->if_true = if_true->clone();
        if ( if_false )
            cexpr->if_false = if_false->clone();
        return cexpr;
    }
    
    void ExprIfThenElse::inferType(InferTypeContext & context) {
        type.reset();
        cond->topLevel = true;
        cond->inferType(context);
        if_true->inferType(context);
        if ( if_false )
            if_false->inferType(context);
        if ( !cond->type ) return;
        // infer
        if ( !cond->type->isSimpleType(Type::tBool) ) {
            context.error("if-then-else condition must be boolean", at, CompilationError::condition_must_be_bool);
        }
    }
    
    SimNode * ExprIfThenElse::simulate (Context & context) const {
        return context.makeNode<SimNode_IfThenElse>(at, cond->simulate(context), if_true->simulate(context),
                                                    if_false ? if_false->simulate(context) : nullptr);
    }

    // ExprWhile
    
    ExpressionPtr ExprWhile::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        vis.preVisitWhileBody(this, body.get());
        body = body->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprWhile::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprWhile>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->body = body->clone();
        return cexpr;
    }
    
    uint32_t ExprWhile::getEvalFlags() const {
        return body->getEvalFlags() & ~EvalFlags::stopForBreak;
    }
    
    void ExprWhile::inferType(InferTypeContext & context) {
        type.reset();
        cond->topLevel = true;
        cond->inferType(context);
        if ( !cond->type ) return;
        // infer
        if ( !cond->type->isSimpleType(Type::tBool) ) {
            context.error("while loop condition must be boolean", at);
        } else {
            context.loop.push_back(shared_from_this());
            body->inferType(context);
            context.loop.pop_back();        }
    }

    SimNode * ExprWhile::simulate (Context & context) const {
        return context.makeNode<SimNode_While>(at, cond->simulate(context),body->simulate(context));
    }
    
    // ExprFor

    ExpressionPtr ExprFor::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & var : iteratorVariables ) {
            vis.preVisitFor(this, var, var==iteratorVariables.back());
            var = vis.visitFor(this, var, var==iteratorVariables.back());
        }
        for ( auto & src : sources ) {
            vis.preVisitForSource(this, src.get(), src==sources.back());
            src = src->visit(vis);
            src = vis.visitForSource(this, src.get(), src==sources.back());
        }
        if ( filter ) {
            vis.preVisitForFilter(this, filter.get());
            filter = filter->visit(vis);
        }
        vis.preVisitForBody(this, subexpr.get());
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprFor::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprFor>(expr);
        Expression::clone(cexpr);
        cexpr->iterators = iterators;
        for ( auto & src : sources )
            cexpr->sources.push_back(src->clone());
        for ( auto & var : iteratorVariables )
            cexpr->iteratorVariables.push_back(var->clone());
        cexpr->subexpr = subexpr->clone();
        if ( filter )
            cexpr->filter = filter->clone();
        return cexpr;
    }
    
    Variable * ExprFor::findIterator(const string & name) const {
        for ( auto & v : iteratorVariables ) {
            if ( v->name==name ) {
                return v.get();
            }
        }
        return nullptr;
    }
    
    uint32_t ExprFor::getEvalFlags() const {
        return subexpr->getEvalFlags() & ~EvalFlags::stopForBreak;
    }
    
    void ExprFor::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        if ( !iterators.size() ) {
            context.error("for needs at least one iterator", at);
            return;
        } else if ( iterators.size() != sources.size() ) {
            context.error("for needs as many iterators as there are sources", at);
            return;
        } else if ( sources.size()>MAX_FOR_ITERATORS ) {
            context.error("too many sources for now", at);
            return;
        }
        auto sp = context.stackTop;
        auto sz = context.local.size();
        // determine iteration types
        nativeIterators = false;
        fixedArrays = false;
        dynamicArrays = false;
        rangeBase = false;
        fixedSize = UINT16_MAX;
        for ( auto & src : sources ) {
            src->inferType(context);
            if ( !src->type ) return;
            if ( src->type->isArray() ) {
                fixedSize = min(fixedSize, src->type->dim.back());
                fixedArrays = true;
            } else if ( src->type->isGoodArrayType() ) {
                dynamicArrays = true;
            } else if ( src->type->isGoodIteratorType() ) {
                nativeIterators = true;
            } else if ( src->type->isHandle() ) {
                nativeIterators = true;
            } else if ( src->type->isRange() ) {
                rangeBase = true;
            }
        }
        int idx = 0;
        for ( auto & src : sources ) {
            if ( !src->type ) return;
            auto pVar = make_shared<Variable>();
            pVar->name = iterators[idx];
            pVar->at = at;
            if ( src->type->dim.size() ) {
                pVar->type = make_shared<TypeDecl>(*src->type);
                pVar->type->ref = true;
                pVar->type->dim.pop_back();
            } else if ( src->type->isGoodIteratorType() ) {
                pVar->type = make_shared<TypeDecl>(*src->type->firstType);
            } else if ( src->type->isGoodArrayType() ) {
                pVar->type = make_shared<TypeDecl>(*src->type->firstType);
                pVar->type->ref = true;
            } else if ( src->type->isRange() ) {
                pVar->type = make_shared<TypeDecl>(src->type->getRangeBaseType());
                pVar->type->ref = false;
            } else if ( src->type->isHandle() && src->type->annotation->isIterable() ) {
                pVar->type = make_shared<TypeDecl>(*src->type->annotation->makeIteratorType());
            } else {
                context.error("unsupported iteration type for " + pVar->name, at);
                return;
            }
            context.local.push_back(pVar);
            pVar->stackTop = context.stackTop;
            context.stackTop += (pVar->type->getSizeOf() + 0xf) & ~0xf;
            iteratorVariables.push_back(pVar);
            ++ idx;
        }
        if ( filter ) {
            filter->inferType(context);
            if ( !filter->type ) return;
            if ( !filter->type->isSimpleType(Type::tBool) ) {
                context.error("where clause must be boolean", at);
            }
        }
        subexpr->inferType(context);
        context.func->totalStackSize = max(context.func->totalStackSize, context.stackTop);
        context.stackTop = sp;
        context.local.resize(sz);
    }

    SimNode * ExprFor::simulate (Context & context) const {
        int  total = sources.size();
        int  sourceTypes = int(dynamicArrays) + int(fixedArrays) + int(rangeBase);
        bool hybridRange = rangeBase && (total>1);
        if ( (sourceTypes>1) || hybridRange || nativeIterators ) {
            SimNode_ForWithIteratorBase * result = (SimNode_ForWithIteratorBase *)
                context.makeNodeUnroll<SimNode_ForWithIterator>(total, at);
            for ( int t=0; t!=total; ++t ) {
                if ( sources[t]->type->isGoodIteratorType() ) {
                    result->source_iterators[t] = sources[t]->simulate(context);
                } else if ( sources[t]->type->isGoodArrayType() ) {
                    result->source_iterators[t] = context.makeNode<SimNode_GoodArrayIterator>(
                        sources[t]->at,
                        sources[t]->simulate(context),
                        sources[t]->type->firstType->getStride());
                } else if ( sources[t]->type->isRange() ) {
                    result->source_iterators[t] = context.makeNode<SimNode_RangeIterator>(
                        sources[t]->at,
                        sources[t]->simulate(context));
                } else if ( sources[t]->type->isHandle() ) {
                    result->source_iterators[t] = sources[t]->type->annotation->simulateGetIterator(
                         context,
                         sources[t]->at,
                         sources[t]->simulate(context)
                    );
                } else if ( sources[t]->type->dim.size() ) {
                    result->source_iterators[t] = context.makeNode<SimNode_FixedArrayIterator>(
                        sources[t]->at,
                        sources[t]->simulate(context),
                        sources[t]->type->dim.back(),
                        sources[t]->type->getStride());
                } else {
                    assert(0 && "we should not be here yet");
                    return nullptr;
                }
                result->stackTop[t] = iteratorVariables[t]->stackTop;
            }
            result->body = subexpr->simulate(context);
            result->filter = filter ? filter->simulate(context) : nullptr;
            return result;
        } else {
            SimNode_ForBase * result;
            if ( dynamicArrays ) {
                result = (SimNode_ForBase *) context.makeNodeUnroll<SimNode_ForGoodArray>(total, at);
            } else if ( fixedArrays ) {
                result = (SimNode_ForBase *) context.makeNodeUnroll<SimNode_ForFixedArray>(total, at);
            } else if ( rangeBase ) {
                assert(total==1 && "simple range on 1 loop only");
                result = context.makeNode<SimNode_ForRange>(at);
            } else {
                assert(0 && "we should not be here yet");
                return nullptr;
            }
            for ( int t=0; t!=total; ++t ) {
                result->sources[t] = sources[t]->simulate(context);
                if ( sources[t]->type->isGoodArrayType() ) {
                    result->strides[t] = sources[t]->type->firstType->getStride();
                } else {
                    result->strides[t] = sources[t]->type->getStride();
                }
                result->stackTop[t] = iteratorVariables[t]->stackTop;
            }
            result->size = fixedSize;
            result->body = subexpr->simulate(context);
            result->filter = filter ? filter->simulate(context) : nullptr;
            return result;
        }
    }
    
    // ExprLet
    
    ExpressionPtr ExprLet::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & var : variables ) {
            vis.preVisitLet(this, var, var==variables.back());
            if ( var->init ) {
                vis.preVisitLetInit(this, var->init.get());
                var->init = var->init->visit(vis);
                var->init = vis.visitLetInit(this, var->init.get());
            }
            var = vis.visitLet(this, var, var==variables.back());
        }
        if ( subexpr )
            subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }
    
    ExpressionPtr ExprLet::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprLet>(expr);
        Expression::clone(cexpr);
        for ( auto & var : variables )
            cexpr->variables.push_back(var->clone());
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    Variable * ExprLet::find(const string & name) const {
        for ( auto & v : variables ) {
            if ( v->name==name ) {
                return v.get();
            }
        }
        return nullptr;
    }
    
    void ExprLet::setBlockReturnsValue() {
        returnsValue = true;
        if ( subexpr ) {
            subexpr->setBlockReturnsValue();
        }
    }
    
    uint32_t ExprLet::getEvalFlags() const {
        return subexpr ? subexpr->getEvalFlags() : 0;
    }
    
    void ExprLet::inferType(InferTypeContext & context) {
        type.reset();
        // infer
        auto sp = context.stackTop;
        auto sz = context.local.size();
        for ( auto & var : variables ) {
            if ( var->type->ref )
                context.error("local variable can't be reference", var->at, CompilationError::invalid_variable_type);
            if ( var->type->isVoid() )
                context.error("local variable can't be void", var->at, CompilationError::invalid_variable_type);
            if ( var->type->isHandle() && !var->type->annotation->isLocal() )
                context.error("handled type " + var->type->annotation->name + " can't be local", var->at, CompilationError::invalid_variable_type);
            context.local.push_back(var);
            if ( var->init ) {
                var->init->inferType(context);
                if ( !var->init->type ) {
                    var->init->inferType(context);
                    return;
                }
                if ( !var->type->isSameType(*var->init->type,false) ) {
                    context.error("variable initialization type mismatch, "
                        + var->type->describe() + " = " + var->init->type->describe(), var->at );
                } else if ( var->type->baseType==Type::tStructure ) {
                    context.error("can't initialize structures", var->at );
                } else if ( !var->init->type->canCopy() && !var->init->type->canMove() ) {
                    context.error("this variable can't be initialized at all", var->at);
                }
            }
            var->stackTop = context.stackTop;
            context.stackTop += (var->type->getSizeOf() + 0xf) & ~0xf;
        }
        if ( subexpr )
            subexpr->inferType(context);
        // block type
        if ( returnsValue && subexpr && subexpr->type ) {
            subexpr = autoDereference(subexpr);
            type = make_shared<TypeDecl>(*subexpr->type);
        }
        context.func->totalStackSize = max(context.func->totalStackSize, context.stackTop);
        if ( scoped ) {
            context.stackTop = sp;
            context.local.resize(sz);
        }
    }
    
    SimNode * ExprLet::simulateInit(Context & context, const VariablePtr & var, bool local) {
        SimNode * init = var->init->simulate(context);
        SimNode * get;
        if ( local )
            get = context.makeNode<SimNode_GetLocal>(var->init->at, var->stackTop);
        else
            get = context.makeNode<SimNode_GetGlobal>(var->init->at, var->index);
        if ( var->type->canCopy() )
            return makeCopy(var->init->at, context, *var->init->type, get, init);
        else if ( var->type->canMove() )
            return makeMove(var->init->at, context, *var->init->type, get, init);
        else {
            assert(0 && "we should not be here");
            return nullptr;
        }
    }
    
    SimNode * ExprLet::simulate (Context & context) const {
        auto let = context.makeNode<SimNode_Let>(at);
        let->total = (uint32_t) variables.size();
        let->list = (SimNode **) context.allocate(let->total * sizeof(SimNode*));
        int vi = 0;
        for ( auto & var : variables ) {
            if ( var->init ) {
                let->list[vi++] = simulateInit(context, var, true);
            } else {
                let->list[vi++] = context.makeNode<SimNode_InitLocal>(at, var->stackTop, var->type->getSizeOf());
            }
        }
        let->subexpr = subexpr ? subexpr->simulate(context) : nullptr;
        return let;
    }
    
    // ExprLooksLikeCall
    
    ExpressionPtr ExprLooksLikeCall::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            arg = arg->visit(vis);
        }
        return vis.visit(this);
    }
    
    ExpressionPtr ExprLooksLikeCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprLooksLikeCall>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        return cexpr;
    }
    
    string ExprLooksLikeCall::describe() const {
        stringstream stream;
        stream << "(" << name;
        for ( auto & arg : arguments ) {
            if ( arg->type )
                stream << " " << *arg->type;
            else
                stream << " ???";
        }
        stream << ")";
        return stream.str();
    }
    
    void ExprLooksLikeCall::inferType(InferTypeContext & context) {
        argumentsFailedToInfer = false;
        for ( auto & ar : arguments ) {
            ar->argLevel = true;
            ar->inferType(context);
            if ( !ar->type ) argumentsFailedToInfer = true;
        }
    }
    
    void ExprLooksLikeCall::autoDereference() {
        for ( size_t iA = 0; iA != arguments.size(); ++iA ) {
            arguments[iA] = Expression::autoDereference(arguments[iA]);
        }
    }
    
    // ExprCall
    
    ExpressionPtr ExprCall::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitCallArg(this, arg.get(), arg==arguments.back());
            arg = arg->visit(vis);
            arg = vis.visitCallArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }
    
    ExpressionPtr ExprCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCall>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->func = func;
        return cexpr;
    }
    
    void ExprCall::inferType(InferTypeContext & context) {
        type.reset();
        ExprLooksLikeCall::inferType(context);
        if ( argumentsFailedToInfer ) return;
        // infer
        vector<TypeDeclPtr> types;
        types.reserve(arguments.size());
        for ( auto & ar : arguments ) {
            types.push_back(ar->type);
        }
        auto functions = context.program->findMatchingFunctions(name, types);
        if ( functions.size()==0 ) {
            string candidates = context.program->describeCandidates(context.program->findCandidates(name,types));
            context.error("no matching function " + describe() + "\n" + candidates, at, CompilationError::function_not_found);
        } else if ( functions.size()>1 ) {
            string candidates = context.program->describeCandidates(functions);
            context.error("too many matching functions " + describe() + "\n" + candidates, at, CompilationError::function_not_found);
        } else {
            func = functions[0];
            type = make_shared<TypeDecl>(*func->result);
            for ( size_t iT = arguments.size(); iT != func->arguments.size(); ++iT ) {
                auto newArg = func->arguments[iT]->init->clone();
                if ( !newArg->type )
                    newArg->inferType(context);
                arguments.push_back(newArg);
            }
            for ( size_t iA = 0; iA != arguments.size(); ++iA )
                if ( !func->arguments[iA]->type->isRef() )
                    arguments[iA] = Expression::autoDereference(arguments[iA]);
        }
    }
    
    SimNode * ExprCall::simulate (Context & context) const {
        SimNode_Call * pCall = static_cast<SimNode_Call *>(func->makeSimNode(context));
        pCall->debug = at;
        pCall->fnIndex = func->index;
        if ( int nArg = (int) arguments.size() ) {
            pCall->arguments = (SimNode **) context.allocate(nArg * sizeof(SimNode *));
            pCall->nArguments = nArg;
            for ( int a=0; a!=nArg; ++a ) {
                pCall->arguments[a] = arguments[a]->simulate(context);
            }
        } else {
            pCall->arguments = nullptr;
            pCall->nArguments = 0;
        }
        return pCall;
    }

    // program
    
    
    vector<AnnotationPtr> Program::findAnnotation ( const string & name ) const {
        return library.findAnnotation(name);
    }
    
    vector<StructurePtr> Program::findStructure ( const string & name ) const {
        return library.findStructure(name);
    }
    
    VariablePtr Program::findVariable ( const string & name ) const {
        return thisModule->findVariable(name);
    }
    
    void Program::inferTypes() {
        // structure declarations (precompute offsets of fields)
        for ( auto & ist : thisModule->structures ) {
            auto & st = ist.second;
            int offset = 0;
            for ( auto & fi : st->fields ) {
                fi.offset = offset;
                offset += fi.type->getSizeOf();
            }
        }
        // global variables
        int gvi = 0;
        for ( auto & it : thisModule->globals ) {
            auto pvar = it.second;
            pvar->index = gvi ++;
            if ( pvar->type->ref )
                error("global variable can't be reference", pvar->at, CompilationError::invalid_variable_type);
            if ( pvar->type->isVoid() )
                error("global variable can't be void", pvar->at, CompilationError::invalid_variable_type);
            if ( pvar->type->isHandle() && !pvar->type->annotation->isLocal() )
                error("handled type " + pvar->type->annotation->name + "can't be global", pvar->at, CompilationError::invalid_variable_type);
            if ( pvar->init ) {
                Expression::InferTypeContext context;
                pvar->init->inferType(context);
                if ( failed() )
                    return;
            }
        }
        // functions
        totalFunctions = 0;
        for ( auto & fit : thisModule->functions ) {
            Expression::InferTypeContext context;
            context.program = shared_from_this();
            context.func = fit.second;
            if ( !context.func->builtIn ) {
                context.func->totalStackSize = context.stackTop = sizeof(Prologue);
                context.func->index = totalFunctions ++;
                for ( auto & arg : context.func->arguments ) {
                    if ( arg->init ) {
                        arg->init->inferType(context);
                        if ( failed() )
                            return;
                        if ( !arg->type->isSameType(*arg->init->type, true) ) {
                            context.error("function argument default value type mismatch", arg->init->at);
                        }
                    }
                }
                context.func->body->inferType(context);
            }
        }
    }
    
    vector<FunctionPtr> Program::findCandidates ( const string & name, const vector<TypeDeclPtr> & types ) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        vector<FunctionPtr> result;
        library.foreach([&](Module * mod) -> bool {
            auto itFnList = mod->functionsByName.find(funcName);
            if ( itFnList != mod->functionsByName.end() ) {
                auto & goodFunctions = itFnList->second;
                result.insert(result.end(), goodFunctions.begin(), goodFunctions.end());
            }
            return true;
        },moduleName);
        return result;
    }
        
    vector<FunctionPtr> Program::findMatchingFunctions ( const string & name, const vector<TypeDeclPtr> & types ) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        vector<FunctionPtr> result;
        library.foreach([&](Module * mod) -> bool {
            auto itFnList = mod->functionsByName.find(funcName);
            if ( itFnList != mod->functionsByName.end() ) {
                auto & goodFunctions = itFnList->second;
                for ( auto & pFn : goodFunctions ) {
                    if ( pFn->arguments.size() >= types.size() ) {
                        bool typesCompatible = true;
                        for ( auto ai = 0; ai != types.size(); ++ai ) {
                            auto & argType = pFn->arguments[ai]->type;
                            auto & passType = types[ai];
                            if ( passType && ((argType->isRef() && !passType->isRef()) || !argType->isSameType(*passType, false, false)) ) {
                                typesCompatible = false;
                                break;
                            }
                            // ref types can only add constness
                            if ( argType->isRef() && !argType->constant && passType->constant ) {
                                typesCompatible = false;
                                break;
                            }
                            // pointer types can only add constant
                            if ( argType->isPointer() && !argType->constant && passType->constant ) {
                                typesCompatible = false;
                                break;
                            }
                        }
                        bool tailCompatible = true;
                        for ( auto ti = types.size(); ti != pFn->arguments.size(); ++ti ) {
                            if ( !pFn->arguments[ti]->init ) {
                                tailCompatible = false;
                            }
                        }
                        if ( typesCompatible && tailCompatible ) {
                            result.push_back(pFn);
                        }
                    }
                }
            }
            return true;
        },moduleName);
        return result;
    }
    
    FuncInfo * Program::makeFunctionDebugInfo ( Context & context, const Function & fn ) {
        FuncInfo * fni = context.makeNode<FuncInfo>();
        fni->name = context.allocateName(fn.name);
        fni->stackSize = fn.totalStackSize;
        fni->argsSize = (uint32_t) fn.arguments.size();
        fni->args = (VarInfo **) context.allocate(sizeof(VarInfo *) * fni->argsSize);
        for ( uint32_t i=0; i!=fni->argsSize; ++i ) {
            fni->args[i] = makeVariableDebugInfo(context, *fn.arguments[i]);
        }
        return fni;
    }
    
    StructInfo * Program::makeStructureDebugInfo ( Context & context, const Structure & st ) {
        StructInfo * sti = context.makeNode<StructInfo>();
        sti->name = context.allocateName(st.name);
        sti->fieldsSize = (uint32_t) st.fields.size();
        sti->fields = (VarInfo **) context.allocate( sizeof(VarInfo *) * sti->fieldsSize );
        for ( uint32_t i=0; i!=sti->fieldsSize; ++i ) {
            auto & var = st.fields[i];
            VarInfo * vi = context.makeNode<VarInfo>();
            makeTypeInfo(vi, context, var.type);
            vi->name = context.allocateName(var.name);
            sti->fields[i] = vi;
        }
        return sti;
    }
    
    void Program::makeTypeInfo ( TypeInfo * info, Context & context, const TypeDeclPtr & type ) {
        info->type = type->baseType;
        info->dimSize = (uint32_t) type->dim.size();
        info->annotation = type->annotation.get();
        if ( info->dimSize ) {
            info->dim = (uint32_t *) context.allocate(sizeof(uint32_t) * info->dimSize );
            for ( uint32_t i=0; i != info->dimSize; ++i ) {
                info->dim[i] = type->dim[i];
            }
        }
        if ( type->baseType==Type::tStructure  ) {
            auto st = sdebug.find(type->structType->name);
            if ( st==sdebug.end() ) {
                info->structType = makeStructureDebugInfo(context, *type->structType);
                sdebug[type->structType->name] = info->structType;
            } else {
                info->structType = st->second;
            }
        }
        info->ref = type->ref;
        if ( type->isRefType() )
            info->ref = false;
        info->canCopy = type->canCopy();
        info->isPod = type->isPod();
        if ( type->firstType ) {
            info->firstType = context.makeNode<TypeInfo>();
            makeTypeInfo(info->firstType, context, type->firstType);
        } else {
            info->firstType = nullptr;
        }
        if ( type->secondType ) {
            info->secondType = context.makeNode<TypeInfo>();
            makeTypeInfo(info->secondType , context, type->secondType);
        } else {
            info->secondType = nullptr;
        }
    }

    VarInfo * Program::makeVariableDebugInfo ( Context & context, const Variable & var ) {
        VarInfo * vi = context.makeNode<VarInfo>();
        makeTypeInfo(vi, context, var.type);
        vi->name = context.allocateName(var.name);
        return vi;
    }
    
    void Program::simulate ( Context & context ) {
        context.thisProgram = this;
        context.globalVariables = (GlobalVariable *) context.allocate( uint32_t(thisModule->globals.size()*sizeof(GlobalVariable)) );
        for ( auto & it : thisModule->globals ) {
            auto pvar = it.second;
            auto & gvar = context.globalVariables[pvar->index];
            gvar.name = context.allocateName(pvar->name);
            gvar.size = pvar->type->getSizeOf();
            gvar.debug = makeVariableDebugInfo(context, *it.second);
            gvar.value = cast<void *>::from(context.allocate(gvar.size));
            gvar.init = pvar->init ? ExprLet::simulateInit(context, pvar, false) : nullptr;
        }
        context.totalVariables = (int) thisModule->globals.size();
        context.functions = (SimFunction *) context.allocate( totalFunctions*sizeof(SimFunction) );
        context.totalFunctions = totalFunctions;
        for ( auto & it : thisModule->functions ) {
            auto pfun = it.second;
            if ( pfun->index==-1 )
                continue;
            auto & gfun = context.functions[pfun->index];
            gfun.name = context.allocateName(pfun->name);
            gfun.code = pfun->simulate(context);
            gfun.stackSize = pfun->totalStackSize;
            gfun.debug = makeFunctionDebugInfo(context, *pfun);
        }
        sdebug.clear();
        context.thisProgram = nullptr;
        context.linearAllocatorExecuteBase = context.linearAllocator;
        context.restart();
        context.runInitScript();
        context.restart();
    }
    
    void Program::error ( const string & str, const LineInfo & at, CompilationError cerr ) {
        // cout << "ERROR: " << str << ", at " << at.describe() << "\n";
        errors.emplace_back(str,at,cerr);
        failToCompile = true;
    }
    
    void Program::addModule ( Module * pm ) {
        library.addModule(pm);
    }
    
    bool Program::addVariable ( const VariablePtr & var ) {
        return thisModule->addVariable(var);
    }
    
    bool Program::addStructure ( const StructurePtr & st ) {
        return thisModule->addStructure(st);
    }
    
    bool Program::addFunction ( const FunctionPtr & fn ) {
        return thisModule->addFunction(fn);
    }
    
    bool Program::addStructureHandle ( const StructurePtr & st, const TypeAnnotationPtr & ann, const AnnotationArgumentList & arg ) {
        if ( ann->isStructureAnnotation() ) {
            auto annotation = static_pointer_cast<StructureTypeAnnotation>(ann->clone());
            annotation->name = st->name;
            string err;
            if ( annotation->create(st,arg,err) ) {
                thisModule->addAnnotation(annotation);
                return true;
            } else {
                error("can't create structure handle "+ann->name + "\n" + err,st->at,CompilationError::invalid_annotation);
                return false;
            }
        } else {
            error("not a structure annotation "+ann->name,st->at,CompilationError::invalid_annotation);
            return false;
        }
    }
    
    Program::Program() {
        thisModule = make_unique<Module>();
        library.addBuiltInModule();
        library.addModule(thisModule.get());
    }
    
    TypeDecl * Program::makeTypeDeclaration(const LineInfo &at, const string &name) {
        auto structs = findStructure(name);
        auto handles = findAnnotation(name);
        if ( structs.size() && handles.size() ) {
            string candidates = describeCandidates(structs);
            candidates += describeCandidates(handles, false);
            error("undefined type "+name + "\n" + candidates,at,CompilationError::type_not_found);
            return nullptr;
        } else if ( structs.size() ) {
            if ( structs.size()==1 ) {
                auto pTD = new TypeDecl(Type::tStructure);
                pTD->structType = structs.back();
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(structs);
                error("too many options for "+name + "\n" + candidates,at,CompilationError::structure_not_found);
                return nullptr;
            }
        } else if ( handles.size() ) {
            if ( handles.size()==1 ) {
                if ( handles.back()->isHandledTypeAnnotation() ) {
                    auto pTD = new TypeDecl(Type::tHandle);
                    pTD->annotation = static_pointer_cast<TypeAnnotation>(handles.back());
                    pTD->at = at;
                    return pTD;
                } else {
                    error("not a handled type annotation "+name,at,CompilationError::handle_not_found);
                    return nullptr;
                }
            } else {
                string candidates = describeCandidates(handles);
                error("too many options for "+name + "\n" + candidates,at,CompilationError::handle_not_found);
                return nullptr;
            }
        } else {
            error("undefined type " + name,at,CompilationError::type_not_found);
            return nullptr;
        }
    }
    
    ExprLooksLikeCall * Program::makeCall ( const LineInfo & at, const string & name ) {
        vector<ExprCallFactory *> ptr;
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        library.foreach([&](Module * pm) -> bool {
            if ( auto pp = pm->findCall(funcName) )
                ptr.push_back(pp);
            return false;
        }, moduleName);
        if ( ptr.size()==1 ) {
            return (*ptr.back())(at);
        } else if ( ptr.size()==0 ) {
            return new ExprCall(at,name);
        } else {
            error("too many options for " + name, at, CompilationError::function_not_found);
            return new ExprCall(at,name);
        }
    }
    
    void Program::visit(Visitor & vis) {
        for ( auto & fn : thisModule->functions ) {
            fn.second = fn.second->visit(vis);
        }
    }

    // PARSER
    
    ProgramPtr g_Program;
    
    ProgramPtr parseDaScript ( const char * script ) {
        int err;
        auto program = g_Program = make_shared<Program>();
        yybegin(script);
        err = yyparse();        // TODO: add mutex or make thread safe?
        yylex_destroy();
        g_Program.reset();
        if ( err || program->failed() ) {
            sort(program->errors.begin(),program->errors.end());
            return program;
        } else {
            program->inferTypes();
            sort(program->errors.begin(),program->errors.end());
            return program;
        }
    }
}
