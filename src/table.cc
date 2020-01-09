#include "table.hpp"

using namespace aup;

#define TABLE_MAX_LOAD  0.75

static Entry *findEntry(vector<Entry>& entries, int capacity, String *key)
{
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];

        if (entry->key == NULL) {
            if (entry->value.isNil()) {
                // Empty entry.                              
                return tombstone != NULL ? tombstone : entry;
            }
            else {
                // We found a tombstone.                     
                if (tombstone == NULL) tombstone = entry;
            }
        }
        else if (entry->key == key) {
            // We found the key.                           
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

bool Table::get(String *key, Value& value)
{
    if (count == 0) return false;

    Entry *entry = findEntry(entries, capacity, key);
    if (entry->key == NULL) {
        return false;
    }

    value = entry->value;
    return true;
}

void Table::growCapacity()
{
    size_t newCapacity = (capacity < 8) ? 8 : (capacity * 2);
    vector<Entry> newEntries = vector<Entry>(newCapacity, Entry());

    count = 0;
    for (size_t i = 0; i < capacity; i++) {
        Entry *entry = &newEntries[i];
        if (entry->key == NULL) continue;

        Entry *dest = findEntry(newEntries, newCapacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        count++;
    }

    entries.swap(newEntries);
    capacity = newCapacity;
}

bool Table::set(String *key, const Value& value)
{
    if (count + 1 > capacity * TABLE_MAX_LOAD) { 
        growCapacity();
    }

    Entry *entry = findEntry(entries, capacity, key);

    bool isNewKey = (entry->key == NULL);
    if (isNewKey && entry->value.isNil()) {
        count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool Table::remove(String *key)
{
    if (count == 0) return false;

    // Find the entry.                                             
    Entry *entry = findEntry(entries, capacity, key);
    if (entry->key == NULL) return false;

    // Place a tombstone in the entry.                             
    entry->key = NULL;
    entry->value = Value(true);

    return true;
}

void Table::addAll(Table& from)
{
    for (size_t i = 0; i < from.capacity; i++) {
        Entry *entry = &from.entries[i];
        if (entry->key != NULL) {
            set(entry->key, entry->value);
        }
    }
}

String *Table::findString(const char *chars, int length, uint32_t hash)
{
    if (count == 0) return NULL;

    uint32_t index = hash % capacity;

    for (;;) {
        Entry *entry = &entries[index];
        String *key = entry->key;

        if (key == NULL) {
            // Stop if we find an empty non-tombstone entry.                 
            if (entry->value.isNil()) return NULL;
        }
        else if (key->length == length && key->hash == hash) {
            // We found it.                                                  
            return key;
        }

        index = (index + 1) % capacity;
    }
}
