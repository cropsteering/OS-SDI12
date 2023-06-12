/**
 * @file config.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __mqtt_config_H__
#define __mqtt_config_H__

#include <Arduino.h>

/** Turn on/off MQTT debug output*/
#define MQTT_DEBUG 1
/** Keep wifi/MQTT alive*/
const uint16_t KEEP_ALIVE = 120;
/** WiFi credentials */
const char* SSID = "";
const char* PASSWORD = "";
/** MQTT credentials */
const char* MQTT_SERVER =  "";
const uint16_t MQTT_PORT = 8883;
const char* MQTT_ID = "";
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
const String ZONE_NAME = "Zone1";

/** Secure client cert */
const char* server_root_ca = \
    "-----BEGIN CERTIFICATE-----\n" \

    "-----END CERTIFICATE-----\n";

#endif
