/**
 * @file logger.h
 * @author Jamie Howse (r4wknet@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __logger_H__
#define __logger_H__

/**
 * @brief LOGGER Lib
 * 
 */
class LOGGER
{
    public:
    void logger_setup();
    void write_sd(String data);
};

/** Overloads for logic */
extern bool use_log;
extern int32_t gmtoffset_sec;
extern uint32_t daylightoffset_sec;

#endif
