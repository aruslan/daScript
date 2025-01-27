#pragma once

#include "daScript/misc/arraytype.h"
#include "daScript/simulate/hash.h"

namespace das
{

    // TODO:
    //  -   return correct insert index of original value? is this at all possible?
    //  -   throw runtime error in the context, when grow inside locked table (recover well)

    extern const char * rts_null;

    template <typename KeyType>
    struct KeyCompare {
        __forceinline bool operator () ( const KeyType & a, const KeyType & b ) {
            return a == b;
        }
    };

    template <>
    struct KeyCompare <char *> {
        __forceinline bool operator () ( const char * a, const char * b ) {
            if ( a==b ) return true;
            if ( !a || !b ) return false;
            return strcmp(a,b)==0;
        }
    };


    template <typename KeyType>
    class TableHash {
        Context *   context = nullptr;
        uint32_t    valueTypeSize = 0;
        constexpr static uint32_t minCapacity = 64u;
        constexpr static uint32_t minLookups = 4u;
    public:
        TableHash () = delete;
        TableHash ( const TableHash & ) = delete;
        TableHash ( Context * ctx, uint32_t vs ) : context(ctx), valueTypeSize(vs) {}

        __forceinline uint32_t indexFromHash(uint32_t hash, uint32_t shift ) const {
            return hash >> shift; // i don't know why this is faster, but it is
        }

        __forceinline uint32_t computeShift(uint32_t capacity) {
            return __builtin_clz(capacity-1);
        }

        __forceinline uint32_t computeMaxLookups(uint32_t capacity) {
            uint32_t desired = 32 - __builtin_clz(capacity-1);
            return das::max(minLookups, desired * 6);
        }

        __forceinline int find ( Table & tab, KeyType key, uint32_t hash ) const {
            uint32_t mask = tab.capacity - 1;
            uint32_t index = indexFromHash(hash, tab.shift);
            uint32_t lastI = (index+tab.maxLookups) & mask;
            auto pKeys = (const KeyType *) tab.keys;
            auto pHashes = tab.hashes;
            while ( index != lastI ) {
                auto kh = pHashes[index];
                if ( kh==HASH_EMPTY32 ) {
                    return -1;
                } else if ( kh==hash && KeyCompare<KeyType>()(pKeys[index],key) ) {
                    return (int) index;
                }
                index = (index + 1) & mask;
            }
            return -1;
        }

        __forceinline int insertNew ( Table & tab, uint32_t hash ) const {
            // TODO: take key under account and be less agressive?
            uint32_t mask = tab.capacity - 1;
            uint32_t index = indexFromHash(hash, tab.shift);
            uint32_t lastI = (index+tab.maxLookups) & mask;
            auto pHashes = tab.hashes;
            while ( index != lastI ) {
                auto kh = pHashes[index];
                if ( kh==HASH_EMPTY32 ) {
                    return (int) index;
                }
                index = (index + 1) & mask;
            }
            return -1;
        }

        __forceinline int reserve ( Table & tab, KeyType key, uint32_t hash ) {
            for ( ;; ) {
                uint32_t mask = tab.capacity - 1;
                uint32_t index = indexFromHash(hash, tab.shift);
                uint32_t lastI = (index+tab.maxLookups) & mask;
                auto pKeys = (KeyType *) tab.keys;
                auto pHashes = tab.hashes;
                while ( index != lastI ) {
                    auto kh = pHashes[index];
                    if (kh <= HASH_KILLED32) {
                        pHashes[index] = hash;
                        pKeys[index] = key;
                        tab.size++;
                        return (int)index;
                    } else if (kh == hash && KeyCompare<KeyType>()(pKeys[index], key)) {
                        return (int)index;
                    }
                    index = (index + 1) & mask;
                }
                if ( !grow(tab) ) {
                    return -1;
                }
            }
        }

        __forceinline int erase ( Table & tab, KeyType key, uint32_t hash ) {
            uint32_t mask = tab.capacity - 1;
            uint32_t index = indexFromHash(hash, tab.shift);
            uint32_t lastI = (index+tab.maxLookups) & mask;
            auto pKeys = (const KeyType *) tab.keys;
            auto pHashes = tab.hashes;
            while ( index != lastI ) {
                auto kh = pHashes[index];
                if ( kh==HASH_EMPTY32 ) {
                    return -1;
                } else if ( kh==hash && KeyCompare<KeyType>()(pKeys[index],key) ) {
                    tab.size--;
                    pHashes[index] = HASH_KILLED32;
                    memset(tab.data + index*valueTypeSize, 0, valueTypeSize);
                    return (int) index;
                }
                index = (index + 1) & mask;
            }
            return -1;
        }

        bool grow ( Table & tab ) {
            uint32_t newCapacity = das::max(minCapacity, tab.capacity*2);
        repeatIt:;
            Table newTab;
            uint32_t memSize = newCapacity * (valueTypeSize + sizeof(KeyType) + sizeof(uint32_t));
            newTab.data = (char *) context->heap.allocate(memSize);
            if ( !newTab.data ) {
                context->throw_error("can't grow table, out of heap");
                return false;
            }
            newTab.keys = newTab.data + newCapacity * valueTypeSize;
            newTab.hashes = (uint32_t *)(newTab.keys + newCapacity * sizeof(KeyType));
            newTab.size = tab.size;
            newTab.capacity = newCapacity;
            newTab.lock = tab.lock;
            newTab.maxLookups = computeMaxLookups(newCapacity);
            newTab.shift = computeShift(newCapacity);
            memset(newTab.data, 0, newCapacity*valueTypeSize);
            auto pHashes = newTab.hashes;
            memset(pHashes, 0, newCapacity * sizeof(uint32_t));
            if ( tab.size ) {
                auto pKeys = (KeyType *) newTab.keys;
                auto pOldValues = tab.data;
                auto pValues = newTab.data;
                auto pOldKeys = (const KeyType *) tab.keys;
                auto pOldHashes = tab.hashes;
                for ( uint32_t i=0; i!=tab.capacity; ++i ) {
                    auto hash = pOldHashes[i];
                    if ( hash>HASH_KILLED32 ) {
                        int index = insertNew(newTab, hash);
                        if ( index==-1 ) {
                             newCapacity *= 2;
                            goto repeatIt;
                        } else {
                            pHashes[index] = hash;
                            pKeys[index] = pOldKeys[i];
                            memcpy ( pValues + index*valueTypeSize, pOldValues + i*valueTypeSize, valueTypeSize );
                        }
                    }
                }
            }
            swap ( newTab, tab );
            return true;
        }
    };
}



