#include "GPSClient.h"

void GPSClient::begin() {
    _location = {0, 0, 0, false};
}

void GPSClient::update() {
    tryConnectBLE();
}

void GPSClient::tryConnectBLE() {
    // TODO: Implement BLE GATT client for iOS location
    // iOS doesn't expose location via standard BLE GATT.
    // Requires a companion iOS app or specific BLE service.
    // Practical fallback: server's /api/gps endpoint with IP geolocation.
}

void GPSClient::parseLocationData(const uint8_t* data, size_t len) {
    // TODO: Parse BLE characteristic data into Location struct
    // Expected format TBD based on BLE service specification
    if (len >= sizeof(double) * 2 + sizeof(float)) {
        double lat, lng;
        float alt;
        memcpy(&lat, data, sizeof(double));
        memcpy(&lng, data + sizeof(double), sizeof(double));
        memcpy(&alt, data + sizeof(double) * 2, sizeof(float));
        _location = {lat, lng, alt, true};
    }
}
