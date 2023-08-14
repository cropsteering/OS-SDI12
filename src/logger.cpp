/**
 * @file logger.cpp
 * @author Jamie Howse (r4wknet@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <Arduino.h>
#include <logger.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <vector>
#include <sstream>

/** Configurage switch */
bool use_sd = true;
/** Logic switch */
bool use_log = false;
/** Card found switch */
bool card_found = false;
/** Time server */
String ntp_server = "pool.ntp.org";
/** Time zone offset */
int16_t gmtoffset_sec = -12600;
/** Daylight savings time offset */
uint16_t daylightoffset_sec = 3600;

/** Turn on/off LOGGER debug output*/
#define LOGGER_DEBUG 1

/** File instance to hold log */
File r4k_file;

void setup_sd();
void setup_rtc();
String get_timestamp();
String parse_data_sd(String data);
void LOGGER_LOG(String chan, String data);

/**
 * @brief Setup logger
 * 
 */
void LOGGER::logger_setup() 
{
  setup_rtc();
  setup_sd();
}

/**
 * @brief Setup SD card
 * 
 */
void setup_sd()
{
  if(use_sd)
  {
    LOGGER_LOG("LOG", "SD begin");
    if (!SD.begin()) 
    {
      LOGGER_LOG("LOG", "SD init failed");
      card_found = false;
    } else {
      LOGGER_LOG("LOG", "SD init success");
      card_found = true;
    }
  }
}

/**
 * @brief Write to SD card
 * 
 * @param data String to write to SD log file
 */
void LOGGER::write_sd(String data)
{
  if(use_sd && card_found && use_log)
  {
    r4k_file = SD.open("/sdi12log.txt", FILE_APPEND);
    data = parse_data_sd(data);

    if(r4k_file)
    {
      r4k_file.println(get_timestamp() + " " + data);
      LOGGER_LOG("LOG", "Wrote " + get_timestamp() + " " + data);
      r4k_file.close();
    } else {
      LOGGER_LOG("LOG", "Could not open log file");
    }
  }
}

/**
 * @brief Set up real time clock
 * 
 */
void setup_rtc()
{
  struct tm timeinfo;
  configTime(gmtoffset_sec, daylightoffset_sec, ntp_server.c_str());
  if(!getLocalTime(&timeinfo))
  {
    LOGGER_LOG("LOG", "Failed to obtain time");
    LOGGER_LOG("LOG", "Using defaults");
  } else {
    LOGGER_LOG("LOG", "Got time from " + String(ntp_server));
  }
}

/**
 * @brief Get the timestamp object
 * 
 * @return String 
 */
String get_timestamp()
{
  struct tm timeinfo;
  String timestamp;
  if(!getLocalTime(&timeinfo))
  {
    LOGGER_LOG("LOG", "Failed to obtain time");
  } else {
    char time_c[100];
    strftime(time_c, 50, "%D %T", &timeinfo);
    timestamp = String(time_c);
  }

  return timestamp;
}

/**
 * @brief Parse incoming string to csv
 * 
 * @param data 
 */
String parse_data_sd(String data)
{
  String log_data;
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
          log_data += String(seglist[x].c_str());
      } else {
          log_data += String(seglist[x].c_str()) + ", ";
      }
  }

  return log_data;
}

/**
 * @brief Debug output text
 * 
 * @param chan Output channel
 * @param data String to output
 */
void LOGGER_LOG(String chan, String data)
{
    #if LOGGER_DEBUG
    String disp = "["+chan+"] " + data;
    Serial.println(disp);
    #endif
}
