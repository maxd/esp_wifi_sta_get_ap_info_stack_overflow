//
// Created by Maxim Dobryakov on 14/09/2020.
//

#ifndef AGV_REMOTE_CONTROL_WIFIMANAGER_H
#define AGV_REMOTE_CONTROL_WIFIMANAGER_H


#include <atomic>
#include <functional>
#include <string>
#include <esp_supplicant/esp_wps.h>
#include <esp_event_base.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

class WiFiManager {
private:
    const char *LOG_TAG = "WiFiManager";

public:
    std::atomic_bool isWiFiClientStarted = ATOMIC_VAR_INIT(false);
    std::atomic_bool isWiFiConnectionEstablished = ATOMIC_VAR_INIT(false);

    WiFiManager();

    void startWiFiClient();
    void stopWiFiClient();

    void connect();
    void connect(const std::string &ssid, const std::string &password);
    void disconnect();

private:
    static void wiFiEventHandlerWrapper(void* eventHandlerArg,
                                 esp_event_base_t eventBase,
                                 int32_t eventId,
                                 void* eventData);

    void wiFiEventHandler(esp_event_base_t eventBase, int32_t eventId, void *eventData);

    TimerHandle_t strangeMeasurementsTimer = nullptr;

    void startStrangeMeasurements();
    void stopStrangeMeasurements();
    static void strangeMeasurementsTimerHandler(TimerHandle_t timer);
};


#endif //AGV_REMOTE_CONTROL_WIFIMANAGER_H
