/**
 * @file main.cpp
 * @author Jamie Howse (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <Arduino.h>
#include <RAK13010_SDI12.h>
#include <MQTT.h>
#include <vector>
#include <map>

/** Pin setup 
 * SDI-12 data bus, TX
 * SDI-12 data bus, RX
 * Output enable
*/
#define TX_PIN WB_IO6
#define RX_PIN WB_IO5
#define OE     WB_IO4
/** Turn on/off debug output */
#define DEBUG 1

/** RAK SDI-12 Lib */
RAK_SDI12 sdi12_bus(RX_PIN,TX_PIN,OE);
/** MQTT Lib */
MQTT mqtt_lib;
/** Wait period between sensor readings */
uint32_t PERIOD = 15000000;
/** Is the SDI-12 bus ready */
bool sdi_ready = true;
/** Number of online sensors */
uint8_t num_sensors;
/** Cache of online sensor addresses */
std::vector<String> addr_cache;
/** Lookup for current sensors data set i.e D0-D9 */
std::map<String, uint16_t> data_set;
/** Lookup of known sensor IDs and their data set count */
std::map<uint16_t, uint16_t> ds_lookup;

/** Forward declaration */
void R_LOG(String chan, String data);
bool is_online(String addr);
void cache_online();
void sdi_measure();
void set_lookup(String addr);

/**
 * @brief Setup firmware
 * 
 */
void setup()
{
    pinMode(WB_IO2, OUTPUT);
    digitalWrite(WB_IO2, HIGH); 

    time_t timeout = millis();
    Serial.begin(115200);
    while (!Serial)
    {
        if ((millis() - timeout) < 5000)
        {
            delay(100);
        } else {
            break;
        }
    }

    /** Join WiFi and connect to MQTT */
    mqtt_lib.mqtt_setup();

    /** Add known sensors to lookup array */
    /** Acclima TDR-310W */
    ds_lookup.insert({22667, 0});

    R_LOG("SDI-12", "Starting SDI-12 bus");
    sdi12_bus.begin();
    delay(500);

    sdi12_bus.forceListen();
    cache_online();
}

/**
 * @brief Firmwares main loop
 * Run periodic measure
 * 
 */
void loop() 
{
    /** Loop our MQTT lib */
    mqtt_lib.mqtt_loop();
    /** Measure every X seconds if SDI-12 bus is ready */
    static uint32_t last_time;
    if (micros() - last_time >= PERIOD && sdi_ready)
    {
        last_time += PERIOD;
        sdi_measure();
    }
}

/**
 * @brief Poll SDI-12 sensor for data
 * Measure: [a]M! - prepares sensor for reading and returns reading info
 * Data: [a]D[0-9]! - Ask sensor for all data values
 * 
 */
void sdi_measure()
{
    sdi_ready = false;
    static String sdi_response;
    for(int x = 0; x < num_sensors; x++)
    {
        sdi_response = "";
        sdi12_bus.sendCommand(addr_cache[x] + "M!");
        R_LOG("SDI-12", "Sent: " + addr_cache[x] + "M!");
        delay(100);

        sdi_response = sdi12_bus.readString();
        sdi_response.trim();
        R_LOG("SDI-12", "Reply: " + sdi_response);
        /** Added 1 second padding */
        uint8_t wait = sdi_response.substring(1, 4).toInt();
        uint32_t wait_ms = (wait+1)*1000;
        R_LOG("SDI-12", "Pausing for: " + String(wait+1));
        delay(wait_ms);

        uint16_t ds_amt = data_set[addr_cache[x]];
        String mqtt_splice;

        for(int y = 0; y <= ds_amt; y++)
        {
            sdi_response = "";
            sdi12_bus.sendCommand(addr_cache[x] + "D" + String(y) + "!");
            R_LOG("SDI-12", "Sent: " + addr_cache[x] + "D" + String(y) + "!");
            delay(100);

            /** Drop first byte, repeated address*/
            if(y == 0) { sdi12_bus.read(); }
            sdi_response = sdi12_bus.readString();
            sdi_response.trim();
            R_LOG("SDI-12", "Reply: " + sdi_response);
            if(y != ds_amt)
            {
                mqtt_splice += sdi_response + "+";
            } else {
                 mqtt_splice += sdi_response;
            }
            sdi12_bus.clearBuffer();
        }
        mqtt_lib.mqtt_publish(addr_cache[x], mqtt_splice);
    }
    sdi_ready = true;   
}

/**
 * @brief Cache all online SDI-12 sensor addresses
 * Info: [a]I! - Returns information about the sensor
 * 
 */
void cache_online()
{
    sdi_ready = false;
    for(int x = 0; x <= 9; x++)
    {
        if(is_online(String(x)))
        {
            R_LOG("SDI-12", "Address cached " + String(x));
            addr_cache.push_back(String(x));
            set_lookup(String(x));
        }
    }

    // for (char y = 'a'; y <= 'z'; y++)
    // {
    //     if(is_online(String(y)))
    //     {
    //         R_LOG("SDI-12", "Address cached" + String(y));
    //         addr_cache.push_back(String(y));
    //     }
    // }

    // for (char z = 'A'; z <= 'Z'; z++)
    // {
    //     if(is_online(String(z)))
    //     {
    //         R_LOG("SDI-12", "Address cached" + String(z));
    //         addr_cache.push_back(String(z));
    //     }
    // }

    num_sensors = addr_cache.size();
    sdi_ready = true; 
}

/**
 * @brief Sends a basic ack command and if it gets a reply
 * the SDI-12 sensor at that address is confirmed online
 * 
 * Sends basic acknowledge command [a][!]
 * 
 * @param addr 
 * @return true SDI-12 sensor found
 * @return false SDI-12 not sensor found
 */
bool is_online(String addr)
{
    bool found = false;

    for(int x = 0; x < 3; x++)
    {
        String sdi_command;
        sdi_command = "";
        sdi_command += addr;
        sdi_command += "!";
        sdi12_bus.sendCommand(sdi_command);
        delay(100);

        uint16_t avail = sdi12_bus.available();

        if(avail > 0)
        {
            R_LOG("SDI-12", "Sensor found on " + addr);
            sdi12_bus.clearBuffer();
            found = true;
            return true;
        } else {
            R_LOG("SDI-12", "No sensor found on " + addr);
            sdi12_bus.clearBuffer();
            found = false;
        }
    }

    return found;
}

/**
 * @brief Poll sensor for data set info
 * 
 * @param addr 
 */
void set_lookup(String addr)
{
    static String sdi_response;
    sdi_response = "";
    sdi12_bus.sendCommand(addr + "I!");
    R_LOG("SDI-12", "Sent: " + addr + "I!");
    delay(100);

    sdi_response = sdi12_bus.readString();
    sdi_response.trim();
    uint16_t sensor_id = sdi_response.substring(20).toInt();
    R_LOG("SDI-12", "Reply: " + String(sensor_id));
    data_set.insert({addr, ds_lookup[sensor_id]});
}

/**
 * @brief 
 * 
 * @param chan Output channel
 * @param data String to output
 */
void R_LOG(String chan, String data)
{
    #if DEBUG
    String disp = "["+chan+"] " + data;
    Serial.println(disp);
    #endif
}
