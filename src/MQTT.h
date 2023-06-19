/**
 * @file MQTT.h
 * @author Jamie Howse (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __MQTT_H__
#define __MQTT_H__

/**
 * @brief MQTT Lib
 * 
 */
class MQTT
{
    public:
    void mqtt_setup();
    void mqtt_loop();
    void mqtt_publish(String addr, String data);
};

extern uint32_t PERIOD;
void chng_addr(String addr_old, String addr_new);

#endif
