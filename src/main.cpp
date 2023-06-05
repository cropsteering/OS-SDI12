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

/** Forward declaration */
void R_LOG(String chan, String data);
bool is_online(String addr);
void cache_online();

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

    R_LOG("SDI-12", "Opening SDI-12 bus.");
    sdi12_bus.begin();
    delay(500);

    sdi12_bus.forceListen();
    cache_online();
}

void loop() 
{
    static uint8_t sdi_flag = 0;
    static String  sdi_reply;
    static boolean reply_ready = false;

    static uint32_t last_time;
    if (micros() - last_time >= PERIOD && sdi_ready)
    {
        last_time += PERIOD;
        sdi_ready = false;
        sdi_flag = 1;
    }

    int avail = sdi12_bus.available();
    if (avail < 0) 
    {
        sdi12_bus.clearBuffer();  // Buffer is full,clear.
    } else if (avail > 0) {
        for (int a = 0; a < avail; a++) 
        {
            char inByte2 = sdi12_bus.read();
            if (inByte2 == '\n') 
            {
                reply_ready = true;
            } 
            else {
                sdi_reply += String(inByte2);
            }
        }
    }

    if (reply_ready)
    {
        R_LOG("SDI-12", "Reply: "+ sdi_reply);
        reply_ready = false;  // Reset String for next SDI-12 message.
        sdi_reply   = "";
        if(sdi_flag == 2) { sdi_flag = 3; }
    }

    if (sdi_flag)
    {
        if(sdi_flag == 1)
        {
            sdi_flag = 2;
            for(int x = 0; x < num_sensors; x++)
            {
                sdi12_bus.sendCommand(addr_cache[x] + "M!");
                R_LOG("SDI-12", "Sent: " + addr_cache[x] + "M!");
            }
        }
        if(sdi_flag == 3)
        {
            sdi_flag = 0;
            delay(2000*num_sensors);
            for(int x = 0; x < num_sensors; x++)
            {
                sdi12_bus.sendCommand(addr_cache[x] + "D0!");
                R_LOG("SDI-12", "Sent: " + addr_cache[x] + "D0!");
                sdi12_bus.clearBuffer();
                sdi_ready = true;
            }
        }
    }
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
            R_LOG("SDI-12", "Address cached " + String(x));
            addr_cache.push_back(String(x));
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
