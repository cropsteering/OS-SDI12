/**
 * @file MQTT.cpp
 * @author Jamie Howse (r4wknet@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <Arduino.h>
#include <MQTT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <mqtt_config.h>
#include <vector>
#include <sstream>

/** SSL/TLS WiFi client */
WiFiClientSecure secure_client;
/** MQTT client */
PubSubClient mqtt_client(secure_client);
/** Use CSV or individual readings */
bool CSV = true;
/** Give up on trying to connect */
bool give_up = false;
/** Retry time for WiFi/MQTT */
uint64_t connect_time;

/** Forward declaration */
void wifi_connect();
String parse_data(String data);
void mqtt_connect();
void mqtt_downlink(char* topic, byte* message, unsigned int length);
void MQTT_LOG(String chan, String data);
void parse_config(String data);

/**
 * @brief Connect to WiFi and setup MQTT
 * 
 */
void MQTT::mqtt_setup()
{
    wifi_connect();
    mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt_client.setKeepAlive(KEEP_ALIVE);
    mqtt_client.setSocketTimeout(KEEP_ALIVE);
    mqtt_client.setCallback(mqtt_downlink);
    connect_time = 3600000000;
}

/**
 * @brief MQTT Loop
 * 
 */
void MQTT::mqtt_loop()
{
    /** Always check MQTT connection */
    if(!give_up)
    {
        if(!mqtt_client.connected()) { mqtt_connect(); }
        mqtt_client.loop();
    } else {
        static uint32_t last_time;
        if ((micros() - last_time) >= connect_time)
        {
            last_time += connect_time;
            wifi_connect();
        }
    }
}

/**
 * @brief Public CSV to MQTT
 * 
 * @param data 
 */
void MQTT::mqtt_publish(String addr, String data)
{
    if(mqtt_client.connected()) 
    {
        String mqtt_data = parse_data(data);
        String mqtt_topic = String(MQTT_USER) + "/" + ZONE_NAME + "/" + addr;
        if(CSV)
        {
            if(mqtt_client.publish(mqtt_topic.c_str(), mqtt_data.c_str()))
            {
                MQTT_LOG("MQTT", "Publish CSV");
                MQTT_LOG("MQTT", mqtt_topic);
                MQTT_LOG("MQTT", mqtt_data);
            }
        } else {
            std::stringstream ss(data.c_str());
            std::string segment;
            char value = 'a';
            while(std::getline(ss, segment, '+'))
            {
                mqtt_topic = String(MQTT_USER) + "/" + ZONE_NAME + "/" + addr + "/" + String(value++);
                if(mqtt_client.publish(mqtt_topic.c_str(), segment.c_str()))
                {
                    MQTT_LOG("MQTT", "Publish SEGMENT");
                    MQTT_LOG("MQTT", mqtt_topic);
                    MQTT_LOG("MQTT", String(segment.c_str()));
                } 
            }
        }
    }
}

/**
 * @brief Parse incoming string to csv
 * 
 * @param data 
 */
String parse_data(String data)
{
    String mqtt_data;
    std::stringstream ss(data.c_str());
    std::string segment;
    std::vector<std::string> seglist;
    while(std::getline(ss, segment, '+'))
    {
        seglist.push_back(segment);
    }

    uint16_t size = seglist.size();
    for(int x = 0; x < size; x++)
    {
        if (x == size-1) {
            mqtt_data += String(seglist[x].c_str());
        } else {
            mqtt_data += String(seglist[x].c_str()) + ",";
        }
    }

    return mqtt_data;
}

/**
 * @brief Connect to Wifi
 * Use cert and secure client for SSL/TLS
 * 
 */
