#include "hash.hpp"

using namespace aup;
using namespace std;

#define HASH_MAX_LOAD   0.75

static Index *findIndex(vector<Index>& indexes, size_t capacity, uint64_t key)
{
    uint32_t i = key % capacity;
    Index *tombstone = NULL;

    for (;;) {
        Index *index = &indexes[i];

        if (index->key == Index::UNUSED) {
            if (index->value.isNil()) {
                // Empty entry.                              
                return tombstone != NULL ? tombstone : index;
            }
            else {
                // We found a tombstone.                     
                if (tombstone == NULL) tombstone = index;
            }
        }
        else if (index->key == key) {
            // We found the key.
            return index;
        }

        i = (i + 1) % capacity;
    }
}

void Hash::growCapacity()
{
    size_t newCapacity = (capacity < 8) ? 8 : (capacity * 2);
    vector<Index> newIndexes = vector<Index>(newCapacity, Index());

    count = 0;
    for (size_t i = 0; i < capacity; i++) {
        Index *index = &indexes[i];
        if (index->key == Index::UNUSED) continue;

        Index *dest = findIndex(newIndexes, newCapacity, index->key);
        dest->key = index->key;
        dest->value = index->value;
        count++;
    }

    indexes.swap(newIndexes);
    capacity = newCapacity;
}

bool Hash::get(uint64_t key, Value& value)
{
    if (count == 0) return false;

    Index *index = findIndex(indexes, capacity, key);
    if (index->key == Index::UNUSED) {
        return false;
    }

    value = index->value;
    return true;
}

bool Hash::set(uint64_t key, const Value& value)
{
    if (count + 1 > capacity * HASH_MAX_LOAD) {
        growCapacity();
    }

    Index *index = findIndex(indexes, capacity, key);

    bool isNewKey = (index->key == Index::UNUSED);
    if (isNewKey && index->value.isNil()) {
        count++;
    }

    index->key = key;
    index->value = value;
    return isNewKey;
}
