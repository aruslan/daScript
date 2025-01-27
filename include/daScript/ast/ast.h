#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/misc/string_writer.h"
#include "daScript/misc/vectypes.h"
#include "daScript/misc/arraytype.h"
#include "daScript/misc/rangetype.h"
#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/ast/compilation_errors.h"
#include "daScript/ast/ast_typedecl.h"
#include "daScript/simulate/aot_library.h"

namespace das
{
    class Function;
    typedef shared_ptr<Function> FunctionPtr;

    struct Variable;
    typedef shared_ptr<Variable> VariablePtr;

    class Program;
    typedef shared_ptr<Program> ProgramPtr;

    struct FunctionAnnotation;
    typedef shared_ptr<FunctionAnnotation> FunctionAnnotationPtr;

    struct Expression;
    typedef shared_ptr<Expression> ExpressionPtr;

    //      [annotation (value,value,...,value)]
    //  or  [annotation (key=value,key,value,...,key=value)]
    struct AnnotationArgument {
        Type    type;       // only tInt, tFloat, tBool, and tString are allowed
        string  name;
        string  sValue;
        union {
            bool    bValue;
            int     iValue;
            float   fValue;
        };
        AnnotationArgument () : type(Type::tVoid), iValue(0) {}
        //explicit copy is required to avoid copying union as float and cause FPE
        AnnotationArgument (const AnnotationArgument&a)
            : type(a.type), name(a.name), sValue(a.sValue), iValue(a.iValue) {}
        AnnotationArgument ( const string & n, const string & s )
            : type(Type::tString), name(n), sValue(s), iValue(0) {}
        AnnotationArgument ( const string & n, bool  b )
            : type(Type::tBool), name(n), bValue(b) {}
        AnnotationArgument ( const string & n, int   i )
            : type(Type::tInt), name(n), iValue(i) {}
        AnnotationArgument ( const string & n, float f )
            : type(Type::tFloat), name(n), fValue(f) {}
    };

    typedef vector<AnnotationArgument> AnnotationArguments;

    struct AnnotationArgumentList : AnnotationArguments {
        const AnnotationArgument * find ( const string & name, Type type ) const;
        bool getOption(const string & name, bool def = false) const;
    };

    struct Annotation : BasicAnnotation, enable_shared_from_this<Annotation> {
        Annotation ( const string & n, const string & cpn = "" ) : BasicAnnotation(n,cpn) {}
        virtual ~Annotation() {}
        virtual void seal( Module * m ) { module = m; }
        virtual bool rtti_isHandledTypeAnnotation() const { return false; }
        virtual bool rtti_isStructureAnnotation() const { return false; }
        virtual bool rtti_isStructureTypeAnnotation() const { return false; }
        virtual bool rtti_isFunctionAnnotation() const { return false; }
        string describe() const { return name; }
        string getMangledName() const;
        Module *    module = nullptr;
    };

    struct AnnotationDeclaration : enable_shared_from_this<AnnotationDeclaration> {
        AnnotationPtr           annotation;
        AnnotationArgumentList  arguments;
        string getMangledName() const;
    };
    typedef shared_ptr<AnnotationDeclaration> AnnotationDeclarationPtr;

    typedef vector<AnnotationDeclarationPtr> AnnotationList;

    class Enumeration : public enable_shared_from_this<Enumeration> {
    public:
        Enumeration() = default;
        Enumeration( const string & na ) : name(na) {}
        bool add ( const string & f );
        bool add ( const string & f, int v );
        string describe() const { return name; }
        string getMangledName() const;
        int find ( const string & na, int def ) const;
        string find ( int va, const string & def ) const;
        pair<int,bool> find ( const string & f ) const;
    public:
        string          name;
        LineInfo        at;
        map<string,int> list;
        map<int,string> listI;
        int             lastOne = 0;
        Module *        module = nullptr;
    };

