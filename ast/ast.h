#pragma once

#include "simulate.h"
#include "vectypes.h"
#include "arraytype.h"
#include "function_traits.h"
#include "interop.h"
#include "debug_info.h"

namespace yzg
{
    using namespace std;
    
    class Structure;
    typedef shared_ptr<Structure> StructurePtr;
    
    class Function;
    typedef shared_ptr<Function> FunctionPtr;
    
    struct Variable;
    typedef shared_ptr<Variable> VariablePtr;
    
    struct Expression;
    typedef shared_ptr<Expression> ExpressionPtr;
    
    class Program;
    typedef shared_ptr<Program> ProgramPtr;
    
    struct TypeDecl;
    typedef shared_ptr<TypeDecl> TypeDeclPtr;
    
    class Module;
    typedef shared_ptr<Module> ModulePtr;
    
    class ModuleLibrary;
    
    enum class Operator {
        none,
        // 2-char
        r2l,
        p2r,
        addEqu,
        subEqu,
        divEqu,
        mulEqu,
        modEqu,
        andEqu,
        orEqu,
        xorEqu,
        eqEq,
        lessEqu,
        greaterEqu,
        notEqu,
        binNotEqu,
        inc,
        dec,
        postInc,
        postDec,
        // 1-char
        at,         // @
        dot,        // .
        binand,
        binor,
        binxor,
        add,
        sub,
        div,
        mul,
        mod,
        is,         // ?
        boolNot,    // !
        binNot,     // ~
        less,
        greater
    };
    
    string to_string ( Operator op );
    bool isUnaryOperator ( Operator op );
    bool isBinaryOperator ( Operator op );
    bool isTrinaryOperator ( Operator op );
    
    struct TypeDecl : enable_shared_from_this<TypeDecl> {
        TypeDecl() = default;
        TypeDecl(const TypeDecl & decl);
        TypeDecl & operator = (const TypeDecl & decl) = delete;
        TypeDecl(Type tt) : baseType(tt) {}
        friend ostream& operator<< (ostream& stream, const TypeDecl & decl);
        string getMangledName() const;
        bool isSameType ( const TypeDecl & decl, bool refMatters = true ) const;
        bool isIteratorType ( const TypeDecl & decl ) const;
        bool isSimpleType () const;
        bool isSimpleType ( Type typ ) const;
        bool isArray() const;
        bool isGoodIteratorType() const;
        bool isGoodArrayType() const;
        bool isGoodTableType() const;
        bool isVoid() const;
        bool isRef() const;
        bool isRefType() const;
        bool isIndex() const;
        bool isPointer() const;
        int getSizeOf() const;
        int getBaseSizeOf() const;
        int getStride() const;
        string describe() const { stringstream ss; ss << *this; return ss.str(); }
        bool canCopy() const;
        bool isPod() const;
        bool isWorkhorseType() const; // we can return this, or pass this,
        Type                baseType = Type::tVoid;
        Structure *         structType = nullptr;
        TypeDeclPtr         firstType;      // map.first or array
        TypeDeclPtr         secondType;     // map.second
        vector<uint32_t>    dim;
        bool                ref = false;
        LineInfo            at;
    };
    
    template <typename TT>  struct ToBasicType;
    template <typename QQ> struct ToBasicType<QQ &> : ToBasicType<QQ> {};
    template <typename QQ> struct ToBasicType<const QQ &> : ToBasicType<QQ> {};
    template<> struct ToBasicType<Iterator *>   { enum { type = Type::tIterator }; };
    template<> struct ToBasicType<void *>       { enum { type = Type::tPointer }; };
    template<> struct ToBasicType<char *>       { enum { type = Type::tString }; };
    template<> struct ToBasicType<const char *> { enum { type = Type::tString }; };
    template<> struct ToBasicType<string>       { enum { type = Type::tString }; };
    template<> struct ToBasicType<bool>         { enum { type = Type::tBool }; };
    template<> struct ToBasicType<int64_t>      { enum { type = Type::tInt64 }; };
    template<> struct ToBasicType<uint64_t>     { enum { type = Type::tUInt64 }; };
    template<> struct ToBasicType<int32_t>      { enum { type = Type::tInt }; };
    template<> struct ToBasicType<uint32_t>     { enum { type = Type::tUInt }; };
    template<> struct ToBasicType<float>        { enum { type = Type::tFloat }; };
    template<> struct ToBasicType<void>         { enum { type = Type::tVoid }; };
    template<> struct ToBasicType<float2>       { enum { type = Type::tFloat2 }; };
    template<> struct ToBasicType<float3>       { enum { type = Type::tFloat3 }; };
    template<> struct ToBasicType<float4>       { enum { type = Type::tFloat4 }; };
    template<> struct ToBasicType<int2>         { enum { type = Type::tInt2 }; };
    template<> struct ToBasicType<int3>         { enum { type = Type::tInt3 }; };
    template<> struct ToBasicType<int4>         { enum { type = Type::tInt4 }; };
    template<> struct ToBasicType<uint2>        { enum { type = Type::tUInt2 }; };
    template<> struct ToBasicType<uint3>        { enum { type = Type::tUInt3 }; };
    template<> struct ToBasicType<uint4>        { enum { type = Type::tUInt4 }; };
    template<> struct ToBasicType<Array *>      { enum { type = Type::tArray }; };
    template<> struct ToBasicType<Table *>      { enum { type = Type::tTable }; };

