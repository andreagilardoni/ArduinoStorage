/*
 * This file is part of Arduino_KVStore.
 *
 * Copyright (c) 2024 Arduino SA
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#if defined(ARDUINO_ARCH_ESP32)
#include "ESP32.h"
#include "nvs.h"
#include "nvs_flash.h"

bool ESP32KVStore::begin(const char* name, bool readOnly, const char* partition_label) {
    if(_started){
        return false;
    }
    _readOnly = readOnly;
    esp_err_t err = ESP_OK;
    if (partition_label != NULL) {
        err = nvs_flash_init_partition(partition_label);
        if (err) {
            log_e("nvs_flash_init_partition failed: %s", nvs_error(err));
            return false;
        }
        err = nvs_open_from_partition(partition_label, name, readOnly ? NVS_READONLY : NVS_READWRITE, &_handle);
    } else {
        err = nvs_open(name, readOnly ? NVS_READONLY : NVS_READWRITE, &_handle);
    }
    if(err){
        log_e("nvs_open failed: %s", nvs_error(err));
        return false;
    }
    _started = true;
    return true;
}

bool ESP32KVStore::begin() {
    return begin(DEFAULT_KVSTORE_NAME);
}

bool ESP32KVStore::end() {
    if(!_started){
        return false;
    }
    nvs_close(_handle);
    _started = false;

    return true;
}

bool ESP32KVStore::clear() {
    if(!_started || _readOnly){
        return false;
    }
    esp_err_t err = nvs_erase_all(_handle);
    if(err){
        log_e("nvs_erase_all fail: %s", nvs_error(err));
        return false;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s", nvs_error(err));
        return false;
    }
    return true;
}

typename KVStoreInterface<const char*>::res_t ESP32KVStore::remove(const key_t& key) {
    if(!_started || !key || _readOnly){
        return false;
    }
    esp_err_t err = nvs_erase_key(_handle, key);
    if(err){
        log_e("nvs_erase_key fail: %s %s", key, nvs_error(err));
        return false;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return false;
    }
    return true;
}

typename KVStoreInterface<const char*>::res_t ESP32KVStore::putBytes(const key_t& key, uint8_t value[], size_t len) {
    if(!_started || !key || !value || !len || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_blob(_handle, key, value, len);
    if(err){
        log_e("nvs_set_blob fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

typename KVStoreInterface<const char*>::res_t ESP32KVStore::getBytes(const key_t& key, uint8_t buf[], size_t maxLen) const {
    size_t len = getBytesLength(key);
    if(!len || !buf || !maxLen){
        return len;
    }
    if(len > maxLen){
        log_e("not enough space in buffer: %u < %u", maxLen, len);
        return 0;
    }
    esp_err_t err = nvs_get_blob(_handle, key, buf, &len);
    if(err){
        log_e("nvs_get_blob fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t ESP32KVStore::getBytesLength(const key_t& key) const {
    size_t len = 0;
    if(!_started || !key){
        return 0;
    }
    esp_err_t err = nvs_get_blob(_handle, key, NULL, &len);
    if(err){
        log_e("nvs_get_blob len fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}


bool ESP32KVStore::exists(const key_t& key) const {
    return getType(key) != PT_INVALID;
}

ESP32KVStore::Type ESP32KVStore::getType(const key_t& key) const {
    if(!_started || !key || strlen(key)>15){
        return PT_INVALID;
    }
    int8_t mt1; uint8_t mt2; int16_t mt3; uint16_t mt4;
    int32_t mt5; uint32_t mt6; int64_t mt7; uint64_t mt8;
    size_t len = 0;
    if(nvs_get_i8(_handle, key, &mt1) == ESP_OK) return PT_I8;
    if(nvs_get_u8(_handle, key, &mt2) == ESP_OK) return PT_U8;
    if(nvs_get_i16(_handle, key, &mt3) == ESP_OK) return PT_I16;
    if(nvs_get_u16(_handle, key, &mt4) == ESP_OK) return PT_U16;
    if(nvs_get_i32(_handle, key, &mt5) == ESP_OK) return PT_I32;
    if(nvs_get_u32(_handle, key, &mt6) == ESP_OK) return PT_U32;
    if(nvs_get_i64(_handle, key, &mt7) == ESP_OK) return PT_I64;
    if(nvs_get_u64(_handle, key, &mt8) == ESP_OK) return PT_U64;
    if(nvs_get_str(_handle, key, NULL, &len) == ESP_OK) return PT_STR;
    if(nvs_get_blob(_handle, key, NULL, &len) == ESP_OK) return PT_BLOB;
    return PT_INVALID;
}

// specialization of put and get, when esp nvs treats them differently
template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<int8_t>(const key_t& key, int8_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_i8(_handle, key, value);
    if(err){
        log_e("nvs_set_i8 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 1;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<uint8_t>(const key_t& key, uint8_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_u8(_handle, key, value);
    if(err){
        log_e("nvs_set_u8 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 1;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<int16_t>(const key_t& key, int16_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_i16(_handle, key, value);
    if(err){
        log_e("nvs_set_i16 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 2;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<uint16_t>(const key_t& key, uint16_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_u16(_handle, key, value);
    if(err){
        log_e("nvs_set_u16 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 2;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<int32_t>(const key_t& key, int32_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_i32(_handle, key, value);
    if(err){
        log_e("nvs_set_i32 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 4;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<uint32_t>(const key_t& key, uint32_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_u32(_handle, key, value);
    if(err){
        log_e("nvs_set_u32 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 4;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<int64_t>(const key_t& key, int64_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_i64(_handle, key, value);
    if(err){
        log_e("nvs_set_i64 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 8;
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<uint64_t>(const key_t& key, uint64_t value) {
    if(!_started || !key || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_u64(_handle, key, value);
    if(err){
        log_e("nvs_set_u64 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 8;
}

size_t ESP32KVStore::putString(key_t key, const char* value) {
    if(!_started || !key || !value || _readOnly){
        return 0;
    }
    esp_err_t err = nvs_set_str(_handle, key, value);

    if(err){
        log_e("nvs_set_str fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(_handle);
    if(err){
        log_e("nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return strlen(value);
}

size_t ESP32KVStore::putString(key_t key, String value) {
    return putString(key, value.c_str());
}

template<>
typename KVStoreInterface<const char*>::res_t ESP32KVStore::put<const char*>(const key_t& key, const char* value) {
    return putString(key, value);
}

template<>
typename KVStoreInterface<string>::res_t ESP32KVStore::put<string>(const key_t& key, string value) {
    return putString(key, value.c_str());
}

template<>
typename KVStoreInterface<String>::res_t ESP32KVStore::put<String>(const key_t& key, String value) {
    return putString(key, value.c_str());
}

template<>
KVStoreInterface<const char*>::reference<int8_t> ESP32KVStore::get<int8_t>(const key_t& key, int8_t defaultValue) {
    int8_t value = defaultValue;
    if(!_started || !key){
        return reference<int8_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_i8(_handle, key, &value);
    if(err){
        log_v("nvs_get_i8 fail: %s %s", key, nvs_error(err));
    }
    return reference<int8_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<uint8_t> ESP32KVStore::get<uint8_t>(const key_t& key, uint8_t defaultValue) {
    uint8_t value = defaultValue;
    if(!_started || !key){
        return reference<uint8_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_u8(_handle, key, &value);
    if(err){
        log_v("nvs_get_u8 fail: %s %s", key, nvs_error(err));
    }
    return reference<uint8_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<int16_t> ESP32KVStore::get<int16_t>(const key_t& key, int16_t defaultValue) {
    int16_t value = defaultValue;
    if(!_started || !key){
        return reference<int16_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_i16(_handle, key, &value);
    if(err){
        log_v("nvs_get_i16 fail: %s %s", key, nvs_error(err));
    }
    return reference<int16_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<uint16_t> ESP32KVStore::get<uint16_t>(const key_t& key, uint16_t defaultValue) {
    uint16_t value = defaultValue;
    if(!_started || !key){
        return reference<uint16_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_u16(_handle, key, &value);
    if(err){
        log_v("nvs_get_u16 fail: %s %s", key, nvs_error(err));
    }
    return reference<uint16_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<int32_t> ESP32KVStore::get<int32_t>(const key_t& key, int32_t defaultValue) {
    int32_t value = defaultValue;
    if(!_started || !key){
        return reference<int32_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_i32(_handle, key, &value);
    if(err){
        log_v("nvs_get_i32 fail: %s %s", key, nvs_error(err));
    }
    return reference<int32_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<uint32_t> ESP32KVStore::get<uint32_t>(const key_t& key, uint32_t defaultValue) {
    uint32_t value = defaultValue;
    if(!_started || !key){
        return reference<uint32_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_u32(_handle, key, &value);
    if(err){
        log_v("nvs_get_u32 fail: %s %s", key, nvs_error(err));
    }
    return reference<uint32_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<int64_t> ESP32KVStore::get<int64_t>(const key_t& key, int64_t defaultValue) {
    int64_t value = defaultValue;
    if(!_started || !key){
        return reference<int64_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_i64(_handle, key, &value);
    if(err){
        log_v("nvs_get_i64 fail: %s %s", key, nvs_error(err));
    }
    return reference<int64_t>(key, value, *this);
}

template<>
KVStoreInterface<const char*>::reference<uint64_t> ESP32KVStore::get<uint64_t>(const key_t& key, uint64_t defaultValue) {
    uint64_t value = defaultValue;
    if(!_started || !key){
        return reference<uint64_t>(key, value, *this);
    }
    esp_err_t err = nvs_get_u64(_handle, key, &value);
    if(err){
        log_v("nvs_get_u64 fail: %s %s", key, nvs_error(err));
    }
    return reference<uint64_t>(key, value, *this);
}

// KVStoreInterface<const char*>::reference<const char*> ESP32KVStore::get<const char*>(const key_t& key, const char* defaultValue) {
    // size_t len = 0;
    // Serial.println(len);

    // if(!_started || !key){
    //     return reference<const char*>(key, defaultValue, *this);;
    // }
    // esp_err_t err = nvs_get_str(_handle, key, NULL, &len);
    // Serial.println(len);

    // if(err){
    //     log_e("nvs_get_str len fail: %s %s", key, nvs_error(err));
    //     return reference<const char*>(key, defaultValue, *this);;
    // }

    // char* result = new char[len + 1];
    // Serial.println(len);
    // err = nvs_get_str(_handle, key, result, &len);
    // if(err){
    //     log_e("nvs_get_str fail: %s %s", key, nvs_error(err));
    //     return reference<const char*>(key, defaultValue, *this);;
    // }

    // result[len] = '\0';
    // return reference<const char*>(key, result, *this, [](const char* v) {
    //     delete v;
    // });
// }

size_t ESP32KVStore::getString(const char* key, char* value, size_t maxLen) {
    size_t len = 0;
    if(!_started || !key || !value || !maxLen){
        return 0;
    }

    esp_err_t err = nvs_get_str(_handle, key, NULL, &len);
    if(err){
        log_e("nvs_get_str len fail: %s %s", key, nvs_error(err));
        return 0;
    }
    if(len > maxLen){
        log_e("not enough space in value: %u < %u", maxLen, len);
        return 0;
    }
    err = nvs_get_str(_handle, key, value, &len);
    if(err){
        log_e("nvs_get_str fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

String ESP32KVStore::getString(key_t key, const String defaultValue) {
    size_t len = 0;
    String res = defaultValue;
    if(!_started || !key){
        return res;
    }

    esp_err_t err = nvs_get_str(_handle, key, NULL, &len);
    if(err){
        log_e("nvs_get_str len fail: %s %s", key, nvs_error(err));
        return res;
    }

    char *str = new char[len+1];

    getString(key, str, len+1);
    str[len] = '\0';

    res = str;
    delete str;
    str = nullptr;

    return res;
}


// template<>
// KVStoreInterface<const char*>::reference<string> ESP32KVStore::get<string>(const key_t& key, string defaultValue) {
    // char * value = NULL;
    // size_t len = 0;
    // if(!_started || !key){
    //     return string(defaultValue);
    // }
    // esp_err_t err = nvs_get_str(_handle, key, value, &len);
    // if(err){
    //     log_e("nvs_get_str len fail: %s %s", key, nvs_error(err));
    //     return string(defaultValue);
    // }
    // char buf[len];
    // value = buf;
    // err = nvs_get_str(_handle, key, value, &len);
    // if(err){
    //     log_e("nvs_get_str fail: %s %s", key, nvs_error(err));
    //     return string(defaultValue);
    // }
    // return string(buf);
// }

// template<>
// KVStoreInterface<const char*>::reference<String> ESP32KVStore::get<String>(const key_t& key, String defaultValue) {
    // char * value = NULL;
    // size_t len = 0;
    // if(!_started || !key){
    //     return String(defaultValue);
    // }
    // esp_err_t err = nvs_get_str(_handle, key, value, &len);
    // if(err){
    //     log_e("nvs_get_str len fail: %s %s", key, nvs_error(err));
    //     return String(defaultValue);
    // }
    // char buf[len];
    // value = buf;
    // err = nvs_get_str(_handle, key, value, &len);
    // if(err){
    //     log_e("nvs_get_str fail: %s %s", key, nvs_error(err));
    //     return String(defaultValue);
    // }
    // return String(buf);
// }

#endif // defined(ARDUINO_ARCH_ESP32)
