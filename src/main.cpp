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
/** Default poll time is 15 seconds */
uint16_t wait_time = 15;
/** SSL/TLS WiFi client */
WiFiClientSecure secure_client;
/** MQTT client */
PubSubClient mqtt_client(secure_client);

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
    poll_sensor_ticker.once(wait_time, concurrent_measure);
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
}

/**
 * @brief Measure all SDI-12 sensors at once, takes 5 seconds
 * SDI-12 concurrent measurement command format [address]["C"][!]
 * Get number of responses from each SDI-12 sensor
 * 
 * @param addr address of SDI-12 sensor
 */
void concurrent_measure()
{
    sdi12_bus.clearBuffer();
    std::map<std::string, uint8_t> resp_map;

    for(int x = 0; x < num_sensors; x++)
    {
        std::string sdi_command;
        sdi_command = "";
        sdi_command += addr_cache[x];
        sdi_command += "C";
        sdi_command += "!";

        sdi12_bus.sendCommand(sdi_command.c_str());
        String sdi_response = sdi12_bus.readStringUntil('\n');
        sdi_response.trim();
        uint8_t num_resp = sdi_response.substring(4).toInt();

        resp_map.emplace(addr_cache[x], num_resp);
    }

    /** Wait 5+ seconds for all readings to finish
     * TODO: remove delay, handle this properly
     */
    delay(10000);

    std::map<std::string, uint8_t>::iterator it = resp_map.begin();
    while (it != resp_map.end())
    {
        get_data(it->first, it->second);
        ++it;
    }
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
    uint8_t resp_count;
    std::string mqtt_csv;

    while(resp_count < num_resp)
    {
        std::string sdi_command;
        sdi_command = "";
        sdi_command += addr;
        sdi_command += "D0";
        sdi_command += "!";

        sdi12_bus.sendCommand(sdi_command.c_str());

        uint32_t start = millis();
        while (sdi12_bus.available() < 3 && (millis() - start) < 1500);
        sdi12_bus.read();
        char c = sdi12_bus.peek();

        if(c == '+')
        {
            sdi12_bus.read();
        }

        while(sdi12_bus.available())
        {
            char c = sdi12_bus.peek();
            if (c == '-' || (c >= '0' && c <= '9') || c == '.')
            {
                float sdi_data = sdi12_bus.parseFloat(SKIP_NONE);
                if(sdi_data != -9999) { resp_count++; }
                if(resp_count < num_resp) 
                { 
                    mqtt_csv += std::to_string(sdi_data) + ", "; 
                } else { 
                    mqtt_csv += std::to_string(sdi_data);
                }
            } else if(c == '+') {
                sdi12_bus.read();
            } else {
                sdi12_bus.read();
            }
            delay(10);
        }
        delay(100);
    }

    sdi12_bus.clearBuffer();

    if(mqtt_client.connected()) 
    { 
        std::string mqtt_sub = get_model(addr) + "/" + addr;
        mqtt_client.publish(mqtt_sub.c_str(), mqtt_csv.c_str());
        R_LOG("MQTT", "Publish");
        R_LOG("MQTT", mqtt_sub);
        R_LOG("MQTT", mqtt_csv);
    }

    poll_sensor_ticker.once(wait_time, concurrent_measure);
}

/**
 * @brief Get the model object
 * 
 * @param addr SDI-12 sensor address
 * @return std::string Manufacturer sensor model
 */
std::string get_model(std::string addr)
{
    std::string sdi_command;
    sdi_command = "";
    sdi_command += addr;
    sdi_command += "I!";

    sdi12_bus.sendCommand(sdi_command.c_str());
    delay(100);

    String sdi_response = sdi12_bus.readStringUntil('\n');
    sdi_response.trim();

    std::string s(sdi_response.substring(11, 17).c_str());
    
    return s;
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

    for (char y = 'a'; y <= 'z'; y++)
    {
        std::ostringstream ss;
        ss << y;
        if(is_online(ss.str()))
        {
            R_LOG("SDI-12", "Address cached" + std::to_string(y));
            addr_cache.push_back(std::to_string(y));
        }
    }

    for (char z = 'A'; z <= 'Z'; z++)
    {
        std::ostringstream ss;
        ss << z;
        if(is_online(ss.str()))
        {
            R_LOG("SDI-12", "Address cached" + std::to_string(z));
            addr_cache.push_back(std::to_string(z));
        }
    }

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
    mqtt_client.setCallback(mqtt_downlink);
}

/**
 * @brief Connect to MQTT server
 * 
 */
void mqtt_connect()
{
    while(!mqtt_client.connected())
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
}

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