    template <typename TT>
    struct typeFactory {
        static TypeDeclPtr make(const ModuleLibrary &) {
            auto t = make_shared<TypeDecl>();
            t->baseType = Type(ToBasicType<TT>::type);
            t->ref = is_reference<TT>::value;
            return t;
        }
    };
    
    template <typename TT>
    __forceinline TypeDeclPtr makeType(const ModuleLibrary & ctx) {
        return typeFactory<TT>::make(ctx);
    }
    
    class Structure : public enable_shared_from_this<Structure> {
    public:
        struct FieldDeclaration {
            string      name;
            TypeDeclPtr type;
            LineInfo    at;
            int         offset = 0;
        };
    public:
        Structure ( const string & n ) : name(n) {}
        const FieldDeclaration * findField ( const string & name ) const;
        friend ostream& operator<< (ostream& stream, const Structure & structure);
        int getSizeOf() const;
        bool canCopy() const;
        bool isPod() const;
    public:
        string                      name;
        vector<FieldDeclaration>    fields;
        LineInfo                    at;
    };
    
    struct Variable {
        friend ostream& operator<< (ostream& stream, const Variable & var);
        VariablePtr clone() const;
        string          name;
        TypeDeclPtr     type;
        ExpressionPtr   init;
        LineInfo        at;
        int             index = -1;
        uint32_t        stackTop = 0;
    };
    
    struct Expression : enable_shared_from_this<Expression> {
        struct InferTypeContext {
            ProgramPtr              program;
            FunctionPtr             func;
            vector<VariablePtr>     local;
            vector<ExpressionPtr>   loop;
            uint32_t                stackTop = 0;
            void error ( const string & err, const LineInfo & at );
        };
        Expression() = default;
        Expression(const LineInfo & a) : at(a) {}
        virtual ~Expression() {}
        friend ostream& operator<< (ostream& stream, const Expression & func);
        virtual void log(ostream& stream, int depth) const = 0;
        virtual void inferType(InferTypeContext & context) = 0;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const;
        void logType(ostream& stream) const;
        static ExpressionPtr autoDereference ( const ExpressionPtr & expr );
        virtual SimNode * simulate (Context & context) const = 0;
        virtual bool isSequence() const { return false; }
        virtual bool isStringConstant() const { return false; }
        LineInfo    at;
        TypeDeclPtr type;
    };
    
    template <typename ExprType>
    __forceinline shared_ptr<ExprType> clonePtr ( const ExpressionPtr & expr ) {
        return expr ? static_pointer_cast<ExprType>(expr) : make_shared<ExprType>();
    }
    
    struct ExprRef2Value : Expression {
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   subexpr;
    };
    
    struct ExprPtr2Ref : Expression {
        ExprPtr2Ref () = default;
        ExprPtr2Ref ( const LineInfo & a, ExpressionPtr s ) : Expression(a), subexpr(s) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   subexpr;
    };
    
    struct ExprNew : Expression {
        ExprNew() = default;
        ExprNew ( const LineInfo & a, TypeDeclPtr t ) : Expression(a), typeexpr(t) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        TypeDeclPtr     typeexpr;
    };
    
    struct ExprAt : Expression {
        ExprAt() = default;
        ExprAt ( const LineInfo & a, ExpressionPtr s, ExpressionPtr i ) : Expression(a), subexpr(s), index(i) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   subexpr, index;
    };

