#include "daScript/misc/platform.h"

#include "daScript/simulate/bin_serializer.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/hash.h"

namespace das {

#ifndef DEBUG_BIN_DATA
#define DEBUG_BIN_DATA(...)
#endif

    struct BinDataSerialize : DataWalker {
        char * bytesAt = nullptr;
        uint32_t bytesAllocated = 0;
        uint32_t bytesWritten = 0;
        uint32_t bytesGrow = 1024;
    // writer
        BinDataSerialize ( Context & ctx ) {
            DEBUG_BIN_DATA("writing\n");
            context = &ctx;
            reading = false;
        }
        BinDataSerialize ( Context & ctx, char * b, uint32_t l ) {
            context = &ctx;
            reading = true;
            bytesAt = b;
            bytesAllocated = l;
            DEBUG_BIN_DATA("reading %i bytes\n", bytesAllocated);
        }
        __forceinline void read ( void * data, uint32_t size ) {
            if ( bytesWritten + size <= bytesAllocated ) {
                DEBUG_BIN_DATA("reading %i bytes at %i\n", size, bytesWritten );
                memcpy ( data, bytesAt + bytesWritten, size );
                bytesWritten += size;
            } else {
                error("binary data too short");
            }
        }
        __forceinline void write ( void * data, uint32_t size ) {
            if ( bytesWritten + size > bytesAllocated ) {
                uint32_t newSize = das::max ( bytesAllocated + bytesGrow, bytesWritten + size );
                bytesAt = context->heap.reallocate(bytesAt, bytesAllocated, newSize);
                bytesAllocated = newSize;
            }
            DEBUG_BIN_DATA("writing %i bytes at %i\n", size, bytesWritten );
            memcpy ( bytesAt + bytesWritten, data, size );
            bytesWritten += size;
        }
        template <typename TT>
        __forceinline void save ( TT & data ) {
            write ( &data, sizeof(TT) );
        }
        template <typename TT>
        __forceinline void load ( TT & data ) {
            read ( &data, sizeof(TT) );
        }
        template <typename TT>
        __forceinline void serialize ( TT & data ) {
            if ( reading ) {
                read ( &data, sizeof(TT) );
            } else {
                write ( &data, sizeof(TT) );
            }
        }
        __forceinline void verify ( uint32_t & data ) {
            if ( reading ) {
                uint32_t test = 0;
                load(test);
                DEBUG_BIN_DATA("verify, reading %xi\n", test);
                if ( test != data ) {
                    error("binary data type mismatch");
                }
            } else {
                DEBUG_BIN_DATA("verify, writing %xi\n", data);
                save(data);
            }
        }
        void close () {
            if ( !reading && bytesAt ) {
                DEBUG_BIN_DATA("close at %i bytes\n\n", bytesWritten);
                bytesAt = context->heap.reallocate(bytesAt, bytesAllocated, bytesWritten);
            }
        }
    // data structures
        virtual void beforeStructure ( char *, StructInfo * si ) override {
            verify(si->hash);
        }
        virtual void beforeDim ( char *, TypeInfo * ti ) override {
            verify(ti->hash);
            verify(ti->dimSize);
        }
        virtual void beforeArray ( Array * pa, TypeInfo * ti ) override {
            verify(ti->hash);
            if ( reading ) {
                uint32_t newSize;
                load(newSize);
                array_clear(*context, *pa);
                array_resize(*context, *pa, newSize, getTypeBaseSize(ti), true);
            } else {
                save(pa->size);
            }
        }
        virtual void beforeTable ( Table *, TypeInfo * ) override {
            error("binary serialization of tables is not supported");
        }
        virtual void beforePtr ( char *, TypeInfo * ) override {
            error("binary serialization of pointers is not supported");
        }
        virtual void beforeHandle ( char *, TypeInfo * ti ) override {
            verify(ti->hash);
        }
    // types
        virtual void String ( char * & data ) override {
            DEBUG_BIN_DATA("string\n");
            if ( reading ) {
                uint32_t length;
                load ( length );
                auto hh = (StringHeader *) context->heap.allocate(length + 1 + sizeof(StringHeader));
                hh->length = length;
                hh->hash = 0;
                data = (char *) (hh + 1);
                read ( data, length );
                data[length] = 0;
            } else {
                uint32_t length = stringLengthSafe(*context, data);
                save ( length );
                write ( data, length );
            }
        }
        virtual void Bool ( bool & data ) override {
            serialize(data);
        }
        virtual void Int8 ( int8_t & data ) override {
            serialize(data);
        }
        virtual void UInt8 ( uint8_t & data ) override {
            serialize(data);
        }
        virtual void Int16 ( int16_t & data ) override {
            serialize(data);
        }
        virtual void UInt16 ( uint16_t & data ) override {
            serialize(data);
        }
        virtual void Int64 ( int64_t & data ) override {
            serialize(data);
        }
        virtual void UInt64 ( uint64_t & data ) override {
            serialize(data);
        }
        virtual void Double ( double & data ) override {
            serialize(data);
        }
        virtual void Float ( float & data ) override {
            serialize(data);
        }
        virtual void Int ( int32_t & data ) override {
            serialize(data);
        }
        virtual void UInt ( uint32_t & data ) override {
            serialize(data);
        }
        virtual void Int2 ( int2 & data ) override {
            serialize(data);
        }
        virtual void Int3 ( int3 & data ) override {
            serialize(data);
        }
        virtual void Int4 ( int4 & data ) override {
            serialize(data);
        }
        virtual void UInt2 ( uint2 & data ) override {
            serialize(data);
        }
        virtual void UInt3 ( uint3 & data ) override {
            serialize(data);
        }
        virtual void UInt4 ( uint4 & data ) override {
            serialize(data);
        }
        virtual void Float2 ( float2 & data ) override {
            serialize(data);
        }
        virtual void Float3 ( float3 & data ) override {
            serialize(data);
        }
        virtual void Float4 ( float4 & data ) override {
            serialize(data);
        }
        virtual void Range ( range & data ) override {
            serialize(data);
        }
        virtual void URange ( urange & data ) override {
            serialize(data);
        }
        virtual void WalkEnumeration ( int32_t & data, EnumInfo * ei ) override {
            verify(ei->hash);
            serialize(data);
        }
        virtual void Null ( TypeInfo * ) override {
            error("binary serialization of null pointers is not supported");
        }
        virtual void WalkIterator ( struct Iterator * ) override {
            error("binary serialization of iterators is not supported");
        }
        virtual void WalkBlock ( Block * ) override {
            error("binary serialization of blocks is not supported");
        }
        virtual void WalkFunction ( Func * func ) override {
            serialize(func->index);
        }
        virtual void WalkLambda ( Lambda * lambda ) override {
            TypeInfo * info = nullptr;
            if ( reading ) {
                uint32_t hash = 0;
                serialize(hash);
                info = context->debugInfo->lookup[hash];    // TODO: verify if there is capture, all that
                DAS_ASSERTF(info,"type info not found. how did we get type, which is not in the typeinfo hash?");
                uint32_t size = getTypeSize(info) + 16;
                char * ptr = context->heap.allocate(size);
                memset ( ptr, 0, size );
                *((TypeInfo **)ptr) = info;
                ptr += 16;
                lambda->capture = ptr;
            } else {
                info = *((TypeInfo **) (((char *)lambda->capture)-16));
                serialize(info->hash);
            }
            walk ( (char *) lambda->capture, info );
        }
    };