void wifi_connect()
{
    uint8_t wifi_retry = 0;
    delay(10);

    MQTT_LOG("WiFi", "Connecting to " + String(SSID));

    WiFi.setHostname("SDI-12_data_logger");
    WiFi.begin(SSID, PASSWORD);

    while(WiFi.status() != WL_CONNECTED)
    {
        wifi_retry++;
        if(wifi_retry == 10) 
        { 
            Serial.println(" Failed to connect to WiFi");
            give_up = true;
            break; 
        }
        MQTT_LOG("WiFi", "Retrying");
        delay(500);
    }

    if(WiFi.status() == WL_CONNECTED)
    {
        MQTT_LOG("WiFi", "Connected");
        MQTT_LOG("WiFi", "IP address: " + String(WiFi.localIP().toString()));
        secure_client.setTimeout(KEEP_ALIVE);
        secure_client.setCACert(server_root_ca);
        give_up = false;
    }
}

/**
 * @brief Connect to MQTT server
 * 
 */
void mqtt_connect()
{
    uint8_t mqtt_retry = 0;
    while(!mqtt_client.connected() && WiFi.status() == WL_CONNECTED)
    {
        MQTT_LOG("MQTT", "Connecting to broker");
        if(mqtt_client.connect(MQTT_ID, MQTT_USER, MQTT_PASS))
        {
            MQTT_LOG("MQTT", "Connected to broker");
            mqtt_client.subscribe(MQTT_CONFIG.c_str());
            give_up = false;
        } else {
            MQTT_LOG("MQTT", "Error code: " + String(mqtt_client.state()));
            mqtt_retry++;
            if(mqtt_retry == 10)
            {
                MQTT_LOG("MQTT", "Failed to connect to MQTT");
                give_up = true;
                break; 
            }
            delay(5000);
        }
    }
}

/**
 * @brief MQTT Downlink handler
 * 
 * @param topic 
 * @param message 
 * @param length 
 * 
 */
void mqtt_downlink(char* topic, byte* message, unsigned int length)
{
    String topic_string = String(topic);
    String mqtt_data;
    if(topic_string == MQTT_CONFIG)
    {
        for(int x = 0; x < length; x++)
        {
            mqtt_data += (char)message[x];
        }
        parse_config(mqtt_data);
    } else {
        MQTT_LOG("MQTT", "MQTT downlink recieved");
    }
}

/**
 * @brief Parse incoming MQTT data for config changes
 * 
 * @param data 
 */
void parse_config(String data)
{
    String config_data;
    std::stringstream ss(data.c_str());
    std::string segment;
    std::vector<std::string> seglist;
    while(std::getline(ss, segment, '+'))
    {
        seglist.push_back(segment);
    }

    u_int16_t cmd_int = stoi(seglist[0]);
    switch(cmd_int)
    {
        /** CMD 0: CSV */
        case 0:
            if(seglist[1] == "true")
            {
                CSV = true;
                MQTT_LOG("MQTT", "CSV set to true");
            } else {
                CSV = false;
                MQTT_LOG("MQTT", "CSV set to false");
            }
            flash_bool("csv", CSV, false);
        break;
        /** CMD 1: Sleep period */
        case 1:
            delay_time = stoi(seglist[1])*1000000;
            flash_64u("period", delay_time, false);
            MQTT_LOG("MQTT", "Delay set to " + String(seglist[1].c_str()));
        break;
        /** CMD 2: Change SDI-12 address */
        case 2:
            chng_addr(seglist[1].c_str(), seglist[2].c_str());
        break;
        /** CMD 3: Add sensor data set */
        case 3:
            flash_32u(seglist[1].c_str(), stoi(seglist[2]), true);
        break;
        /** CMD 4: Use SD card */
        case 4:
            if(seglist[1] == "true")
            {
                use_sd = true;
                MQTT_LOG("SD", "Set to true");
            } else {
                use_sd = false;
                MQTT_LOG("SD", "Set to false");
            }
            flash_bool("sd", use_sd, false);
            ESP.restart();
        break;
        /** CMD 5: Change GMT/DST offset */
        case 5:
            flash_32("gmt", stoi(seglist[1]), false);
            flash_32u("dst", stoi(seglist[2]), false);
            ESP.restart();
        break;
    }
}

/**
 * @brief Debug output text
 * 
 * @param chan Output channel
 * @param data String to output
 */
void MQTT_LOG(String chan, String data)
{
    #if MQTT_DEBUG
    String disp = "["+chan+"] " + data;
    Serial.println(disp);
    #endif
}
