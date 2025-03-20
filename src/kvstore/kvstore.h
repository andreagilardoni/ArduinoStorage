/*
 * This file is part of Arduino_KVStore.
 *
 * Copyright (c) 2024 Arduino SA
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <Arduino.h>
#include <math.h>

/** KVStoreInterface class
 *
 * Interface for HW abstraction of a KV store
 *
 * In order to implement this interface it is just required to implement just the pure virtual methods
 * and as a consequence all the other methods will work out of the box.
 * If some datatypes have optimizations, put and get methods can be overwritten and specialized for them
 */
template<typename Key=const char*>
class KVStoreInterface {
public:

    typedef Key key_t;
    typedef int res_t;

    // proxy class to allow operator[] with assignment

    /** reference class
     *
     * This class is a container that holds a key-value pair and provides
     * access to it in both read and write mode
     */
    template<typename T>
    class reference {
    public:
        reference(const key_t &key, T value, KVStoreInterface& owner): key(key), value(value), owner(owner) {}

        // assign a new value to the reference and update the store
        reference& operator=(T t) noexcept {
            printf("save the value\n");
            value = t;
            this->save();
            return *this;
        }

        // assign a new value to the reference copying from another reference value
        reference& operator=(const reference<T>& r) noexcept {
            value = r.value;
            return *this;
        }

        // get the referenced value
        T operator*() noexcept       { return getValue(); }

        // cast the reference to the value it contains -> get the value references
        operator T () noexcept       { return getValue(); }

        inline key_t getKey() const  { return key; }
        inline T getValue()          { load(); return value; } // FIXME is it ok to load it every time?

        // load the stored value
        void load()                  { value = owner.get<T>(key).value; }

        // save the value contained in this reference
        void save()                  { owner.put(key, value); }

        // check if this reference is contained in the store
        bool exists() const          { return owner.exists(key); }

        // remove this reference from the store
        void remove()                { owner.remove(key); }
    private:
        const key_t key;
        T value;

        KVStoreInterface<Key>& owner;
    };
    // static constexpr reference<void*> NullReference = reference<void*>(0, nullptr); // TODO

    /**
     * @brief virtual empty destructor
     */
    virtual ~KVStoreInterface() {};

    /**
     * @brief function that provides initializatiopn for the KV store
     *
     * @returns true on correct execution false otherwise
     */
    virtual bool begin() = 0;

    /**
     * @brief function that provides finalization for the KV store
     *
     * @returns true on correct execution false otherwise
     */
    virtual bool end() = 0;

    /**
     * @brief function that clears all the content of the KV store
     *
     * @returns true on correct execution false otherwise
     */
    virtual bool clear() = 0;

    /**
     * @brief remove what is referenced with the key
     *
     * @param[in]  key              Key which data has to be deleted
     *
     * @returns 1 on correct execution anything else otherwise
     */
    virtual res_t remove(const key_t& key) = 0;

    /**
     * @brief check if a key is already being used
     *
     * @param[in]  key              Key to search for
     *
     * @returns 1 on correct execution anything else otherwise
     */
    virtual bool exists(const key_t& key) const = 0;

    /**
     * @brief put values in the store provinding a byte array
     *
     * @param[in]  key              Key to insert
     * @param[in]  b                byte array
     * @param[in]  s                the length of the array
     *
     * @returns 1 on correct execution anything else otherwise
     */
    virtual res_t putBytes(const key_t& key, uint8_t *b, size_t s) = 0;

    /**
     * @brief get values from the store provinding a byte array
     * TODO make this better
     *
     * @param[in]  key              Key to get
     * @param[out] b                byte array
     * @param[in]  s                the length of the array
     *
     * @returns 1 on correct execution anything else otherwise
     */
    virtual res_t getBytes(const key_t& key, uint8_t *b, size_t s) const = 0;

    /**
     * @brief get the number of bytes used by a certain value referenced by key
     * TODO make this better
     *
     * @param[in]  key              Key
     *
     * @returns len>0 if the key exists 0 otherwise
     */
    virtual size_t getBytesLength(const key_t& key) const = 0;

    /**
     * @brief templated method that puts a value of a certain type T
     *        by converting it to a bytearray and puts it into the KV store
     *
     * @param[in]  key              Key
     * @param[in]  value            Value to insert
     *
     * @returns 1 on correct execution anything else otherwise
     */
    template<typename T> // TODO define res_t // FIXME should these be virtual
    inline res_t put(const key_t& key, T value) { return putBytes(key, (uint8_t*)&value, sizeof(value)); }

    /**
     * @brief templated method that gets a value of a certain type T. If it doesn't exist in the store a
     *        reference is returned, which is not saved, until the proper method is called
     *
     * @param[in]  key              Key
     * @param[in]  def              a default value that is assigned to the reference object
     *
     * @returns a reference to the desired key
     */
    template<typename T>
    inline reference<T> get(const key_t& key, const T def = 0) {
        if(exists(key)) {
            T t;
            auto res = getBytes(key, (uint8_t*)&t, sizeof(t)); // FIXME res not considered
            return reference<T>(key, t, *this);
        } else {
            return reference<T>(key, def, *this);
        }
    }