    struct ExprBlock : Expression {
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        vector<ExpressionPtr>   list;
    };
    
    struct ExprVar : Expression {
        ExprVar () = default;
        ExprVar ( const LineInfo & a, const string & n ) : Expression(a), name(n) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        string      name;
        VariablePtr variable;
        bool        local = false;
        bool        argument = false;
        int         argumentIndex = -1;
    };
    
    struct ExprField : Expression {
        ExprField () = default;
        ExprField ( const LineInfo & a, ExpressionPtr val, const string & n ) : Expression(a), value(val), name(n) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   value;
        string          name;
        const Structure::FieldDeclaration * field = nullptr;
    };
    
    struct ExprOp : Expression {
        ExprOp () = default;
        ExprOp ( const LineInfo & a, Operator o ) : Expression(a), op(o) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        Operator        op;
        FunctionPtr     func;   // always built-in function
    };
    
    // unary    !subexpr
    struct ExprOp1 : ExprOp {
        ExprOp1 () = default;
        ExprOp1 ( const LineInfo & a, Operator o, ExpressionPtr s ) : ExprOp(a,o), subexpr(s) {}
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   subexpr;
    };
    
    // binary   left < right
    struct ExprOp2 : ExprOp {
        ExprOp2 () = default;
        ExprOp2 ( const LineInfo & a, Operator o, ExpressionPtr l, ExpressionPtr r ) : ExprOp(a,o), left(l), right(r) {}
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   left, right;
    };
    
    // this copies one object to the other
    struct ExprCopy : ExprOp2 {
        ExprCopy () = default;
        ExprCopy ( const LineInfo & a, ExpressionPtr l, ExpressionPtr r ) : ExprOp2(a, Operator::none, l, r) {};
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    // this moves one object to the other
    struct ExprMove : ExprOp2 {
        ExprMove () = default;
        ExprMove ( const LineInfo & a, ExpressionPtr l, ExpressionPtr r ) : ExprOp2(a, Operator::none, l, r) {};
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    // this only exists during parsing, and can't be
    // and this is why it does not have CLONE
    struct ExprSequence : ExprOp2 {
        ExprSequence ( const LineInfo & a, ExpressionPtr l, ExpressionPtr r ) : ExprOp2(a, Operator::none, l, r) {}
        virtual bool isSequence() const override { return true; }
    };
    
    // trinary  subexpr ? left : right
    struct ExprOp3 : ExprOp {
        ExprOp3 () = default;
        ExprOp3 ( const LineInfo & a, Operator o, ExpressionPtr s, ExpressionPtr l, ExpressionPtr r )
            : ExprOp(a,o), subexpr(s), left(l), right(r) {}
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   subexpr, left, right;
    };
    
    struct ExprTryCatch : Expression {
        ExprTryCatch() = default;
        ExprTryCatch ( const LineInfo & a, ExpressionPtr t, ExpressionPtr c ) : Expression(a), try_block(t), catch_block(c) {}
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual SimNode * simulate (Context & context) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        ExpressionPtr try_block, catch_block;
    };
    
    struct ExprReturn : Expression {
        ExprReturn() = default;
        ExprReturn ( const LineInfo & a, ExpressionPtr s ) : Expression(a), subexpr(s) {}
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual SimNode * simulate (Context & context) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        ExpressionPtr subexpr;
    };
    
    struct ExprBreak : Expression {
        ExprBreak() = default;
        ExprBreak ( const LineInfo & a ) : Expression(a) {}
        virtual void inferType(InferTypeContext & context) override;
        virtual void log(ostream& stream, int depth) const override;
        virtual SimNode * simulate (Context & context) const override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
    };
    
    template <typename TT, typename ExprConstExt>
    struct ExprConst : Expression {
        ExprConst(TT val = TT()) : value(val) {}
        ExprConst(const LineInfo & a, TT val = TT()) : Expression(a), value(val) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr ) const override {
            auto cexpr = clonePtr<ExprConstExt>(expr);
            Expression::clone(cexpr);
            cexpr->value = value;
            return cexpr;
        }
        virtual void inferType(InferTypeContext & context) override {
            type = make_shared<TypeDecl>((Type)ToBasicType<TT>::type);
        }
        virtual SimNode * simulate (Context & context) const override {
            return context.makeNode<SimNode_ConstValue<TT>>(at,value);
        }
        virtual void log(ostream& stream, int depth) const override {
            stream << value;
        }
        TT getValue() const { return value; }
        TT  value;
    };
    
