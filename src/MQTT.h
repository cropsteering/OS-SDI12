/**
 * @file MQTT.h
 * @author Jamie Howse (r4wknet@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __MQTT_H__
#define __MQTT_H__

#include <map>

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

/** Overloads for config */
extern uint64_t delay_time;
extern bool CSV;
extern bool give_up;
extern bool use_sd;
void chng_addr(String addr_old, String addr_new);
void flash_32(const char* key, uint32_t value, bool restart);
void flash_64(const char* key, uint64_t value, bool restart);
void flash_bool(const char* key, bool value, bool restart);

#endif