    /**
     * @brief RW direct access to a value with the operator[]
     *
     * @param[in]  key              Key
     *
     * @returns a reference to the desired key
     */
    template<typename T>
    reference<T>& operator[](const key_t& key) { // write access to the value
        return get<T>(key);
    }

    /**
     * @brief R direct access to a value with the operator[]
     *
     * @param[in]  key              Key
     *
     * @returns a read-only reference to the desired key
     */
    template<typename T>
    const reference<T>& operator[](const key_t& key) const { // ro access to the value
        return get<T>(key);
    }

    /**
     * @brief put a char in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putChar(key_t key, const int8_t value)                        { return put(key, value); }

    /**
     * @brief put a uchar in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putUChar(key_t key, const uint8_t value)                      { return put(key, value); }

    /**
     * @brief put a short in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putShort(key_t key, const int16_t value)                      { return put(key, value); }

    /**
     * @brief put a ushort in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putUShort(key_t key, const uint16_t value)                    { return put(key, value); }

    /**
     * @brief put an int in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putInt(key_t key, const int32_t value)                        { return put(key, value); }

    /**
     * @brief put a uint in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putUInt(key_t key, const uint32_t value)                      { return put(key, value); }

    /**
     * @brief put a long in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putLong(key_t key, const int32_t value)                       { return put(key, value); }

    /**
     * @brief put a ulong in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putULong(key_t key, const uint32_t value)                     { return put(key, value); }

    /**
     * @brief put a long64 in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putLong64(key_t key, const int64_t value)                     { return put(key, value); }

    /**
     * @brief put a ulong64 in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putULong64(key_t key, const uint64_t value)                   { return put(key, value); }

    /**
     * @brief put a float in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putFloat(key_t key, const float value)                        { return put(key, value); }

    /**
     * @brief put a double in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putDouble(key_t key, const double value)                      { return put(key, value); }

    /**
     * @brief put a bool in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putBool(key_t key, const bool value)                          { return put(key, value); }

    /**
     * @brief put a C string in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putString(key_t key, const char * const value)                 { return putBytes(key, (uint8_t*)value, strlen(value)); }

#ifdef ARDUINO
    /**
     * @brief put an Arduino string in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  value            value
     *
     * @returns the size of the inserted value
     */
    virtual size_t      putString(key_t key, const String value)                      { return putBytes(key, (uint8_t*)value.c_str(), value.length()); }
#endif // ARDUINO

    /**
     * @brief get a char in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual int8_t      getChar(key_t key, const int8_t defaultValue = 0)             { return get(key, defaultValue); }

    /**
     * @brief get a uchar in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual uint8_t     getUChar(key_t key, const uint8_t defaultValue = 0)           { return get(key, defaultValue); }

    /**
     * @brief get a short in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual int16_t     getShort(key_t key, const int16_t defaultValue = 0)           { return get(key, defaultValue); }

    /**
     * @brief get a ushort in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual uint16_t    getUShort(key_t key, const uint16_t defaultValue = 0)         { return get(key, defaultValue); }

    /**
     * @brief get an int in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual int32_t     getInt(key_t key, const int32_t defaultValue = 0)             { return get(key, defaultValue); }

    /**
     * @brief get a uint in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual uint32_t    getUInt(key_t key, const uint32_t defaultValue = 0)           { return get(key, defaultValue); }

    /**
     * @brief get a long in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual int32_t     getLong(key_t key, const int32_t defaultValue = 0)            { return get(key, defaultValue); }

    /**
     * @brief get a ulong in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual uint32_t    getULong(key_t key, const uint32_t defaultValue = 0)          { return get(key, defaultValue); }

    /**
     * @brief get a long64 in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual int64_t     getLong64(key_t key, const int64_t defaultValue = 0)          { return get(key, defaultValue); }

    /**
     * @brief get a ulong64 in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual uint64_t    getULong64(key_t key, const uint64_t defaultValue = 0)        { return get(key, defaultValue); }

    /**
     * @brief get a float in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual float       getFloat(key_t key, const float defaultValue = NAN)             { return get(key, defaultValue); }

    /**
     * @brief get a double in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual double      getDouble(key_t key, const double defaultValue = NAN)           { return get(key, defaultValue); }

    /**
     * @brief get a bool in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual bool        getBool(key_t key, const bool defaultValue = false)           { return get(key, defaultValue); }

    /**
     * @brief get a C string in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual size_t      getString(key_t key, char* value, size_t maxLen)        { return getBytes(key, (uint8_t*)value, maxLen); }

#ifdef ARDUINO
    /**
     * @brief get an Arduino String in the kvstore
     *
     * @param[in]  key              Key
     * @param[in]  defaultValue     in the case the key do not exist this value is returned
     *
     * @returns the value present in the kvstore or defaultValue if not present
     */
    virtual String getString(key_t key, const String defaultValue = String()) {
        size_t len = getBytesLength(key);
        char *str = new char[len+1];

        getString(key, str, len+1);
        str[len] = '\0';

        String res(str);
        delete str;
        str = nullptr;

        return res;
    }
#endif // ARDUINO
};
