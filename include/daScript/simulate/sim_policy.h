#pragma once

namespace  das {

    class Context;

    // POLICY BASED OPERATIONS

    template <typename TT>
    struct SimPolicy_CoreType {
        static __forceinline bool Equ     ( TT a, TT b, Context & ) { return a == b;  }
        static __forceinline bool NotEqu  ( TT a, TT b, Context & ) { return a != b;  }
    };

    template <typename TT>
    struct SimPolicy_GroupByAdd : SimPolicy_CoreType<TT> {
        static __forceinline TT Unp ( TT x, Context & ) { return x; }
        static __forceinline TT Unm ( TT x, Context & ) { return -x; }
        static __forceinline TT Add ( TT a, TT b, Context & ) { return a + b; }
        static __forceinline TT Sub ( TT a, TT b, Context & ) { return a - b; }
        static __forceinline void SetAdd  ( TT & a, TT b, Context & ) { a += b; }
        static __forceinline void SetSub  ( TT & a, TT b, Context & ) { a -= b; }
    };

    template <typename TT>
    struct SimPolicy_Ordered {
        // ordered
        static __forceinline bool LessEqu ( TT a, TT b, Context & ) { return a <= b;  }
        static __forceinline bool GtEqu   ( TT a, TT b, Context & ) { return a >= b;  }
        static __forceinline bool Less    ( TT a, TT b, Context & ) { return a < b;  }
        static __forceinline bool Gt      ( TT a, TT b, Context & ) { return a > b;  }
    };

    template <typename TT>
    struct SimPolicy_Type : SimPolicy_GroupByAdd<TT>, SimPolicy_Ordered<TT> {
        // numeric
        static __forceinline TT Inc ( TT & x, Context & ) { return ++x; }
        static __forceinline TT Dec ( TT & x, Context & ) { return --x; }
        static __forceinline TT IncPost ( TT & x, Context & ) { return x++; }
        static __forceinline TT DecPost ( TT & x, Context & ) { return x--; }
        static __forceinline TT Div ( TT a, TT b, Context & context ) {
            if ( b==0 ) context.throw_error("division by zero");
            return a / b;
        }
        static __forceinline TT Mul ( TT a, TT b, Context & ) { return a * b; }
        static __forceinline void SetDiv  ( TT & a, TT b, Context & context ) {
            if ( b==0 ) context.throw_error("division by zero");
            a /= b;
        }
        static __forceinline void SetMul  ( TT & a, TT b, Context & ) { a *= b; }
    };

    struct SimPolicy_Bool : SimPolicy_CoreType<bool> {
        static __forceinline bool BoolNot ( bool x, Context & ) { return !x; }
        static __forceinline bool BoolAnd ( bool a, bool b, Context & ) { return a && b; }
        static __forceinline bool BoolOr  ( bool a, bool b, Context & ) { return a || b; }
        static __forceinline bool BoolXor ( bool a, bool b, Context & ) { return a ^ b; }
        static __forceinline void SetBoolAnd  ( bool & a, bool b, Context & ) { a &= b; }
        static __forceinline void SetBoolOr   ( bool & a, bool b, Context & ) { a |= b; }
        static __forceinline void SetBoolXor  ( bool & a, bool b, Context & ) { a ^= b; }
    };

    template <typename TT>
    struct SimPolicy_Bin : SimPolicy_Type<TT> {
        enum { BITS = sizeof(TT)*8 };
        static __forceinline TT Mod ( TT a, TT b, Context & context ) {
            if ( b==0 ) context.throw_error("division by zero in modulo");
            return a % b;
        }
        static __forceinline TT BinNot ( TT x, Context & ) { return ~x; }
        static __forceinline TT BinAnd ( TT a, TT b, Context & ) { return a & b; }
        static __forceinline TT BinOr  ( TT a, TT b, Context & ) { return a | b; }
        static __forceinline TT BinXor ( TT a, TT b, Context & ) { return a ^ b; }
        static __forceinline TT BinShl ( TT a, TT b, Context & ) { return a << b; }
        static __forceinline TT BinShr ( TT a, TT b, Context & ) { return a >> b; }
        static __forceinline TT BinRotl ( TT a, TT b, Context & ) {
            b &= (BITS-1);
            return (a << b) | (a >> (BITS - b));
        }
        static __forceinline TT BinRotr ( TT a, TT b, Context & ) {
            b &= (BITS-1);
            return (a >> b) | (a << (BITS - b));
        }
        static __forceinline void SetBinAnd ( TT & a, TT b, Context & ) { a &= b; }
        static __forceinline void SetBinOr  ( TT & a, TT b, Context & ) { a |= b; }
        static __forceinline void SetBinXor ( TT & a, TT b, Context & ) { a ^= b; }
        static __forceinline void SetBinShl ( TT & a, TT b, Context & ) { a <<= b; }
        static __forceinline void SetBinShr ( TT & a, TT b, Context & ) { a >>= b; }
        static __forceinline void SetBinRotl ( TT & a, TT b, Context & ) {
            b &= (BITS-1);
            a = (a << b) | (a >> (BITS - b));
        }
        static __forceinline void SetBinRotr ( TT & a, TT b, Context & ) {
            b &= (BITS-1);
            a = (a >> b) | (a << (BITS - b));
        }
        static __forceinline void SetMod    ( TT & a, TT b, Context & context ) {
            if ( b==0 ) context.throw_error("division by zero in modulo");
            a %= b;
        }
    };