    // save ( obj, block<(bytesAt)> )
    vec4f _builtin_binary_save ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        BinDataSerialize writer(context);
        // args
        Block * block = cast<Block *>::to(args[1]);
        auto info = call->types[0];
        StringHeader header;
        header.hash = 0;
        header.length = 0;
        writer.write(&header, sizeof(StringHeader));
        writer.walk(args[0], info);
        writer.close();
        vec4f bargs[2];
        StringHeader * hh = (StringHeader *) writer.bytesAt;
        hh->length = writer.bytesWritten;
        bargs[0] = cast<char *>::from(writer.bytesAt + sizeof(StringHeader));
        context.invoke(*block, bargs, nullptr);
        return v_zero();
    }

    // load ( obj, bytesAt )
    void _builtin_binary_load ( Context & context, TypeInfo* info, const char *data, uint32_t len, char *to) {
        if ( !(info->flags&(TypeInfo::flag_refType | TypeInfo::flag_ref)) )
            return;
        BinDataSerialize reader(context, const_cast<char*>(data), len);
        reader.walk(to, info);
    }

    vec4f _builtin_binary_load ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        char * ba = cast<char *>::to(args[1]);
        StringHeader * hh = ((StringHeader *)ba) - 1;
        _builtin_binary_load(context, call->types[0], ba, hh->length, cast<char *>::to(args[0]));
        return v_zero();
    }

}
