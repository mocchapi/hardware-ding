#include <env.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>

const char *WIFI_TAG = "[WIFI] ";

void wifi_setup() {

  Serial.print(WIFI_TAG);
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print(WIFI_TAG);
  Serial.println("connected");
  Serial.print(WIFI_TAG);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