    class Structure : public enable_shared_from_this<Structure> {
    public:
        struct FieldDeclaration {
            string                  name;
            TypeDeclPtr             type;
            ExpressionPtr           init;
            AnnotationArgumentList  annotation;
            bool                    moveSemantic;
            LineInfo                at;
            int                     offset = 0;
            bool                    parentType = false;
            FieldDeclaration() = default;
            FieldDeclaration(const string & n, const TypeDeclPtr & t,  const ExpressionPtr & i,
                             const AnnotationArgumentList & alist, bool ms, const LineInfo & a )
                : name(n), type(t), init(i), annotation(alist), moveSemantic(ms), at(a) {}
        };
    public:
        Structure ( const string & n ) : name(n) {}
        StructurePtr clone() const;
        bool isCompatibleCast ( const Structure & castS ) const;
        const FieldDeclaration * findField ( const string & name ) const;
        int getSizeOf() const;
        int getAlignOf() const;
        bool canCopy() const;
        bool canClone() const;
        bool canAot() const;
        bool canAot( set<Structure *> & recAot ) const;
        bool isPod() const;
        bool isRawPod() const;
        bool isLocal() const;
        string describe() const { return name; }
        string getMangledName() const;
        bool hasAnyInitializers() const;
    public:
        string                      name;
        vector<FieldDeclaration>    fields;
        LineInfo                    at;
        Module *                    module = nullptr;
        bool                        genCtor = false;
        Structure *                 parent = nullptr;
        AnnotationList              annotations;
    };

    struct Variable : public enable_shared_from_this<Variable> {
        VariablePtr clone() const;
        string getMangledName() const;
        string          name;
        TypeDeclPtr     type;
        ExpressionPtr   init;
        ExpressionPtr   source;     // if its interator variable, this is where the source is
        LineInfo        at;
        int             index = -1;
        uint32_t        stackTop = 0;
        Module *        module = nullptr;
        union {
            struct {
                bool    move_to_init : 1;
                bool    used : 1;
                bool    aliasCMRES : 1;
            };
            uint32_t flags = 0;
        };
        union {
            struct {
                bool    access_extern : 1;
                bool    access_get : 1;
                bool    access_ref : 1;
                bool    access_init : 1;
                bool    access_pass : 1;
            };
            uint32_t access_flags = 0;
        };
    };

    struct ExprBlock;
    struct ExprCallFunc;

