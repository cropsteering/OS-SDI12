# OpenSteering-SDI12 - ALPHA
SDI-12 Data logger, up to 62 addresses

# MQTT Downlink Config commands

        /** CMD 0: CSV */
        Turn CSV output on/off
        example: 0+[TRUE/FALSE], 0+TRUE
        saved to flash
        
        /** CMD 1: Sleep period */
        Set sleep period (or time between readings), in seconds
        example: 1+[SECONDS], 1+15
        saved to flash
        
        /** CMD 2: Change SDI-12 address */
        Change SDI-12 sensor address
        example: 2+[CURRENT ADDRESS]+[NEW ADDRESS], 2+7+A
        
        /** CMD 3: Add sensor data set */
        Add how many data sets a sensor has
        example: 3+[SENSOR ID]+[#] (# is zero indexed), 3+12345+2
        saved to flash
        
        /** CMD 4: Use SD card */
        Turn on or off SD card logging completely
        example: 4+[TRUE/FALSE], 4+FALSE
        saved to flash

        /** CMD 5: Change GMT/DST offset */
        Set timezone and DST
        example: 5+[GMT OFFSET]+[DST OFFSET], 5+-12600+3600
        saved to flash

You can send these via MQTT downlink to the following sub
  
    MQTT_USER/MQTT_ID/config
    example: r4wk/test/config

This is all set in the mqtt_config.h

# How to flash
https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11200/Quickstart/#install-platformio

# Hardware needed

Go to [RAK WIRELESS STORE](https://rakwireless.kckb.st/f4cc11c3)
Promo code: CLFWTP 3% off

You'll want a RAK baseboard (only one that will not work is the mini, RAK19003): RAK19007, RAK19001
If you want to use the [SD card module](https://www.adafruit.com/product/4682), you will want to pick the DUAL IO board (RAK19001)

And then you will want this RAK MCU: RAK11200

And then you will want the SDI-12 Module: RAK13010
