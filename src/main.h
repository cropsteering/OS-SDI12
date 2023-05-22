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

/** Pin setup */
#define TX_PIN WB_IO6 // SDI-12 data bus, TX
#define RX_PIN WB_IO5 // SDI-12 data bus, RX
#define OE WB_IO4	  // Output enable

/** Forward declatration */
void cache_online();
bool is_online(std::string addr);
void concurrent_measure();
void get_data(std::string addr, uint8_t num_resp);
void connect_wifi();

/** WiFi credentials */
#define SSID "YOUR_SSID"
#define PASSWORD "YOUR_PASSWORD"