    struct FunctionAnnotation : Annotation {
        FunctionAnnotation ( const string & n ) : Annotation(n) {}
        virtual bool rtti_isFunctionAnnotation() const override { return true; }
        virtual bool apply ( const FunctionPtr & func, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;
        virtual bool finalize ( const FunctionPtr & func, ModuleGroup & libGroup,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & err ) = 0;
        virtual bool apply ( ExprBlock * block, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;
        virtual bool finalize ( ExprBlock * block, ModuleGroup & libGroup,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & err ) = 0;
        virtual string aotName ( ExprCallFunc * call );
        virtual void aotPrefix ( TextWriter &, ExprCallFunc * ) { }
        virtual bool isGeneric() const { return false; }
    };

    struct TypeAnnotation : Annotation {
        TypeAnnotation ( const string & n, const string & cpn = "" ) : Annotation(n,cpn) {}
        virtual TypeAnnotationPtr clone ( const TypeAnnotationPtr & p = nullptr ) const {
            DAS_ASSERTF(p, "can only clone real type %s", name.c_str());
            p->name = name;
            p->cppName = cppName;
            return p;
        }
        virtual bool canAot(set<Structure *> &) const { return true; }
        virtual bool canMove() const { return false; }
        virtual bool canCopy() const { return false; }
        virtual bool canClone() const { return false; }
        virtual bool isPod() const { return false; }
        virtual bool isRawPod() const { return false; }
        virtual bool isRefType() const { return false; }
        virtual bool isLocal() const { return false; }
        virtual bool canNew() const { return false; }
        virtual bool canDelete() const { return false; }
        virtual bool canDeletePtr() const { return false; }
        virtual bool isIndexable ( const TypeDeclPtr & ) const { return false; }
        virtual bool isIterable ( ) const { return false; }
        virtual size_t getSizeOf() const { return sizeof(void *); }
        virtual size_t getAlignOf() const { return 1; }
        virtual TypeDeclPtr makeFieldType ( const string & ) const { return nullptr; }
        virtual TypeDeclPtr makeSafeFieldType ( const string & ) const { return nullptr; }
        virtual TypeDeclPtr makeIndexType ( const ExpressionPtr & /*src*/, const ExpressionPtr & /*idx*/ ) const { return nullptr; }
        virtual TypeDeclPtr makeIteratorType ( const ExpressionPtr & /*src*/ ) const { return nullptr; }
        // aot
        virtual void aotPreVisitGetField ( TextWriter &, const string & ) { }
        virtual void aotPreVisitGetFieldPtr ( TextWriter &, const string & ) { }
        virtual void aotVisitGetField ( TextWriter & ss, const string & fieldName ) { ss << "." << fieldName; }
        virtual void aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName ) { ss << "->" << fieldName; }
        // simulate
        virtual SimNode * simulateDelete ( Context &, const LineInfo &, SimNode *, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateDeletePtr ( Context &, const LineInfo &, SimNode *, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateCopy ( Context &, const LineInfo &, SimNode *, SimNode * ) const { return nullptr; }
        virtual SimNode * simulateClone ( Context &, const LineInfo &, SimNode *, SimNode * ) const { return nullptr; }
        virtual SimNode * simulateRef2Value ( Context &, const LineInfo &, SimNode * ) const { return nullptr; }
        virtual SimNode * simulateGetField ( const string &, Context &, const LineInfo &, const ExpressionPtr & ) const { return nullptr; }
        virtual SimNode * simulateGetFieldR2V ( const string &, Context &, const LineInfo &, const ExpressionPtr & ) const { return nullptr; }
        virtual SimNode * simulateSafeGetField ( const string &, Context &, const LineInfo &, const ExpressionPtr & ) const { return nullptr; }
        virtual SimNode * simulateSafeGetFieldPtr ( const string &, Context &, const LineInfo &, const ExpressionPtr & ) const { return nullptr; }
        virtual SimNode * simulateGetNew ( Context &, const LineInfo & ) const { return nullptr; }
        virtual SimNode * simulateGetAt ( Context &, const LineInfo &, const TypeDeclPtr &,
                                         const ExpressionPtr &, const ExpressionPtr &, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateGetAtR2V ( Context &, const LineInfo &, const TypeDeclPtr &,
                                            const ExpressionPtr &, const ExpressionPtr &, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateGetIterator ( Context &, const LineInfo &, const ExpressionPtr & ) const { return nullptr; }
        virtual void walk ( DataWalker &, void * ) { }
    };

    struct StructureAnnotation : Annotation {
        StructureAnnotation ( const string & n ) : Annotation(n) {}
        virtual bool rtti_isStructureAnnotation() const override { return true; }
        virtual bool touch ( const StructurePtr & st, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;    // this one happens before infer. u can change structure here
        virtual bool look (const StructurePtr & st, ModuleGroup & libGroup,
            const AnnotationArgumentList & args, string & err ) = 0;                    // this one happens after infer. structure is read-only, or at-least infer-safe
    };

    // annotated structure
    //  needs to override
    //      create,clone, simulateGetField, simulateGetFieldR2V, SafeGetField, and SafeGetFieldPtr
    struct StructureTypeAnnotation : TypeAnnotation {
        StructureTypeAnnotation ( const string & n ) : TypeAnnotation(n) {}
        virtual bool rtti_isStructureTypeAnnotation() const override { return true; }
        virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
        virtual bool canCopy() const override { return false; }
        virtual bool isPod() const override { return false; }
        virtual bool isRawPod() const override { return false; }
        virtual bool isRefType() const override { return false; }
        virtual bool create ( const shared_ptr<Structure> & st, const AnnotationArgumentList & /*args*/, string & /*err*/ ) {
            structureType = st;
            return true;
        }
        virtual TypeAnnotationPtr clone ( const TypeAnnotationPtr & p = nullptr ) const override {
            shared_ptr<StructureTypeAnnotation> cp =  p ? static_pointer_cast<StructureTypeAnnotation>(p) : make_shared<StructureTypeAnnotation>(name);
            cp->structureType = structureType;
            return TypeAnnotation::clone(cp);
        }
        shared_ptr<Structure>   structureType;
    };

    struct Expression : enable_shared_from_this<Expression> {
        Expression() = default;
        Expression(const LineInfo & a) : at(a) {}
        virtual ~Expression() {}
        friend TextWriter& operator<< (TextWriter& stream, const Expression & func);
        virtual ExpressionPtr visit(Visitor & vis) = 0;
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const;
        static ExpressionPtr autoDereference ( const ExpressionPtr & expr );
        virtual SimNode * simulate (Context & context) const = 0;
        virtual SimNode * trySimulate (Context & context, uint32_t extraOffset, Type r2vType ) const;
        virtual bool rtti_isSequence() const { return false; }
        virtual bool rtti_isConstant() const { return false; }
        virtual bool rtti_isStringConstant() const { return false; }
        virtual bool rtti_isCall() const { return false; }
        virtual bool rtti_isInvoke() const { return false; }
        virtual bool rtti_isCallLikeExpr() const { return false; }
        virtual bool rtti_isLet() const { return false; }
        virtual bool rtti_isReturn() const { return false; }
        virtual bool rtti_isBreak() const { return false; }
        virtual bool rtti_isContinue() const { return false; }
        virtual bool rtti_isBlock() const { return false; }
        virtual bool rtti_isWith() const { return false; }
        virtual bool rtti_isVar() const { return false; }
        virtual bool rtti_isR2V() const { return false; }
        virtual bool rtti_isRef2Ptr() const { return false; }
        virtual bool rtti_isPtr2Ref() const { return false; }
        virtual bool rtti_isCast() const { return false; }
        virtual bool rtti_isField() const { return false; }
        virtual bool rtti_isSwizzle() const { return false; }
        virtual bool rtti_isSafeField() const { return false; }
        virtual bool rtti_isAt() const { return false; }
        virtual bool rtti_isOp1() const { return false; }
        virtual bool rtti_isOp2() const { return false; }
        virtual bool rtti_isOp3() const { return false; }
        virtual bool rtti_isNullCoalescing() const { return false; }
        virtual bool rtti_isValues() const { return false; }
        virtual bool rtti_isMakeBlock() const { return false; }
        virtual bool rtti_isMakeLocal() const { return false; }
        virtual bool rtti_isMakeTuple() const { return false; }
        virtual bool rtti_isIfThenElse() const { return false; }
        virtual bool rtti_isFor() const { return false; }
        virtual bool rtti_isWhile() const { return false; }
        virtual bool rtti_isAddr() const { return false; }
        virtual Expression * tail() { return this; }
        virtual uint32_t getEvalFlags() const { return 0; }
        LineInfo    at;
        TypeDeclPtr type;
        union {
            struct {
                bool    constexpression : 1;
                bool    noSideEffects : 1;
                bool    noNativeSideEffects : 1;
            };
            uint32_t    flags = 0;
        };
        union {
            struct {
                bool    topLevel :  1;
                bool    argLevel : 1;
                bool    bottomLevel : 1;
            };
            uint32_t    printFlags = 0;
        };
    };

    struct ExprLooksLikeCall;
    typedef function<ExprLooksLikeCall * (const LineInfo & info)> ExprCallFactory;

    template <typename ExprType>
    inline shared_ptr<ExprType> clonePtr ( const ExpressionPtr & expr ) {
        return expr ? static_pointer_cast<ExprType>(expr) : make_shared<ExprType>();
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4324)
#endif
    struct ExprConst : Expression {
        ExprConst ( Type t ) : baseType(t) {}
        ExprConst ( const LineInfo & a, Type t ) : Expression(a), baseType(t) {}
        virtual SimNode * simulate (Context & context) const override;
        virtual bool rtti_isConstant() const override { return true; }
        Type    baseType;
        vec4f   value;
      };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    template <typename TT, typename ExprConstExt>
    struct ExprConstT : ExprConst {
        ExprConstT ( TT val, Type bt ) : ExprConst(bt) { value = cast<TT>::from(val); }
        ExprConstT ( const LineInfo & a, TT val, Type bt ) : ExprConst(a,bt) { value = v_zero(); *((TT *)&value) = val; }
        virtual ExpressionPtr clone( const ExpressionPtr & expr ) const override {
            auto cexpr = clonePtr<ExprConstExt>(expr);
            Expression::clone(cexpr);
            cexpr->value = value;
            return cexpr;
        }
        virtual ExpressionPtr visit(Visitor & vis) override;
        TT getValue() const { return cast<TT>::to(value); }
    };

    enum class SideEffects : uint32_t {
        none =              0
    ,   unsafe =            (1<<0)
    ,   userScenario =      (1<<1)
    ,   modifyExternal =    (1<<2)
    ,   modifyArgument =    (1<<3)
    ,   accessGlobal =      (1<<4)
    ,   invoke =            (1<<5)
    ,   inferedSideEffects = uint32_t(SideEffects::modifyArgument) | uint32_t(SideEffects::accessGlobal) | uint32_t(SideEffects::invoke)
    };

    class Function : public enable_shared_from_this<Function> {
    public:
        virtual ~Function() {}
        friend TextWriter& operator<< (TextWriter& stream, const Function & func);
        string getMangledName() const;
        VariablePtr findArgument(const string & name);
        SimNode * simulate (Context & context) const;
        virtual SimNode * makeSimNode ( Context & context );
        string describe() const;
        virtual FunctionPtr visit(Visitor & vis);
        FunctionPtr setSideEffects ( SideEffects seFlags );
        bool isGeneric() const;
        FunctionPtr clone() const;
        string getLocationExtra() const;
        virtual string getAotBasicName() const { return name; }
        string getAotName(ExprCallFunc * call) const;
        FunctionPtr setAotTemplate();
    public:
        AnnotationList      annotations;
        string              name;
        vector<VariablePtr> arguments;
        TypeDeclPtr         result;
        ExpressionPtr       body;
        int                 index = -1;
        uint32_t            totalStackSize = 0;
        LineInfo            at;
        Module *            module = nullptr;
        set<Function *>     useFunctions;
        set<Variable *>     useGlobalVariables;
        union {
            struct {
                bool    builtIn : 1;
                bool    policyBased : 1;
                bool    callBased : 1;
                bool    interopFn : 1;
                bool    hasReturn: 1;
                bool    copyOnReturn : 1;
                bool    moveOnReturn : 1;
                bool    exports : 1;
                bool    init : 1;
                bool    addr : 1;
                bool    used : 1;
                bool    fastCall : 1;
                bool    knownSideEffects : 1;
                bool    hasToRunAtCompileTime : 1;
                bool    unsafe : 1;
                bool    unsafeOperation : 1;
                bool    hasMakeBlock : 1;
                bool    aotNeedPrologue : 1;
                bool    noAot : 1;
                bool    aotHybrid : 1;
                bool    aotTemplate : 1;
            };
            uint32_t flags = 0;
        };
        uint32_t    sideEffectFlags = 0;
        struct InferHistory {
            LineInfo    at;
            Function *  func = nullptr;
            InferHistory() = default;
            InferHistory(const LineInfo & a, const FunctionPtr & p) : at(a), func(p.get()) {}
        };
        vector<InferHistory> inferStack;
        Function * fromGeneric = nullptr;
        uint64_t hash = 0;
    };

    uint64_t getFunctionHash ( Function * fun, SimNode * node );

    class BuiltInFunction : public Function {
    public:
        BuiltInFunction ( const string & fn, const string & fnCpp );
        virtual string getAotBasicName() const override {
            return cppName.empty() ? name : cppName;
        }
    public:
        string cppName;
    };

    struct Error {
        Error ( const string & w, LineInfo a, CompilationError ce ) : what(w), at(a), cerr(ce)  {}
        __forceinline bool operator < ( const Error & err ) const { return at==err.at ? what < err.what : at<err.at; };
        string              what;
        LineInfo            at;
        CompilationError    cerr;
    };

    enum class ModuleAotType {
        no_aot,
        hybrid,
        cpp
    };

    class Module {
    public:
        Module ( const string & n = "" );
        virtual ~Module();
        virtual void addPrerequisits ( ModuleLibrary & ) const {}
        virtual ModuleAotType aotRequire ( TextWriter & ) const { return ModuleAotType::no_aot; }
        bool addAlias ( const TypeDeclPtr & at, bool canFail = false );
        bool addVariable ( const VariablePtr & var, bool canFail = false );
        bool addStructure ( const StructurePtr & st, bool canFail = false );
        bool removeStructure ( const StructurePtr & st );
        bool addEnumeration ( const EnumerationPtr & st, bool canFail = false );
        bool addFunction ( const FunctionPtr & fn, bool canFail = false );
        bool addGeneric ( const FunctionPtr & fn, bool canFail = false );
        bool addAnnotation ( const AnnotationPtr & ptr, bool canFail = false );
        TypeDeclPtr findAlias ( const string & name ) const;
        VariablePtr findVariable ( const string & name ) const;
        FunctionPtr findFunction ( const string & mangledName ) const;
        StructurePtr findStructure ( const string & name ) const;
        AnnotationPtr findAnnotation ( const string & name ) const;
        EnumerationPtr findEnum ( const string & name ) const;
        ExprCallFactory * findCall ( const string & name ) const;
        bool compileBuiltinModule ( const string & name, unsigned char * str, unsigned int str_len );//will replace last symbol to 0
        static Module * require ( const string & name );
        static void Shutdown();
        static TypeAnnotation * resolveAnnotation ( TypeInfo * info );
        virtual uintptr_t rtti_getUserData() {return uintptr_t(0);}
        void verifyAotReady();
    public:
        template <typename TT, typename ...TARG>
        __forceinline void addCall ( const string & fnName, TARG ...args ) {
            if ( callThis.find(fnName)!=callThis.end() ) {
                DAS_FATAL_LOG("addCall(%s) failed. duplicate call\n", fnName.c_str());
                DAS_FATAL_ERROR;
            }
            callThis[fnName] = [fnName,args...](const LineInfo & at) {
                return new TT(at, fnName, args...);
            };
        }
    public:
        map<string, TypeDeclPtr>                aliasTypes;
        map<string, AnnotationPtr>              handleTypes;
        map<string, StructurePtr>               structures;
        vector<StructurePtr>                    structuresInOrder;
        map<string, EnumerationPtr>             enumerations;
        map<string, VariablePtr>                globals;
        map<string, FunctionPtr>                functions;          // mangled name 2 function name
        map<string, vector<FunctionPtr>>        functionsByName;    // all functions of the same name
        map<string, FunctionPtr>                generics;           // mangled name 2 generic name
        map<string, vector<FunctionPtr>>        genericsByName;     // all generics of the same name
        mutable map<string, ExprCallFactory>    callThis;
        map<uint32_t, uint64_t>                 annotationData;
        string  name;
        bool    builtIn = false;
    private:
        Module * next = nullptr;
        static Module * modules;
        unique_ptr<FileInfo>    ownFileInfo;
    };

    #define REGISTER_MODULE(ClassName) \
        das::Module * register_##ClassName () { \
            static ClassName * module_##ClassName = new ClassName(); \
            return module_##ClassName; \
        }

    #define REGISTER_MODULE_IN_NAMESPACE(ClassName,Namespace) \
        das::Module * register_##ClassName () { \
            static Namespace::ClassName * module_##ClassName = new Namespace::ClassName(); \
            return module_##ClassName; \
        }

    class ModuleLibrary {
        friend class Module;
        friend class Program;
    public:
        virtual ~ModuleLibrary() {};
        void addBuiltInModule ();
        void addModule ( Module * module );
        void foreach ( function<bool (Module * module)> && func, const string & name ) const;
        vector<TypeDeclPtr> findAlias ( const string & name ) const;
        vector<AnnotationPtr> findAnnotation ( const string & name ) const;
        vector<EnumerationPtr> findEnum ( const string & name ) const;
        vector<StructurePtr> findStructure ( const string & name ) const;
        Module * findModule ( const string & name ) const;
        TypeDeclPtr makeStructureType ( const string & name ) const;
        TypeDeclPtr makeHandleType ( const string & name ) const;
        TypeDeclPtr makeEnumType ( const string & name ) const;
    protected:
        vector<Module *>                modules;
    };

    struct ModuleGroupUserData {
        ModuleGroupUserData ( const string & n ) : name(n) {}
        virtual ~ModuleGroupUserData() {}
        string name;
    };
    typedef unique_ptr<ModuleGroupUserData> ModuleGroupUserDataPtr;

    class ModuleGroup : public ModuleLibrary {
    public:
        virtual ~ModuleGroup();
        ModuleGroupUserData * getUserData ( const string & dataName ) const;
        bool setUserData ( ModuleGroupUserData * data );
    protected:
        map<string,ModuleGroupUserDataPtr>  userData;
    };

    class DebugInfoHelper {
    public:
        DebugInfoHelper () { debugInfo = make_shared<DebugInfoAllocator>(); }
        DebugInfoHelper ( const shared_ptr<DebugInfoAllocator> & di ) : debugInfo(di) {}
    public:
        TypeInfo * makeTypeInfo ( TypeInfo * info, const TypeDeclPtr & type );
        VarInfo * makeVariableDebugInfo ( const Variable & var );
        VarInfo * makeVariableDebugInfo ( const Structure & st, const Structure::FieldDeclaration & var );
        StructInfo * makeStructureDebugInfo ( const Structure & st );
        FuncInfo * makeFunctionDebugInfo ( const Function & fn );
        EnumInfo * makeEnumDebugInfo ( const Enumeration & en );
    public:
        shared_ptr<DebugInfoAllocator>  debugInfo;
        bool                            rtti = false;
    protected:
        map<string,StructInfo *>        smn2s;
        map<string,TypeInfo *>          tmn2t;
        map<string,VarInfo *>           vmn2v;
        map<string,FuncInfo *>          fmn2f;
        map<string,EnumInfo *>          emn2e;
    };

    class Program : public enable_shared_from_this<Program> {
    public:
        Program();
        friend TextWriter& operator<< (TextWriter& stream, const Program & program);
        VariablePtr findVariable ( const string & name ) const;
        vector<TypeDeclPtr> findAlias ( const string & name ) const;
        vector<StructurePtr> findStructure ( const string & name ) const;
        vector<AnnotationPtr> findAnnotation ( const string & name ) const;
        vector<EnumerationPtr> findEnum ( const string & name ) const;
        bool addAlias ( const TypeDeclPtr & at );
        bool addVariable ( const VariablePtr & var );
        bool addStructure ( const StructurePtr & st );
        bool addEnumeration ( const EnumerationPtr & st );
        bool addStructureHandle ( const StructurePtr & st, const TypeAnnotationPtr & ann, const AnnotationArgumentList & arg );
        bool addFunction ( const FunctionPtr & fn );
        FunctionPtr findFunction(const string & mangledName) const;
        bool addGeneric ( const FunctionPtr & fn );
        bool addModule ( const string & name );
        void finalizeAnnotations();
        void inferTypes(TextWriter & logs);
        void lint();
        void checkSideEffects();
        bool optimizationRefFolding();
        bool optimizationConstFolding();
        bool optimizationBlockFolding();
        bool optimizationCondFolding();
        bool optimizationUnused(TextWriter & logs);
        void fusion ( Context & context, TextWriter & logs );
        void buildAccessFlags(TextWriter & logs);
        bool verifyAndFoldContracts();
        void optimize(TextWriter & logs);
        void markSymbolUse(bool builtInSym);
        void clearSymbolUse();
        void markOrRemoveUnusedSymbols(bool forceAll = false);
        void allocateStack(TextWriter & logs);
        string dotGraph();
        bool simulate ( Context & context, TextWriter & logs );
        void linkCppAot ( Context & context, AotLibrary & aotLib, TextWriter & logs );
        void error ( const string & str, const LineInfo & at, CompilationError cerr = CompilationError::unspecified );
        bool failed() const { return failToCompile; }
        static ExpressionPtr makeConst ( const LineInfo & at, const TypeDeclPtr & type, vec4f value );
        ExprLooksLikeCall * makeCall ( const LineInfo & at, const string & name );
        TypeDecl * makeTypeDeclaration ( const LineInfo & at, const string & name );
        StructurePtr visitStructure(Visitor & vis, Structure *);
        EnumerationPtr visitEnumeration(Visitor & vis, Enumeration *);
        void visit(Visitor & vis, bool visitGenerics = false);
        void setPrintFlags();
        void aotCpp ( Context & context, TextWriter & logs );
        void registerAotCpp ( TextWriter & logs, Context & context, bool headers = true );
        void buildMNLookup ( Context & context, TextWriter & logs );
        void buildADLookup ( Context & context, TextWriter & logs );
    public:
        template <typename TT>
        string describeCandidates ( const vector<TT> & result, bool needHeader = true ) const {
            if ( !result.size() ) return "";
            TextWriter ss;
            if ( needHeader ) ss << "candidates are:";
            for ( auto & fn : result ) {
                ss << "\n\t";
                if ( fn->module && !fn->module->name.empty() && !(fn->module->name=="$") )
                    ss << fn->module->name << "::";
                ss << fn->describe();
            }
            return ss.str();
        }
    public:
        unique_ptr<Module>          thisModule;
        ModuleLibrary               library;
        ModuleGroup *               thisModuleGroup;
        int                         totalFunctions = 0;
        int                         totalVariables = 0;
        vector<Error>               errors;
        bool                        failToCompile = false;
        uint32_t                    globalInitStackSize = 0;
    public:
        map<CompilationError,int>   expectErrors;
    public:
        AnnotationArgumentList      options;
    };

    // this one works for single module only
    ProgramPtr parseDaScript ( const string & fileName, const FileAccessPtr & access, TextWriter & logs, ModuleGroup & libGroup, bool exportAll = false );

    // this one collectes dependencies and compiles with modules
    ProgramPtr compileDaScript ( const string & fileName, const FileAccessPtr & access, TextWriter & logs, ModuleGroup & libGroup, bool exportAll = false );


    // note: this has sifnificant performance implications
    //      i.e. this is ok for the load time \ map time
    //      it is not ok to use for every call
    template <typename ReturnType, typename ...Args>
    inline bool verifyCall ( FuncInfo * info, const ModuleLibrary & lib ) {
        vector<TypeDeclPtr> args = { makeType<Args>(lib)... };
        if ( args.size() != info->count ) {
            return false;
        }
        DebugInfoHelper helper;
        for ( uint32_t index=0; index != info->count; ++ index ) {
            auto argType = info->fields[index];
            if ( argType->type==Type::anyArgument ) {
                continue;
            }
            auto passType = helper.makeTypeInfo(nullptr, args[index]);
            if ( !isValidArgumentType(argType, passType) ) {
                return false;
            }
        }
        // ok, now for the return type
        auto resType = helper.makeTypeInfo(nullptr, makeType<ReturnType>(lib));
        if ( !isValidArgumentType(resType, info->result) ) {
            return false;
        }
        return true;
    }
}


