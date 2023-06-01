/**
 * @file main.h
 * @author Jamie Howse (r4wknet@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/** Includes here */
#include <Arduino.h>
#include <Wire.h>
#include <RAK13010_SDI12.h>
#include <vector>
#include <sstream>
#include <Ticker.h>
#include <WiFi.h>
#include <map>
#include <iostream>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

/** Pin setup */
#define TX_PIN WB_IO6 // SDI-12 data bus, TX
#define RX_PIN WB_IO5 // SDI-12 data bus, RX
#define OE WB_IO4	  // Output enable
#define DEBUG 1

/** Forward declatration */
void cache_online();
bool is_online(std::string addr);
void concurrent_measure();
void get_data(std::string addr, uint8_t num_resp, uint8_t read_time);
void wifi_connect();
void mqtt_setup();
void mqtt_connect();
void mqtt_downlink(char* topic, byte* message, unsigned int length);

/** WiFi credentials */
#define SSID ""
#define PASSWORD ""

/** MQTT credentials */
#define MQTT_SERVER ""
#define MQTT_PORT 8883
#define MQTT_ID ""
#define MQTT_USER ""
#define MQTT_PASS ""

/** Logger zone name */
std::string zone_name = "Zone1";
/** Default poll time is 15 seconds */
uint16_t wait_time = 15;

#define KEEP_ALIVE 120

/** Secure client cert */
const char* server_root_ca = \
    "-----BEGIN CERTIFICATE-----\n" \

    "-----END CERTIFICATE-----\n";

/**
 * @brief 
 * 
 * @param chan 
 * @param data 
 */
void R_LOG(std::string chan, std::string data)
{
    #if DEBUG
    std::string disp = "["+chan+"] " + data;
    Serial.println(disp.c_str());
    #endif
}
