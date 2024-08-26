#include <Arduino_KVStore.h>


void setup() {
    Serial.begin(115200);

    while(!Serial);
    delay(5000);
    KVStore kvstore("pippo");

    kvstore.begin();

    uint32_t value = 0x01020304;

    Serial.println(kvstore.putUInt("0", value));
    Serial.println(kvstore.getUInt("0"), HEX);
    kvstore.remove("0");

    Serial.println(kvstore.putBytes("0", (uint8_t*)&value, sizeof(value)));
    // Serial.println(kvstore.getbytes("0"), HEX);
    kvstore.remove("0");
}

void loop() {

}
