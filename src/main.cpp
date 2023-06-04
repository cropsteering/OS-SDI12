/**
 * @file main.cpp
 * @author Jamie Howse (r4wknet@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <main.h>

/** Instance of SDI-12 lib */
RAK_SDI12 sdi12_bus(RX_PIN, TX_PIN, OE);
/** Vector array to cache online sensor addresses */
std::vector<std::string> addr_cache;
/** Timer to poll online sensors */
Ticker poll_sensor_ticker;
/** Number of online sensors */
uint8_t num_sensors;
/** SSL/TLS WiFi client */
WiFiClientSecure secure_client;
/** MQTT client */
PubSubClient mqtt_client(secure_client);
/** Bool to trigger sensor reading */
bool trigger_read = false;
/** Failed SDI-12 read count */
uint8_t retry_count = 0;
/** Convert #define username to string */
String s_convert(MQTT_USER);
/** Further convert to std::string */
std::string std_convert(s_convert.c_str());

/**
 * @brief Setup firmware
 * Begin SDI-12 bus
 * Cache all online sensors
 * Start sensor poll timer
 * 
 */
void setup() 
{
    pinMode(WB_IO2, OUTPUT);
    digitalWrite(WB_IO2, HIGH);

    Serial.begin(115200);
    time_t timeout = millis();
    while (!Serial)
    {
        if ((millis() - timeout) < 5000)
        {
            delay(100);
        } else {
            break;
        }
    }

    wifi_connect();
    mqtt_setup();
    mqtt_connect();

    R_LOG("SDI-12", "Starting bus");
    sdi12_bus.begin();
    delay(500);

    cache_online();

    poll_sensor_ticker.once(wait_time, ticker_trigger);
}

/**
 * @brief Do nothing
 * 
 */
void loop() 
{
    /** Always check MQTT connection */
    if(!mqtt_client.connected()) { mqtt_connect(); }
    mqtt_client.loop();
    if(trigger_read) { sdi_measure(); }
}

/**
 * @brief Prepare SDI-12 sensor for reading
 * SDI-12 measurement command format [address]["M"][!]
 * Get number of responses from each SDI-12 sensor
 * 
 * @param addr address of SDI-12 sensor
 */
void sdi_measure()
{
    R_LOG("SDI-12", "Starting measure");
    sdi12_bus.clearBuffer();
    std::map<std::string, uint8_t> resp_map;
    uint32_t read_total = 0;

    for(int x = 0; x < num_sensors; x++)
    {
        std::string sdi_command;
        sdi_command = "";
        sdi_command += addr_cache[x];
        sdi_command += "M";
        sdi_command += "!";

        sdi12_bus.sendCommand(sdi_command.c_str());
        delay(100);
        String sdi_response = sdi12_bus.readStringUntil('\n');
        sdi_response.trim();

        uint8_t num_resp = sdi_response.substring(4).toInt();
        uint8_t read_time = sdi_response.substring(1, 4).toInt();
        if(read_time == 0) { read_time = 1; }

        resp_map.insert(std::make_pair(addr_cache[x], num_resp));
        read_total += read_time;
    }

    /** Added 1 second padding, may not be needed */
    uint32_t read_ms = (read_total * 1000) + 1000;
    time_t timeout_sdi = millis() + read_ms;
    R_LOG("SDI-12", "Pausing for: " + std::to_string(read_ms/1000));
    /** Wait for all sensors to finish */
    while(millis() < timeout_sdi);

    std::map<std::string, uint8_t>::iterator it = resp_map.begin();
    while (it != resp_map.end())
    {
        get_data(it->first, it->second);
        ++it;
    }
    trigger_read = false;
}

/**
 * @brief Get SDI-12 sensor data
 * SDI-12 command to get data [address][D][dataOption][!]
 * 
 * @param addr the SDI-12 sensor address
 * @param num_resp number of responses from SDI-12 sensor
 */
