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
#include <RAK13010_SDI12.h>
#include <vector>
#include <map>

/** Pin setup */
#define TX_PIN WB_IO6 // SDI-12 data bus, TX
#define RX_PIN WB_IO5 // SDI-12 data bus, RX
#define OE WB_IO4	  // Output enable

#define DEBUG 1

RAK_SDI12 sdi12_bus(RX_PIN,TX_PIN,OE);

uint32_t PERIOD = 15000000;
bool sdi_ready = true;

uint8_t num_sensors;
std::vector<String> addr_cache;
std::map<String, uint16_t> data_set;
std::map<uint16_t, uint16_t> ds_lookup;

/** Forward declaration */
void R_LOG(String chan, String data);
bool is_online(String addr);
void cache_online();
void sdi_measure();

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

    ds_lookup.insert({22667, 0});

    R_LOG("SDI-12", "Starting SDI-12 bus");
    sdi12_bus.begin();
    delay(500);

    sdi12_bus.forceListen();
    cache_online();
}

void loop() 
{
    static uint32_t last_time;

    if (micros() - last_time >= PERIOD && sdi_ready)
    {
        last_time += PERIOD;
        sdi_measure();
    }

}

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
            sdi12_bus.clearBuffer();
        }
    }
    
    sdi_ready = true;   
}

/**
 * @brief Cache all online SDI-12 sensor addresses
 * 
 */
void cache_online()
{
    for(int x = 0; x <= 9; x++)
    {
        if(is_online(String(x)))
        {
            static String sdi_response;
            R_LOG("SDI-12", "Address cached " + String(x));
            addr_cache.push_back(String(x));

            sdi_response = "";
            sdi12_bus.sendCommand(String(x) + "I!");
            R_LOG("SDI-12", "Sent: " + String(x) + "I!");
            delay(100);

            sdi_response = sdi12_bus.readString();
            sdi_response.trim();
            uint16_t sensor_id = sdi_response.substring(20).toInt();
            R_LOG("SDI-12", "Reply: " + String(sensor_id));
            data_set.insert({String(x), ds_lookup[sensor_id]});
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
}

/**
 * @brief Sends a basic ack command and if it gets a reply
 * the SDI-12 sensor at that address is confirmed online
 * 
 * Sends basic acknowledge command [address][!]
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
 * @brief 
 * 
 * @param chan 
 * @param data 
 */
void R_LOG(String chan, String data)
{
    #if DEBUG
    String disp = "["+chan+"] " + data;
    Serial.println(disp);
    #endif
}
