#include <env.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>

const char *MQTT_TAG = "[MQTT] ";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

void mqtt_setup()
{
    Serial.print(MQTT_TAG);
    Serial.print("Attempting to connect to the broker: ");
    Serial.println(MQTT_BROKER);

    if (!mqttClient.connect(MQTT_BROKER, MQTT_PORT))
    {
        Serial.print(MQTT_TAG);
        Serial.print("connection failed! Error code = ");
        Serial.println(mqttClient.connectError());

        while (1)
            ;
    }

    Serial.print(MQTT_TAG);
    Serial.println("Connected");
}

void mqtt_loop()
{
    mqttClient.poll();
}

void mqtt_send(String topic, String payload)
{
    Serial.print(MQTT_TAG);
    Serial.println("Sending mqtt");
    mqttClient.beginMessage(topic);
    mqttClient.print(payload);
    mqttClient.endMessage();
}