    struct ExprConstPtr : ExprConst<void *,ExprConstPtr> {
        ExprConstPtr(void * ptr = nullptr) : ExprConst(ptr) {}
        ExprConstPtr(const LineInfo & a, void * ptr = nullptr) : ExprConst(a,ptr) {}
        virtual void log(ostream& stream, int depth) const override {
            if ( value ) stream << hex << "*0x" << intptr_t(value) << dec; else stream << "nil";
        }
    };

    struct ExprConstInt : ExprConst<int32_t,ExprConstInt> {
        ExprConstInt(int32_t i = 0)  : ExprConst(i) {}
        ExprConstInt(const LineInfo & a, int32_t i = 0)  : ExprConst(a, i) {}
    };
    
    struct ExprConstUInt : ExprConst<uint32_t,ExprConstUInt> {
        ExprConstUInt(uint32_t i = 0) : ExprConst(i) {}
        ExprConstUInt(const LineInfo & a, uint32_t i = 0) : ExprConst(a,i) {}
        virtual void log(ostream& stream, int depth) const override {  stream << "0x" << hex << value << dec; }
    };
    
    struct ExprConstBool : ExprConst<bool,ExprConstBool> {
        ExprConstBool(bool i = false) : ExprConst(i) {}
        ExprConstBool(const LineInfo & a, bool i = false) : ExprConst(a,i) {}
        virtual void log(ostream& stream, int depth) const override {  stream << (value ? "true" : "false"); }
    };

    struct ExprConstFloat : ExprConst<float,ExprConstFloat> {
        ExprConstFloat(float i = 0.0f) : ExprConst(i) {}
        ExprConstFloat(const LineInfo & a, float i = 0.0f) : ExprConst(a,i) {}
        virtual void log(ostream& stream, int depth) const override { stream << to_string_ex(value); }
    };
    
    struct ExprConstString : ExprConst<string,ExprConstString> {
        ExprConstString(const string & str = string()) : ExprConst(unescapeString(str)) {}
        ExprConstString(const LineInfo & a, const string & str = string()) : ExprConst(a,unescapeString(str)) {}
        virtual SimNode * simulate (Context & context) const override {
            char * str = context.allocateName(value);
            return context.makeNode<SimNode_ConstValue<char *>>(at,str);
        }
        virtual void log(ostream& stream, int depth) const override {  stream << "\"" << escapeString(value) << "\""; }
        virtual bool isStringConstant() const override { return true; }
    };
    
    struct ExprLet : Expression {
        Variable * find ( const string & name ) const;
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        static SimNode * simulateInit(Context & context, const VariablePtr & var, bool local);
        vector<VariablePtr>     variables;
        ExpressionPtr           subexpr;
    };
    
    // for a,b in foo,bar where a>b ...
    struct ExprFor : Expression {
        ExprFor () = default;
        ExprFor ( const LineInfo & a ) : Expression(a) {}
        Variable * findIterator ( const string & name ) const;
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        vector<string>          iterators;
        vector<VariablePtr>     iteratorVariables;
        vector<ExpressionPtr>   sources;
        ExpressionPtr           subexpr;
        ExpressionPtr           filter;
        uint32_t                fixedSize = 0;
        bool                    dynamicArrays = false;
        bool                    fixedArrays = false;
        bool                    nativeIterators = false;
    };
    
    struct ExprLooksLikeCall : Expression {
        ExprLooksLikeCall () = default;
        ExprLooksLikeCall ( const LineInfo & a, const string & n ) : Expression(a), name(n) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        void autoDereference();
        virtual SimNode * simulate (Context & context) const override { return nullptr; }
        string describe() const;
        string                  name;
        vector<ExpressionPtr>   arguments;
    };
    typedef function<ExprLooksLikeCall * (const LineInfo & info)> ExprCallFactory;
    
