#include <string>
#include "WiFiManager.h"

extern "C" int app_main();

int app_main() {
    std::string ssid = "???";
    std::string password = "???";

    auto *wiFiManager = new WiFiManager();

    wiFiManager->startWiFiClient();
    while(!wiFiManager->isWiFiClientStarted)
        vTaskDelay(pdMS_TO_TICKS(500));

    wiFiManager->connect(ssid, password);
    while(!wiFiManager->isWiFiConnectionEstablished)
        vTaskDelay(pdMS_TO_TICKS(500));

    vTaskDelay(pdMS_TO_TICKS(3600 * 1000)); // wait 1 hour

    wiFiManager->disconnect();
    while(wiFiManager->isWiFiConnectionEstablished)
        vTaskDelay(pdMS_TO_TICKS(500));

    wiFiManager->stopWiFiClient();
    while(wiFiManager->isWiFiClientStarted)
        vTaskDelay(pdMS_TO_TICKS(500));

    return 0;
}
