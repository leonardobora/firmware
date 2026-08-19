#pragma once

#include <Arduino.h>

struct Location {
    double latitude;
    double longitude;
    float altitude;
    bool valid;
};

class GPSClient {
public:
    void begin();
    void update();
    Location location() const { return _location; }

private:
    Location _location = {0, 0, 0, false};
    void tryConnectBLE();
    void parseLocationData(const uint8_t* data, size_t len);
};