    template <typename TT>
    struct SimPolicy_MathTT {
        static __forceinline TT Min   ( TT a, TT b, Context & ) { return a < b ? a : b; }
        static __forceinline TT Max   ( TT a, TT b, Context & ) { return a > b ? a : b; }
        static __forceinline TT Sat   ( TT a, Context & )    { return a < 0 ? 0  : (a > 1 ? 1 : a);}
    };

    struct SimPolicy_Int : SimPolicy_Bin<int32_t>, SimPolicy_MathTT<int32_t> {};
    struct SimPolicy_UInt : SimPolicy_Bin<uint32_t>, SimPolicy_MathTT<uint32_t> {};
    struct SimPolicy_Int64 : SimPolicy_Bin<int64_t>, SimPolicy_MathTT<int64_t> {};
    struct SimPolicy_UInt64 : SimPolicy_Bin<uint64_t>, SimPolicy_MathTT<int64_t> {};

    struct SimPolicy_Float : SimPolicy_Type<float> {
        static __forceinline float Mod ( float a, float b, Context & context ) {
            if ( b==0.0f ) context.throw_error("division by zero in modulo");
            return fmod(a,b);
        }
        static __forceinline void SetMod ( float & a, float b, Context & context ) {
            if ( b==0.0f ) context.throw_error("division by zero in modulo");
            a = fmod(a,b);
        }
    };

    struct SimPolicy_Double : SimPolicy_Type<double>, SimPolicy_MathTT<double> {
        static __forceinline double Mod ( double a, double b, Context & context ) {
            if ( b==0.0 ) context.throw_error("division by zero in modulo");
            return fmod(a,b);
        }
        static __forceinline void SetMod ( double & a, double b, Context & context ) {
            if ( b==0.0 ) context.throw_error("division by zero in modulo");
            a = fmod(a,b);
        }
    };

    struct SimPolicy_Pointer : SimPolicy_CoreType<void *> {
    };

    struct SimPolicy_MathVecI {
        static __forceinline vec4f Min   ( vec4f a, vec4f b, Context & ) { return v_cast_vec4f(v_mini(v_cast_vec4i(a),v_cast_vec4i(b))); }
        static __forceinline vec4f Max   ( vec4f a, vec4f b, Context & ) { return v_cast_vec4f(v_maxi(v_cast_vec4i(a),v_cast_vec4i(b))); }
        static __forceinline vec4f Sat   ( vec4f a, Context & ) {
            return v_cast_vec4f(v_mini(v_maxi(v_cast_vec4i(a),v_cast_vec4i(v_zero())),v_splatsi(1)));
        }
    };

    struct SimPolicy_MathFloat {
        static __forceinline float Abs   ( float a, Context & )          { return v_extract_x(v_abs(v_splats(a))); }
        static __forceinline float Floor ( float a, Context & )          { return v_extract_x(v_floor(v_splats(a))); }
        static __forceinline float Ceil  ( float a, Context & )          { return v_extract_x(v_ceil(v_splats(a))); }
        static __forceinline float Sqrt  ( float a, Context & )          { return v_extract_x(v_sqrt_x(v_splats(a))); }
        static __forceinline float Min   ( float a, float b, Context & ) { return a < b ? a : b; }
        static __forceinline float Max   ( float a, float b, Context & ) { return a > b ? a : b; }
        static __forceinline float Sat   ( float a, Context & )          { return a < 0 ? 0 : (a > 1 ? 1 : a); }
        static __forceinline float Mad   ( float a, float b, float c, Context & ) { return a*b + c; }
        static __forceinline float Lerp  ( float a, float b, float t, Context & ) { return (b-a)*t +a; }
        static __forceinline float Clamp  ( float t, float a, float b, Context & ){ return t>a ? (t<b ? t : b) : a; }

