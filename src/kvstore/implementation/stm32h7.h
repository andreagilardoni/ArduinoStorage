#pragma once
#include <KVStore.h>
#include <TDBStore.h>
#include <Arduino_DebugUtils.h>
#include "QSPIFBlockDevice.h"
#include "MBRBlockDevice.h"

class STM32H7KVStore: public KVStoreInterface<const char*> {
public:
    STM32H7KVStore(mbed::KVStore* store = nullptr);
    ~STM32H7KVStore() {
        if (bd != nullptr) {
            delete kvstore;
            kvstore = nullptr;
            delete bd;
            bd = nullptr;
        }
    }

    bool begin() override;
    bool end() override;
    bool clear() override;

    typename KVStoreInterface<const char*>::res_t remove(const key_t& key) override;
    bool exists(const key_t& key) const override;
    typename KVStoreInterface<const char*>::res_t putBytes(const key_t& key, uint8_t b[], size_t s) override;
    typename KVStoreInterface<const char*>::res_t getBytes(const key_t& key, uint8_t b[], size_t s) const override;
    size_t getBytesLength(const key_t& key) const override;
private:
    mbed::MBRBlockDevice* bd;
    mbed::KVStore* kvstore;
};

STM32H7KVStore::STM32H7KVStore(mbed::KVStore* store): kvstore(store), bd(nullptr) {
    if(kvstore == nullptr) {
        auto root = mbed::BlockDevice::get_default_instance();

        if (root->init() != QSPIF_BD_ERROR_OK) {
            Serial.println(F("Error: QSPI init failure."));
            return;
        }
        bd = new mbed::MBRBlockDevice(root, 4);

        if(bd->get_partition_start() == bd->get_partition_stop()) {
            // resize partition
            Serial.println(F("Resizing partition"));
            auto res = mbed::MBRBlockDevice::partition(root, 4, 0x0B, 14 * 1024 * 1024, 14 * 1024 * 1024 + 512 * 1024); // 14 MB - end
        }

        kvstore = new mbed::TDBStore(bd);
    }
}

bool STM32H7KVStore::begin() {
    return kvstore->init() == MBED_SUCCESS;
}

bool STM32H7KVStore::end() {
    return kvstore->deinit() == MBED_SUCCESS;
}

bool STM32H7KVStore::clear() {
    return kvstore->reset();
}

typename KVStoreInterface<const char*>::res_t STM32H7KVStore::remove(const key_t& key) {
    return kvstore->remove(key);
}

typename KVStoreInterface<const char*>::res_t STM32H7KVStore::putBytes(const key_t& key, uint8_t buf[], size_t len) {
    return kvstore->set(key, buf, len, 0); // TODO flags
}

typename KVStoreInterface<const char*>::res_t STM32H7KVStore::getBytes(const key_t& key, uint8_t buf[], size_t maxLen) const {
    return kvstore->get(key, buf, maxLen);
}

size_t STM32H7KVStore::getBytesLength(const key_t& key) const {
    mbed::KVStore::info_t info;
    auto res = kvstore->get_info(key, &info);

    return res == MBED_SUCCESS ? info.size : 0;
}

bool STM32H7KVStore::exists(const key_t& key) const {
    return getBytesLength(key) > 0;
}
