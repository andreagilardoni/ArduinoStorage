/*
 * This file is part of Arduino_Storage.
 *
 * Copyright (c) 2024 Arduino SA
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "../kvstore.h"

// #if ! __has_include(<utility/wifi_drv.h>)
// #error "WiFiNina library is required for this to work"
// #endif

#include <WiFi.h>

using namespace std;

const char DEFAULT_KVSTORE_NAME[] = "arduino";

class SAMDKVStore: public KVStoreInterface<const char*> {
public:
    SAMDKVStore(const char* name=DEFAULT_KVSTORE_NAME): name(name) {}
    bool begin();
    bool begin(const char* name);
    bool end();
    bool clear();

    typename KVStoreInterface<const char*>::res_t remove(const key_t& key) override;
    bool exists(const key_t& key) const override;
    typename KVStoreInterface<const char*>::res_t putBytes(const key_t& key, uint8_t b[], size_t s) override;
    typename KVStoreInterface<const char*>::res_t getBytes(const key_t& key, uint8_t b[], size_t s) const override;
    size_t getBytesLength(const key_t& key) const override;

    template<typename T1>
    typename KVStoreInterface<const char*>::res_t put(const key_t& key, T1 value) { return KVStoreInterface<const char*>::put(key, value); }

    template<typename T1>
    KVStoreInterface<const char*>::reference<T1> get(const key_t& key, T1 def = 0) { return KVStoreInterface<const char*>::get(key, def); }

private:
    const char* name;
};
