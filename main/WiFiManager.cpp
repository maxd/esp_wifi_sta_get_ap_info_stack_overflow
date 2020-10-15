//
// Created by Maxim Dobryakov on 14/09/2020.
//

#include "WiFiManager.h"

#include <cstring>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <esp_supplicant/esp_wps.h>

WiFiManager::WiFiManager() {
    ESP_LOGI(LOG_TAG, "Initialization of Wi-Fi system");

    ESP_ERROR_CHECK(esp_netif_init());
}

void WiFiManager::startWiFiClient() {
    assert(!isWiFiClientStarted);

    ESP_LOGI(LOG_TAG, "Start STA Wi-Fi client");

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    assert(esp_netif_create_default_wifi_sta());

    wifi_init_config_t wiFiInitConfig = WIFI_INIT_CONFIG_DEFAULT()
    wiFiInitConfig.nvs_enable = 0; //TODO: disable NVS for simplify debug
    ESP_ERROR_CHECK(esp_wifi_init(&wiFiInitConfig));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wiFiEventHandlerWrapper, this));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void WiFiManager::stopWiFiClient() {
    assert(isWiFiClientStarted);

    ESP_LOGI(LOG_TAG, "Stop STA Wi-Fi client");

    ESP_ERROR_CHECK(esp_wifi_stop());

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wiFiEventHandlerWrapper));

    ESP_ERROR_CHECK(esp_wifi_deinit());

    ESP_ERROR_CHECK(esp_event_loop_delete_default());
}

void WiFiManager::connect() {
    assert(isWiFiClientStarted);
    assert(!isWiFiConnectionEstablished);

    ESP_LOGI(LOG_TAG, "Connect to Wi-Fi (use automatically saved to flash memory credentials)");

    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WiFiManager::connect(const std::string &ssid, const std::string &password) {
    assert(isWiFiClientStarted);
    assert(!isWiFiConnectionEstablished);

    ESP_LOGI(LOG_TAG, "Connect to Wi-Fi with SSID and password");

    wifi_config_t wifiConfig = {};
    strlcpy((char *)wifiConfig.sta.ssid, ssid.c_str(), sizeof(wifiConfig.sta.ssid));
    strlcpy((char *)wifiConfig.sta.password, password.c_str(), sizeof(wifiConfig.sta.password));
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiConfig.sta.pmf_cfg.capable = true;
    wifiConfig.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void WiFiManager::disconnect() {
    assert(isWiFiClientStarted);
    assert(isWiFiConnectionEstablished);

    ESP_LOGI(LOG_TAG, "Disconnect from Wi-Fi");

    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

void WiFiManager::wiFiEventHandlerWrapper(void* eventHandlerArg,
                                          esp_event_base_t eventBase,
                                          int32_t eventId,
                                          void* eventData) {
    static_cast<WiFiManager *>(eventHandlerArg)->wiFiEventHandler(eventBase, eventId, eventData);
}

void WiFiManager::wiFiEventHandler(esp_event_base_t eventBase,
                                   int32_t eventId,
                                   void* eventData) {
    switch (eventId) {
        case WIFI_EVENT_STA_START: {
            ESP_LOGD(LOG_TAG, "STA start");
            isWiFiClientStarted = true;
            break;
        }
        case WIFI_EVENT_STA_STOP: {
            ESP_LOGD(LOG_TAG, "STA stop");
            isWiFiClientStarted = false;
            break;
        }
        case WIFI_EVENT_STA_CONNECTED: {
            ESP_LOGD(LOG_TAG, "Connected to AP");
            isWiFiConnectionEstablished = true;
            startStrangeMeasurements();
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED: {
            auto *event = (wifi_event_sta_disconnected_t *)eventData;
            ESP_LOGW(LOG_TAG, "Disconnected from AP. Reason: %d", event->reason);
            isWiFiConnectionEstablished = false;
            stopStrangeMeasurements();
            break;
        }
        default: {
            ESP_LOGI(LOG_TAG, "Unhandled event id: %d", eventId);
            break;
        }
    }
}

void WiFiManager::startStrangeMeasurements() {
    if (strangeMeasurementsTimer != nullptr) {
        ESP_LOGW(LOG_TAG, "Wi-Fi strange measurement timer already started.");
        return;
    }

    strangeMeasurementsTimer = xTimerCreate(
            "strange-measurements-timer",
            pdMS_TO_TICKS(1000), //TODO: fix magic constant
            pdTRUE,
            nullptr,
            strangeMeasurementsTimerHandler);

    if (xTimerStart(strangeMeasurementsTimer, 0) == pdFALSE) {
        ESP_LOGE(LOG_TAG, "Can't start Wi-Fi strange measurement timer.");
        abort();
    }
}

void WiFiManager::stopStrangeMeasurements() {
    if (strangeMeasurementsTimer == nullptr) {
        ESP_LOGW(LOG_TAG, "Wi-Fi strange measurement timer already stopped.");
        return;
    }

    if (xTimerStop(strangeMeasurementsTimer, 0) == pdFALSE) {
        ESP_LOGE(LOG_TAG, "Can't stop Wi-Fi strange measurement timer.");
        abort();
    }

    if (xTimerDelete(strangeMeasurementsTimer, 0) == pdFALSE) {
        ESP_LOGE(LOG_TAG, "Can't delete Wi-Fi strange measurement timer.");
        abort();
    }

    strangeMeasurementsTimer = nullptr;
}

void WiFiManager::strangeMeasurementsTimerHandler(TimerHandle_t timer) {
    void *p0 = nullptr;
    ESP_LOGW("STACK POSITION", "p0: %p", (void *)&p0);

    wifi_ap_record_t wifiApRecord = {};

    void *p1 = nullptr;
    ESP_LOGW("STACK POSITION", "p1: %p", (void *)&p1);

    esp_err_t result = esp_wifi_sta_get_ap_info(&wifiApRecord);

    void *p2 = nullptr;
    ESP_LOGW("STACK POSITION", "p2: %p", (void *)&p2);

    ESP_LOGI("LOG_TAG", "Wi-Fi RSSI: %d", wifiApRecord.rssi);
}