        static __forceinline int Trunci ( float a, Context & )          { return v_extract_xi(v_cvt_vec4i(v_splats(a))); }
        static __forceinline int Roundi ( float a, Context & )          { return v_extract_xi(v_cvt_roundi(v_splats(a))); }
        static __forceinline int Floori ( float a, Context & )          { return v_extract_xi(v_cvt_floori(v_splats(a))); }
        static __forceinline int Ceili  ( float a, Context & )          { return v_extract_xi(v_cvt_ceili(v_splats(a))); }

        static __forceinline float Exp   ( float a, Context & )          { return v_extract_x(v_exp(v_splats(a))); }
        static __forceinline float Log   ( float a, Context & )          { return v_extract_x(v_log(v_splats(a))); }
        static __forceinline float Exp2  ( float a, Context & )          { return v_extract_x(v_exp2(v_splats(a))); }
        static __forceinline float Log2  ( float a, Context & )          { return v_extract_x(v_log2_est_p5(v_splats(a))); }
        static __forceinline float Pow   ( float a, float b, Context & ) { return v_extract_x(v_pow(v_splats(a), v_splats(b))); }
        static __forceinline float Rcp   ( float a, Context & )          { return v_extract_x(v_rcp_x(v_splats(a))); }
        static __forceinline float RcpEst( float a, Context & )          { return v_extract_x(v_rcp_est_x(v_splats(a))); }

        static __forceinline float Sin   ( float a, Context & )          { vec4f s,c; v_sincos4(v_splats(a), s, c);return v_extract_x(s); }
        static __forceinline float Cos   ( float a, Context & )          { vec4f s,c; v_sincos4(v_splats(a), s, c);return v_extract_x(c); }
        static __forceinline float Tan   ( float a, Context & )          { vec4f s,c; v_sincos4(v_splats(a), s, c);return v_extract_x(v_div_x(s,c)); }
        static __forceinline float ATan   ( float a, Context & )         { return v_extract_x(v_atan(v_splats(a))); }
        static __forceinline float ASin   ( float a, Context & )         { return v_extract_x(v_asin_x(v_splats(a))); }
        static __forceinline float ACos   ( float a, Context & )         { return v_extract_x(v_acos_x(v_splats(a))); }
        static __forceinline float ATan2 ( float a, float b, Context & ) { return v_extract_x(v_atan2(v_splats(a), v_splats(b))); }
        static __forceinline float ATan2_est ( float a, float b, Context & ) { return v_extract_x(v_atan2_est(v_splats(a), v_splats(b))); }
    };

    struct SimPolicy_F2IVec {
        static __forceinline vec4f Trunci ( vec4f a, Context & )          { return v_cast_vec4f(v_cvt_vec4i(a)); }
        static __forceinline vec4f Roundi ( vec4f a, Context & )          { return v_cast_vec4f(v_cvt_roundi(a)); }
        static __forceinline vec4f Floori ( vec4f a, Context & )          { return v_cast_vec4f(v_cvt_floori(a)); }
        static __forceinline vec4f Ceili  ( vec4f a, Context & )          { return v_cast_vec4f(v_cvt_ceili(a)); }
    };

