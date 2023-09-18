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

# Hardware needed

You'll want a RAK baseboard and RAK11200 core
- [RAK19007+RAK11200 KIT](https://rakwireless.kckb.st/57a05b8f) (Make sure to pick 19007+11200 in the drop down)
- [BASE BOARDS](https://rakwireless.kckb.st/e0a81f2e)
- [RAK11200](https://rakwireless.kckb.st/797d9c85)

And then you will want the SDI-12 Module: RAK13010
- [RAK13010](https://rakwireless.kckb.st/21a4637e)

If you want to use the [SD card module](https://www.adafruit.com/product/4682), you will want to use the DUAL IO board ( [RAK19001](https://rakwireless.kckb.st/e5bcf28c) )

# How to flash
1. https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11200/Quickstart/#install-platformio
2. Clone this repo to a local folder
3. Open cloned folder in VSCode+PlatformIO (from above)
4. Press the PlatformIO: Upload button (or however you want to build/flash)

# Support
If you want to support, use one of the refferal links above to purchase your RAK hardware. OR just use the refferal code
- [RAK Wireless Store](https://rakwireless.kckb.st/ace5fdc3) 8% off code: WGC279
  
You won't see the discount until you check out