void get_data(std::string addr, uint8_t num_resp)
{
    R_LOG("SDI-12", "Start data read");
    uint8_t resp_count;
    std::string mqtt_csv;

    std::string sdi_command;
    sdi_command = "";
    sdi_command += addr;
    sdi_command += "D0";
    sdi_command += "!";

    sdi12_bus.sendCommand(sdi_command.c_str());
    delay(100);
    sdi12_bus.read();
    char c = sdi12_bus.peek();
    if(c == '+') { sdi12_bus.read(); }

    while(sdi12_bus.available())
    {
        char c = sdi12_bus.peek();
        if (c == '-' || (c >= '0' && c <= '9') || c == '.')
        {
            float sdi_data = sdi12_bus.parseFloat(SKIP_NONE);
            if(sdi_data != -9999) 
            { 
                if(resp_count != 0)
                {
                    if(resp_count < num_resp) 
                    { 
                        mqtt_csv += std::to_string(sdi_data) + ", "; 
                    } else { 
                        mqtt_csv += std::to_string(sdi_data);
                    }
                }
                resp_count++; 
            }
        } else if(c == '+') {
            sdi12_bus.read();
        } else {
            sdi12_bus.read();
        }
        delay(10);
    }

    if(resp_count-1 == num_resp)
    {
        std::string mqtt_sub = std_convert + "/" + zone_name + "/" + addr;
        sdi12_bus.clearBuffer();

        if(mqtt_client.connected()) 
        { 
            if(mqtt_client.publish(mqtt_sub.c_str(), mqtt_csv.c_str()))
            {
                R_LOG("MQTT", "Publish");
                R_LOG("MQTT", "Topic: " + mqtt_sub);
                R_LOG("MQTT", "Data: " + mqtt_csv);
            } else {
                mqtt_connect(); 
            }
        } else {
            mqtt_connect(); 
        }
        poll_sensor_ticker.once(wait_time, ticker_trigger);
    } else {
        R_LOG("SDI-12", "Incorrect data, restarting");
        if(retry_count < 5)
        {
            get_data(addr, num_resp);
            retry_count++;
        } else {
            R_LOG("SDI-12", "Too many retries, dropping data set");
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
        if(is_online(std::to_string(x)))
        {
            R_LOG("SDI-12", "Address cached " + std::to_string(x));
            addr_cache.push_back(std::to_string(x));
        }
    }

    // for (char y = 'a'; y <= 'z'; y++)
    // {
    //     std::ostringstream ss;
    //     ss << y;
    //     if(is_online(ss.str()))
    //     {
    //         R_LOG("SDI-12", "Address cached" + std::to_string(y));
    //         addr_cache.push_back(std::to_string(y));
    //     }
    // }

    // for (char z = 'A'; z <= 'Z'; z++)
    // {
    //     std::ostringstream ss;
    //     ss << z;
    //     if(is_online(ss.str()))
    //     {
    //         R_LOG("SDI-12", "Address cached" + std::to_string(z));
    //         addr_cache.push_back(std::to_string(z));
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
bool is_online(std::string addr)
{
    bool found = false;

    for(int x = 0; x < 3; x++)
    {
        std::string sdi_command;
        sdi_command = "";
        sdi_command += addr;
        sdi_command += "!";
        sdi12_bus.sendCommand(sdi_command.c_str());
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
 * @brief Connect to Wifi
 * Use cert and secure client for SSL/TLS
 * 
 */
void wifi_connect()
{
    delay(10);

    std::string disp = "[WiFi] Connecting to ";
    Serial.println(disp.c_str());
    Serial.print(SSID);

    WiFi.setHostname("SDI-12_data_logger");
    WiFi.begin(SSID, PASSWORD);

    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    secure_client.setTimeout(KEEP_ALIVE);
    secure_client.setCACert(server_root_ca);
}

/**
 * @brief Setup MQTT server
 * Add MQTT downlink call back
 * 
 */
void mqtt_setup()
{
    mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt_client.setKeepAlive(KEEP_ALIVE);
    mqtt_client.setSocketTimeout(KEEP_ALIVE);
    mqtt_client.setCallback(mqtt_downlink);
}

/**
 * @brief Connect to MQTT server
 * 
 */
void mqtt_connect()
{
    while(!mqtt_client.connected() && WiFi.status() == WL_CONNECTED)
    {
        R_LOG("MQTT", "Connecting to broker");
        if(mqtt_client.connect(MQTT_ID, MQTT_USER, MQTT_PASS))
        {
            R_LOG("MQTT", "Connected to broker");
        } else {
            R_LOG("MQTT", "Error code: " + std::to_string(mqtt_client.state()));
            delay(5000);
        }
    }
    if(WiFi.status() != WL_CONNECTED) { wifi_connect(); }
}

/**
 * @brief Call back for Ticker to trigger reading
 * 
 */
void ticker_trigger() { trigger_read = true; }

/**
 * @brief 
 * 
 * @param topic 
 * @param message 
 * @param length 
 * 
 */
void mqtt_downlink(char* topic, byte* message, unsigned int length)
{
    R_LOG("MQTT", "MQTT downlink recieved");
}