    struct SimPolicy_MathVec {
        static __forceinline vec4f Abs   ( vec4f a, Context & )          { return v_abs(a); }
        static __forceinline vec4f Floor ( vec4f a, Context & )          { return v_floor(a); }
        static __forceinline vec4f Ceil  ( vec4f a, Context & )          { return v_ceil(a); }
        static __forceinline vec4f Sqrt  ( vec4f a, Context & )          { return v_sqrt4(a); }
        static __forceinline vec4f Min   ( vec4f a, vec4f b, Context & ) { return v_min(a,b); }
        static __forceinline vec4f Max   ( vec4f a, vec4f b, Context & ) { return v_max(a,b); }
        static __forceinline vec4f Sat   ( vec4f a, Context & )          { return v_min(v_max(a,v_zero()),v_splats(1.0f)); }
        static __forceinline vec4f Clamp ( vec4f a, vec4f r0, vec4f r1, Context & ) { return v_max(v_min(a,r1), r0); }
        static __forceinline vec4f Mad   ( vec4f a, vec4f b, vec4f c, Context & ) { return v_madd(a,b,c); }
        static __forceinline vec4f MadS  ( vec4f a, vec4f b, vec4f c, Context & ) { return v_madd(a,v_perm_xxxx(b),c); }
        static __forceinline vec4f Lerp  ( vec4f a, vec4f b, vec4f t, Context & ) { return v_madd(v_sub(b,a),t,a); }

        static __forceinline vec4f Exp   ( vec4f a, Context & )          { return v_exp(a); }
        static __forceinline vec4f Log   ( vec4f a, Context & )          { return v_log(a); }
        static __forceinline vec4f Exp2  ( vec4f a, Context & )          { return v_exp2(a); }
        static __forceinline vec4f Log2  ( vec4f a, Context & )          { return v_log2_est_p5(a); }
        static __forceinline vec4f Pow   ( vec4f a, vec4f b, Context & ) { return v_pow(a, b); }
        static __forceinline vec4f Rcp   ( vec4f a, Context & )          { return v_rcp(a); }
        static __forceinline vec4f RcpEst( vec4f a, Context & )          { return v_rcp_est(a); }

        static __forceinline vec4f Sin ( vec4f a, Context & ) { vec4f s,c; v_sincos4(a, s, c);return s; }
        static __forceinline vec4f Cos ( vec4f a, Context & ) { vec4f s,c; v_sincos4(a, s, c);return c; }
        static __forceinline vec4f Tan ( vec4f a, Context & ) { vec4f s,c; v_sincos4(a, s, c);return v_div(s,c); }
        static __forceinline vec4f ATan( vec4f a, Context & ) { return v_atan(a); }
        static __forceinline vec4f ASin( vec4f a, Context & ) { return v_asin(a); }
        static __forceinline vec4f ACos( vec4f a, Context & ) { return v_acos(a); }
        static __forceinline vec4f ATan2 ( vec4f a, vec4f b, Context & ) { return v_atan2(a, b); }
        static __forceinline vec4f ATan2_est ( vec4f a, vec4f b, Context & ) { return v_atan2_est(a, b); }
    };