    struct ExprAssert : ExprLooksLikeCall {
        ExprAssert () = default;
        ExprAssert ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a,name) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    struct ExprDebug : ExprLooksLikeCall {
        ExprDebug () = default;
        ExprDebug ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, name) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    struct ExprHash : ExprLooksLikeCall {
        ExprHash () = default;
        ExprHash ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, name) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    struct ExprArrayPush : ExprLooksLikeCall {
        ExprArrayPush() = default;
        ExprArrayPush ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, name) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    template <typename SimNodeT, bool first>
    struct ExprTableKeysOrValues : ExprLooksLikeCall {
        ExprTableKeysOrValues() = default;
        ExprTableKeysOrValues ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, name) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override {
            auto cexpr = clonePtr<ExprTableKeysOrValues<SimNodeT,first>>(expr);
            return cexpr;
        }
        virtual void inferType(InferTypeContext & context) override {
            if ( arguments.size()!=1 ) {
                context.error("expecting " + name + "(table)", at);
            }
            ExprLooksLikeCall::inferType(context);
            auto tableType = arguments[0]->type;
            if ( !tableType ) return;
            if ( !tableType->isGoodTableType() ) {
                context.error("first argument must be fully qualified table", at);
                return;
            }
            auto iterType = first ? tableType->firstType : tableType->secondType;
            type = make_shared<TypeDecl>(Type::tIterator);
            type->firstType = make_shared<TypeDecl>(*iterType);
            type->firstType->ref = true;
        }
        virtual SimNode * simulate (Context & context) const override {
            auto subexpr = arguments[0]->simulate(context);
            auto tableType = arguments[0]->type;
            auto iterType = first ? tableType->firstType : tableType->secondType;
            auto stride = iterType->getSizeOf();
            return context.makeNode<SimNodeT>(at,subexpr,stride);
        }
    };
    
    template <typename SimNodeT>
    struct ExprArrayCallWithSizeOrIndex : ExprLooksLikeCall {
        ExprArrayCallWithSizeOrIndex() = default;
        ExprArrayCallWithSizeOrIndex ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, name) {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override {
            auto cexpr = clonePtr<ExprArrayCallWithSizeOrIndex<SimNodeT>>(expr);
            return cexpr;
        }
        virtual void inferType(InferTypeContext & context) override {
            if ( arguments.size()!=2 ) {
                context.error("expecting array and size or index", at);
            }
            ExprLooksLikeCall::inferType(context);
            auto arrayType = arguments[0]->type;
            auto valueType = arguments[1]->type;
            if ( !arrayType || !valueType ) return;
            if ( !arrayType->isGoodArrayType() ) {
                context.error("first argument must be fully qualified array", at);
                return;
            }
            if ( !valueType->isIndex() )
                context.error("size must be int or uint", at);
            arguments[1] = Expression::autoDereference(arguments[1]);
            type = make_shared<TypeDecl>(Type::tVoid);
        }
        virtual SimNode * simulate (Context & context) const override {
            auto arr = arguments[0]->simulate(context);
            auto newSize = arguments[1]->simulate(context);
            auto size = arguments[0]->type->firstType->getSizeOf();
            return context.makeNode<SimNodeT>(at,arr,newSize,size);
        }
    };
    
