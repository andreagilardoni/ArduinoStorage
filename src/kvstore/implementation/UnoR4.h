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

#include "../kvstore.h"
#include <Arduino.h>
#include <Modem.h>
#include <string>

using namespace std;

constexpr char DEFAULT_KVSTORE_NAME[] = "arduino";

class Unor4KVStore: public KVStoreInterface<const char*> {
public:
    typedef enum {
        PT_I8, PT_U8, PT_I16, PT_U16, PT_I32, PT_U32, PT_I64, PT_U64, PT_STR, PT_BLOB, PT_INVALID
    } Type;

    Unor4KVStore(): name(DEFAULT_KVSTORE_NAME) {}

    bool begin();
    bool begin(const char* name, bool readOnly=false, const char* partitionLabel=nullptr);
    bool end();
    bool clear();

    typename KVStoreInterface<const char*>::res_t remove(const key_t& key) override;
    bool exists(const key_t& key) const override;
    typename KVStoreInterface<const char*>::res_t putBytes(const key_t& key, uint8_t b[], size_t s) override;
    typename KVStoreInterface<const char*>::res_t getBytes(const key_t& key, uint8_t b[], size_t s) const override;
    size_t getBytesLength(const key_t& key) const override;

    template<typename T1>
    typename KVStoreInterface<const char*>::res_t put(const key_t& key, T1 value) override { return KVStoreInterface<const char*>::put(key, value); }

    template<typename T1>
    KVStoreInterface<const char*>::reference<T1> get(const key_t& key, T1 def = 0) override { return KVStoreInterface<const char*>::get(key, def); }

    size_t putString(key_t key, const char* value) override;
    size_t putString(key_t key, String value) override;
    size_t getString(const char* key, char* value, size_t maxLen) override;
    String getString(key_t key, const String defaultValue = String()) override;
private:
    const char* name;
};