    template <typename TT, int mask>
    struct SimPolicy_Vec {
        // setXYZW
        static __forceinline vec4f setXYZW ( float x, float y, float z, float w ) {
            return v_make_vec4f(x, y, z, w);
        }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context & ) {
            return (v_signmask(v_cmp_eq(a,b)) & mask) == mask;
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context & ) {
            return (v_signmask(v_cmp_eq(a, b)) & mask) != mask;
        }
        // numeric
        static __forceinline vec4f Unp ( vec4f x, Context & ) {
            return x;
        }
        static __forceinline vec4f Unm ( vec4f x, Context & ) {
            return v_neg(x);
        }
        static __forceinline vec4f Add ( vec4f a, vec4f b, Context & ) {
            return v_add(a,b);
        }
        static __forceinline vec4f Sub ( vec4f a, vec4f b, Context & ) {
            return v_sub(a,b);
        }
        static __forceinline vec4f Div ( vec4f a, vec4f b, Context & ) {
            return v_div(a,b);
        }
        static __forceinline vec4f Mod ( vec4f a, vec4f b, Context & ) {
            return v_mod(a,b);
        }
        static __forceinline vec4f Mul ( vec4f a, vec4f b, Context & ) {
            return v_mul(a,b);
        }
        static __forceinline void SetAdd  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *) a;
            *pa = cast<TT>::to ( v_add(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetSub  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_sub(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetDiv  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_div(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetMul  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_mul(cast<TT>::from(*pa), b));
        }
        static __forceinline void SetMod  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_mod(cast<TT>::from(*pa), b));
        }
        // vector-scalar
        static __forceinline vec4f DivVecScal ( vec4f a, vec4f b, Context & ) {
            return v_div(a,v_splat_x(b));
        }
        static __forceinline vec4f MulVecScal ( vec4f a, vec4f b, Context & ) {
            return v_mul(a,v_splat_x(b));
        }
        static __forceinline vec4f DivScalVec ( vec4f a, vec4f b, Context & ) {
            return v_div(v_splat_x(a),b);
        }
        static __forceinline vec4f MulScalVec ( vec4f a, vec4f b, Context & ) {
            return v_mul(v_splat_x(a),b);
        }
        static __forceinline void SetDivScal  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_div(cast<TT>::from(*pa), v_splat_x(b)));
        }
        static __forceinline void SetMulScal  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to ( v_mul(cast<TT>::from(*pa), v_splat_x(b)));
        }
    };

    template <typename TT, int mask>
    struct SimPolicy_iVec {
        // setXYZW
        static __forceinline vec4f setXYZW ( int32_t x, int32_t y, int32_t z, int32_t w ) {
            return v_cast_vec4f(v_make_vec4i(x, y, z, w));
        }
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context & ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a),v_cast_vec4i(b)))) & mask) == mask;
        }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context & ) {
            return (v_signmask(v_cast_vec4f(v_cmp_eqi(v_cast_vec4i(a), v_cast_vec4i(b)))) & mask) != mask;
        }
        // numeric
        static __forceinline vec4f Unp ( vec4f x, Context & ) {
            return x;
        }
        static __forceinline vec4f Unm ( vec4f x, Context & ) {
            return v_cast_vec4f(v_negi(v_cast_vec4i(x)));
        }
        static __forceinline vec4f Add ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_addi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Sub ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_subi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Div ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_divi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mod ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_modi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mul ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_muli(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinAnd ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_andi(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinOr ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_ori(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinXor ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_xori(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinShl ( vec4f a, vec4f b, Context & ) {
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            return v_cast_vec4f(v_sll(v_cast_vec4i(a),shift));
        }
        static __forceinline vec4f BinShr ( vec4f a, vec4f b, Context & ) {
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            return v_cast_vec4f(v_sra(v_cast_vec4i(a),shift));
        }
        static __forceinline void SetAdd  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_addi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetSub  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_subi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetDiv  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_divi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMul  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_muli(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMod  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_modi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinAnd  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_andi(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinOr  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_ori(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinXor  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_xori(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinShl  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            *pa = cast<TT>::to (v_cast_vec4f(v_sll(v_cast_vec4i(cast<TT>::from(*pa)), shift)));
        }
        static __forceinline void SetBinShr  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            *pa = cast<TT>::to (v_cast_vec4f(v_sra(v_cast_vec4i(cast<TT>::from(*pa)), shift)));
        }
        // vector-scalar
        static __forceinline vec4f DivVecScal ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_divi(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f MulVecScal ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_muli(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f DivScalVec ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_divi(v_splat_xi(v_cast_vec4i(a)),v_cast_vec4i(b)));
        }
        static __forceinline vec4f MulScalVec ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_muli(v_splat_xi(v_cast_vec4i(a)),v_cast_vec4i(b)));
        }
        static __forceinline void SetDivScal ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_divi(v_cast_vec4i(cast<TT>::from(*pa)),
                v_splat_xi(v_cast_vec4i(b)))));
        }
        static __forceinline void SetMulScal ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_muli(v_cast_vec4i(cast<TT>::from(*pa)),
                v_splat_xi(v_cast_vec4i(b)))));
        }
    };

    template <typename TT, int mask>
    struct SimPolicy_uVec : SimPolicy_iVec<TT,mask> {
        // swapping some numeric operations
        static __forceinline vec4f Div ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_divu(v_cast_vec4i(a), v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mul ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_mulu(v_cast_vec4i(a), v_cast_vec4i(b)));
        }
        static __forceinline vec4f Mod ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_modu(v_cast_vec4i(a),v_cast_vec4i(b)));
        }
        static __forceinline vec4f BinShr ( vec4f a, vec4f b, Context & ) {
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            return v_cast_vec4f(v_srl(v_cast_vec4i(a),shift));
        }
        static __forceinline void SetDiv  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_divu(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMul  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_mulu(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetMod  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_modu(v_cast_vec4i(cast<TT>::from(*pa)), v_cast_vec4i(b))));
        }
        static __forceinline void SetBinShr  ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            int32_t shift = v_extract_xi(v_cast_vec4i(b));
            *pa = cast<TT>::to (v_cast_vec4f(v_srl(v_cast_vec4i(cast<TT>::from(*pa)), shift)));
        }
        // vector-scalar
        static __forceinline vec4f DivVecScal ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_divu(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f MulVecScal ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_mulu(v_cast_vec4i(a),v_splat_xi(v_cast_vec4i(b))));
        }
        static __forceinline vec4f DivScalVec ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_divu(v_splat_xi(v_cast_vec4i(a)), v_cast_vec4i(b)));
        }
        static __forceinline vec4f MulScalVec ( vec4f a, vec4f b, Context & ) {
            return v_cast_vec4f(v_mulu(v_splat_xi(v_cast_vec4i(a)), v_cast_vec4i(b)));
        }
        static __forceinline void SetDivScal ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_divu(v_cast_vec4i(cast<TT>::from(*pa)),
                v_splat_xi(v_cast_vec4i(b)))));
        }
        static __forceinline void SetMulScal ( char * a, vec4f b, Context & ) {
            TT * pa = (TT *)a;
            *pa = cast<TT>::to (v_cast_vec4f(v_mulu(v_cast_vec4i(cast<TT>::from(*pa)),
                v_splat_xi(v_cast_vec4i(b)))));
        }
    };

    struct SimPolicy_Range : SimPolicy_iVec<range,3> {};
    struct SimPolicy_URange : SimPolicy_uVec<urange,3> {};

    extern const char * rts_null;

    __forceinline const char * to_rts ( vec4f a ) {
        const char * s = cast<const char *>::to(a);
        return s ? s : rts_null;
    }

    struct SimPolicy_String {
        // basic
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context & ) { return strcmp(to_rts(a), to_rts(b))==0; }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context & ) { return (bool) strcmp(to_rts(a), to_rts(b)); }
        // ordered
        static __forceinline bool LessEqu ( vec4f a, vec4f b, Context & ) { return strcmp(to_rts(a), to_rts(b))<=0; }
        static __forceinline bool GtEqu   ( vec4f a, vec4f b, Context & ) { return strcmp(to_rts(a), to_rts(b))>=0; }
        static __forceinline bool Less    ( vec4f a, vec4f b, Context & ) { return strcmp(to_rts(a), to_rts(b))<0; }
        static __forceinline bool Gt      ( vec4f a, vec4f b, Context & ) { return strcmp(to_rts(a), to_rts(b))>0; }
        static vec4f Add  ( vec4f a, vec4f b, Context & context );
        static void SetAdd ( char * a, vec4f b, Context & context );
    };

    // generic SimPolicy<X>

    template <typename TT>
    struct SimPolicy;

    template <> struct SimPolicy<bool> : SimPolicy_Bool {};
    template <> struct SimPolicy<int32_t> : SimPolicy_Int {};
    template <> struct SimPolicy<uint32_t> : SimPolicy_UInt {};
    template <> struct SimPolicy<int64_t> : SimPolicy_Int64 {};
    template <> struct SimPolicy<uint64_t> : SimPolicy_UInt64 {};
    template <> struct SimPolicy<float> : SimPolicy_Float, SimPolicy_MathFloat {};
    template <> struct SimPolicy<double> : SimPolicy_Double {};
    template <> struct SimPolicy<void *> : SimPolicy_Pointer {};
    template <> struct SimPolicy<float2> : SimPolicy_Vec<float2,1+2>, SimPolicy_MathVec, SimPolicy_F2IVec {};
    template <> struct SimPolicy<float3> : SimPolicy_Vec<float3,1+2+4>, SimPolicy_MathVec, SimPolicy_F2IVec {};
    template <> struct SimPolicy<float4> : SimPolicy_Vec<float4,1+2+4+8>, SimPolicy_MathVec, SimPolicy_F2IVec {};
    template <> struct SimPolicy<int2> : SimPolicy_iVec<int2,1+2>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<int3> : SimPolicy_iVec<int3,1+2+4>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<int4> : SimPolicy_iVec<int4,1+2+4+8>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<uint2> : SimPolicy_uVec<uint2,1+2>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<uint3> : SimPolicy_uVec<uint3,1+2+4>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<uint4> : SimPolicy_uVec<uint4,1+2+4+8>, SimPolicy_MathVecI, SimPolicy_F2IVec {};
    template <> struct SimPolicy<range> : SimPolicy_Range {};
    template <> struct SimPolicy<urange> : SimPolicy_URange {};
    template <> struct SimPolicy<char *> : SimPolicy_String {};

}