    struct ExprErase : ExprLooksLikeCall {
        ExprErase() = default;
        ExprErase ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, "erase") {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    struct ExprFind : ExprLooksLikeCall {
        ExprFind() = default;
        ExprFind ( const LineInfo & a, const string & name ) : ExprLooksLikeCall(a, "find") {}
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual SimNode * simulate (Context & context) const override;
    };
    
    struct ExprSizeOf : Expression {
        ExprSizeOf () = default;
        ExprSizeOf ( const LineInfo & a, ExpressionPtr s ) : Expression(a), subexpr(s) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   subexpr;
        TypeDeclPtr     typeexpr;
    };

    struct ExprCall : ExprLooksLikeCall {
        ExprCall () = default;
        ExprCall ( const LineInfo & a, const string & n ) : ExprLooksLikeCall(a,n) { }
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        FunctionPtr             func;
        uint32_t                stackTop = 0;
    };
    
    struct ExprIfThenElse : Expression {
        ExprIfThenElse () = default;
        ExprIfThenElse ( const LineInfo & a, ExpressionPtr c, ExpressionPtr ift, ExpressionPtr iff )
            : Expression(a), cond(c), if_true(ift), if_false(iff) {}
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   cond, if_true, if_false;
    };
    
    struct ExprWhile : Expression {
        virtual void log(ostream& stream, int depth) const override;
        virtual void inferType(InferTypeContext & context) override;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const override;
        virtual SimNode * simulate (Context & context) const override;
        ExpressionPtr   cond, body;
    };
    
    class Function {
    public:
        virtual ~Function() {}
        friend ostream& operator<< (ostream& stream, const Function & func);
        string getMangledName() const;
        VariablePtr findArgument(const string & name);
        SimNode * simulate (Context & context) const;
        virtual SimNode * makeSimNode ( Context & context ) { return context.makeNode<SimNode_Call>(at); }
    public:
        string              name;
        vector<VariablePtr> arguments;
        TypeDeclPtr         result;
        ExpressionPtr       body;
        bool                builtIn = false;
        int                 index = -1;
        uint32_t            totalStackSize = 0;
        LineInfo            at;
    };
    
    class BuiltInFunction : public Function {
    public:
        BuiltInFunction ( const string & fn );
    };
    
    struct Error {
        Error ( const string & w, LineInfo a ) : what(w), at(a) {}
        __forceinline bool operator < ( const Error & err ) const { return at==err.at ? what < err.what : at<err.at; };
        string      what;
        LineInfo    at;
    };
    
    class Module : public enable_shared_from_this<Module> {
    public:
        bool addVariable ( const VariablePtr & var );
        bool addStructure ( const StructurePtr & st );
        bool addFunction ( const FunctionPtr & fn );
        VariablePtr findVariable ( const string & name ) const;
        FunctionPtr findFunction ( const string & mangledName ) const;
        StructurePtr findStructure ( const string & name ) const;
    public:
        map<string, StructurePtr>           structures;
        map<string, VariablePtr>            globals;
        map<string, FunctionPtr>            functions;                  // mangled name 2 function name
        map<string, vector<FunctionPtr>>    functionsByName;    // all functions of the same name
    };
    
    class Module_BuiltIn : public Module {
        friend class Program;
    public:
        Module_BuiltIn();
    protected:
        template <typename TT>
        __forceinline void addCall ( const string & fnName ) {
            callThis[fnName] = [fnName](const LineInfo & at) { return new TT(at, fnName); };
        }
        void addRuntime(ModuleLibrary & lib);
        map<string,ExprCallFactory> callThis;
    };
    
    class ModuleLibrary {
    public:
        void addModule ( const ModulePtr & module ) { modules.push_back(module); }
        void foreach ( function<bool (const ModulePtr & module)> && func ) const;
        VariablePtr findVariable ( const string & name ) const;
        FunctionPtr findFunction ( const string & mangledName ) const;
        StructurePtr findStructure ( const string & name ) const;
        TypeDeclPtr makeStructureType ( const string & name ) const;
    protected:
        vector<ModulePtr>   modules;
    };
    
    class Program : public enable_shared_from_this<Program> {
    public:
        Program();
        friend ostream& operator<< (ostream& stream, const Program & program);
        VariablePtr findVariable ( const string & name ) const;
        FunctionPtr findFunction ( const string & mangledName ) const;
        StructurePtr findStructure ( const string & name ) const;
        bool addVariable ( const VariablePtr & var );
        bool addStructure ( const StructurePtr & st );
        bool addFunction ( const FunctionPtr & fn );
        void addModule ( const ModulePtr & pm );
        void inferTypes();
        vector<FunctionPtr> findMatchingFunctions ( const string & name, const vector<TypeDeclPtr> & types ) const;
        void simulate ( Context & context );
        void error ( const string & str, const LineInfo & at );
        bool failed() const { return failToCompile; }
        ExprLooksLikeCall * makeCall ( const LineInfo & at, const string & name );
    public:
        void makeTypeInfo ( TypeInfo * info, Context & context, const TypeDeclPtr & type );
        VarInfo * makeVariableDebugInfo ( Context & context, const Variable & var );
        StructInfo * makeStructureDebugInfo ( Context & context, const Structure & st );
        FuncInfo * makeFunctionDebugInfo ( Context & context, const Function & fn );
    public:
        map<string,StructInfo *>    sdebug;
    public:
        static ModulePtr            builtInModule;
        ModulePtr                   thisModule;
        ModuleLibrary               library;
        int                         totalFunctions = 0;
        vector<Error>               errors;
        bool                        failToCompile = false;
    };
         
    ProgramPtr parseDaScript ( const char * script, function<void (const ProgramPtr & prg)> && defineContext );
}

