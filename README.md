# R4K-SDI12 - ALPHA
SDI-12 Data logger, up to 62 addresses

# MQTT Downlink Config commands

        /** CMD 0: CSV */
        Turn CSV output on/off
        example: 0+[TRUE/FALSE], 0+TRUE
        
        /** CMD 1: Sleep period */
        Set sleep period (or time between readings), in seconds
        example: 1+[SECONDS], 1+15
        
        /** CMD 2: Change SDI-12 address */
        Change SDI-12 sensor address
        example: 2+[CURRENT ADDRESS]+[NEW ADDRESS], 2+7+A
        
        /** CMD 3: Add sensor data set */
        Add how many data sets a sensor has
        example: 3+[SENSOR ID]+[#] (# is zero indexed), 3+12345+2
        
        /** CMD 4: Use SD card */
        Turn on or off SD card logging completely
        example: 4+[TRUE/FALSE], 4+FALSE

        /** CMD 5: Change GMT/DST offset */
        Set timezone and DST
        example: 5+[GMT OFFSET]+[DST OFFSET], 5+-12600+3600

You can send these via MQTT downlink to the following sub
  
    MQTT_USER/MQTT_ID/config
    example: r4wk/test/config

This is all set in the mqtt_config.h

# How to flash
https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11200/Quickstart/#install-platformio
