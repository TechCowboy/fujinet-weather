/**
 * Weather / forecast.c
s *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#include <conio.h>
#include "constants.h"
#include "options.h"
#include "location.h"
#include "screen.h"
#include "io.h"
#include "sprite.h"
#include "direction.h"
#include "sprite.h"
#include "ftime.h"
#include "input.h"
#include "forecast.h"
#include "screen.h"
#include "utils.h"
#include "weather.h"

ForecastData forecastData;
unsigned char forecast_offset = 0;

extern unsigned short timer;
extern bool forceRefresh;

extern OptionsData optData;
extern Location locData;
extern char timezone[48];
extern long timezone_offset;;
extern unsigned long dt;

char request[512];

/*** FORECAST ***/


/*
forecast_open 
- Open the url containing the json data for the forecastd 

Returns
    true: Success
   false: Could not open the website
 
*/

#ifdef USE_METEO

// https://api.open-meteo.com/v1/forecast?latitude=46.2618&longitude=-63.6726&daily=weather_code,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_sum,precipitation_probability_max,wind_speed_10m_max,wind_direction_10m_dominant&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timeformat=unixtime&forecast_days=16
// https://api.open-meteo.com/v1/forecast?latitude=46.2618&longitude=-63.6726&daily=weather_code,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_probability_max,precipitation_sum,wind_speed_10m_max,wind_direction_10m_dominant&timeformat=unixtime&forecast_days=16&timezone=AST

bool forecast_open(void) // METEO
{
    Timestamp ts;
    char url[MAX_URL];
    char units[80];

    get_timezone_from_position(timezone, sizeof(timezone), locData.latitude, locData.longitude);

    if (optData.units == METRIC) 
      strncpy2(units, "", sizeof(units));
    else
      strncpy2(units, "temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&", sizeof(units));

    snprintf(url, sizeof(url),
             "N:HTTP://" ME_API "//v1//forecast?"
             "latitude=%s&"
             "longitude=%s&"
             "daily=weather_code,temperature_2m_max,temperature_2m_min,uv_index_max,precipitation_probability_max,precipitation_sum,wind_speed_10m_max,wind_direction_10m_dominant&"
             "%s"
             "timeformat=unixtime&"
             "forecast_days=16&"
             "timezone=%s",
             locData.latitude, locData.longitude, units, timezone);

    if (io_json_open(url) == 0)
        return false;
    else
        return true;
}

#else

// HTTP://api.openweathermap.org/data/2.5/onecall?lat=44.62335968017578&lon=-63.57278060913086&exclude=minutely,hourly,alerts,current&units=%s&appid=2e8616654c548c26bc1c86b1615ef7f1

bool forecast_open(void) // OPEN_WEATHER
{
    Timestamp ts;
    char url[256];
    char units[10];

    if (optData.units == METRIC)
        strncpy2(units, "metric", sizeof(units));
    else if (optData.units == IMPERIAL)
        strncpy2(units, "imperial", sizeof(units));

    snprintf(url, sizeof(url), "N:HTTP://%s//data/2.5/onecall?lat=%s&lon=%s&exclude=minutely,hourly,alerts,current&units=%s&appid=%s", 
                OW_API, locData.latitude, locData.longitude, units,
                OW_KEY);

    if (io_json_open(url) == 0)
        return false;
    else
        return true;
}

#endif


bool forecast_close(void)
{
    io_json_close();

    return true;
}

/*
Collect the i-th forecast and parse the collected json 
and storing the information in the ForecastData structure
*/

#ifdef USE_METEO

void forecast_parse(unsigned char i, ForecastData *f) // METEO
{
    Timestamp ts;
    char prefix[20];
    char postfix[20];
    char json_part[30];
    char wmo[10];
    long forecast_time;
    /*
    {
      "latitude": 46.26653,
      "longitude": -63.662872,
      "generationtime_ms": 0.351905822753906,
      "utc_offset_seconds": -14400,
      "timezone": "America/Halifax",
      "timezone_abbreviation": "AST",
      "elevation": 18,
      "daily_units": {
        "time": "unixtime",
        "weather_code": "wmo code",
        "temperature_2m_max": "°C",
        "temperature_2m_min": "°C",
        "uv_index_max": "",
        "precipitation_probability_max": "%",
        "precipitation_sum": "mm",
        "wind_speed_10m_max": "km/h",
        "wind_direction_10m_dominant": "°"
      },
      "daily": {
        "time": [1731470400, 1731556800, 1731643200, 1731729600, 1731816000, 1731902400, 1731988800, 1732075200, 1732161600, 1732248000, 1732334400, 1732420800, 1732507200, 1732593600, 1732680000,
    1732766400], "weather_code": [51, 3, 75, 61, 51, 3, 3, 3, 3, 53, 3, 3, 55, 51, 3, 3], "temperature_2m_max": [2.1, 1.8, 8.3, 8.4, 6.9, 6.2, 4.5, 5.1, 7.7, 9.9, 7.4, 5.2, 7.4, 7.6, 2.4, 1.1],
        "temperature_2m_min": [-0.4, -0.3, 1.8, 6.9, 4.5, 3.5, 3.1, 2.7, 3, 6.3, 4.3, 3.4, 2.9, 2.7, -0.2, -1],
        "uv_index_max": [2.45, 2.3, 0.4, 0.3, 0.55, 2, 0.85, 1.7, 0.5, 1.75, 1.2, 1.85, 1.95, 1.95, 1.4, 1],
        "precipitation_probability_max": [38, 53, 98, 87, 67, 42, 42, 28, 36, 36, 29, 29, 35, 28, 19, 23],
        "precipitation_sum": [0.1, 0, 11.1, 8.4, 0.6, 0, 0, 0, 0, 4.5, 0, 0, 6.6, 0.3, 0, null],
        "wind_speed_10m_max": [37.8, 37.3, 48, 35.8, 34.2, 22.5, 32.3, 28.2, 46.2, 61.5, 19.7, 22.7, 39, 45.8, 50.1, 51.2],
        "wind_direction_10m_dominant": [344, 356, 359, 289, 302, 287, 314, 315, 93, 136, 252, 295, 153, 272, 270, null]
      }
    }
    */

    io_json_query("/utc_offset_seconds", json_part, sizeof(json_part));
    timezone_offset = atol(json_part);

    snprintf(prefix, sizeof(prefix), "%s", "/daily/");
    snprintf(postfix, sizeof(postfix), "/%c", '0' + i);

    snprintf(request, sizeof(request), "%sweather_code%s", prefix, postfix);

    io_json_query(request, wmo, sizeof(wmo));

    snprintf(request, sizeof(request), "%stime%s", prefix,postfix);
    io_json_query(request, json_part, sizeof(json_part));

    forecast_time = atol(json_part);

    // change it to local time
    forecast_time += timezone_offset;
    timestamp((unsigned long) forecast_time, &ts);

    snprintf(f->date, 8, "%d %s", ts.day, time_month(ts.month));
    snprintf(f->dow, 4, "%s", time_dow(ts.dow));

    get_description(wmo, f->desc);

    f->icon = get_sprite(wmo);

    snprintf(request, sizeof(request), "%stemperature_2m_min%s", prefix, postfix);
    io_json_query(request, f->lo, sizeof(f->lo));
    io_decimals(f->lo, optData.maxPrecision);
    strcat(f->lo, optData.units == IMPERIAL ? "F" : "C");

        snprintf(request, sizeof(request), "%stemperature_2m_max%s", prefix, postfix);
    io_json_query(request, f->hi, sizeof(f->hi));
    io_decimals(f->hi, optData.maxPrecision);
    strcat(f->hi, optData.units == IMPERIAL ? "F" : "C");

    //snprintf(request, sizeof(request), "%spressure", prefix);
    //io_json_query(request, f->pressure, sizeof(f->pressure));
    strncpy2(f->pressure, "", sizeof(f->pressure));

    snprintf(request, sizeof(request), "%swind_speed_10m_max%s", prefix, postfix);
    io_json_query(request, json_part, sizeof(json_part));
    io_decimals(json_part, optData.maxPrecision);
    snprintf(f->wind, sizeof(f->wind), "WIND:%s%s ", json_part, optData.units == IMPERIAL ? "mph" : "kph");

    snprintf(request, sizeof(request), "%swind_direction_10m_dominant%s", prefix, postfix);
    io_json_query(request, json_part, sizeof(json_part));
    strcat(f->wind, degToDirection(atoi(json_part)));

    snprintf(request, sizeof(request), "%sprecipitation_probability_max%s", prefix, postfix);
    io_json_query(request, json_part, sizeof(json_part));
    snprintf(f->pop, sizeof(f->pop), "POP:%s%%", json_part);

    snprintf(request, sizeof(request), "%sprecipitation_sum%s", prefix, postfix);
    io_json_query(request, json_part, sizeof(json_part));
    if (strcmp(json_part, "0") != 0)
      snprintf(f->rain, sizeof(f->rain), "%s%s", json_part, optData.units == IMPERIAL ? "in" : "mm");
    else
      strncpy2(f->rain, "", sizeof(f->rain));

    // snprintf(request,  sizeof(request),"%ssnow", prefix);
    // io_json_query(request, f->snow, sizeof(f->snow));
    strncpy2(f->snow, "", sizeof(f->snow));
}

#else

void forecast_parse(unsigned char i, ForecastData *f) // OPEN_WEATHER
{
    Timestamp ts;
    char prefix[20];
    char request[60];
    char json_part[30];

    snprintf(prefix, sizeof(prefix), "/daily/%c/", '0' + i);

    snprintf(request, sizeof(request), "%sdt", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    timestamp(atol(json_part), &ts);

    snprintf(f->date, 8, "%d %s", ts.day, time_month(ts.month));
    snprintf(f->dow, 4, "%s", time_dow(ts.dow));

    snprintf(request, sizeof(request), "%sweather/0/description", prefix);
    io_json_query(request, f->desc, sizeof(f->desc));

    snprintf(request, sizeof(request), "%sweather/0/icon", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    f->icon = get_sprite(json_part);

    snprintf(request, sizeof(request), "%stemp/min", prefix);
    io_json_query(request, f->lo, sizeof(f->lo));
    io_decimals(f->lo, optData.maxPrecision);

    snprintf(request, sizeof(request), "%stemp/max", prefix);
    io_json_query(request, f->hi, sizeof(f->hi));
    io_decimals(f->hi, optData.maxPrecision);

    snprintf(request, sizeof(request), "%spressure", prefix);
    io_json_query(request, f->pressure, sizeof(f->pressure));

    snprintf(request, sizeof(request), "%swind_speed", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    io_decimals(json_part, optData.maxPrecision);
    snprintf(f->wind, sizeof(f->wind), "WIND:%s %s ", json_part, optData.units == IMPERIAL ? "mph" : "kph");

    snprintf(request, sizeof(request), "%swind_deg", prefix);
    io_json_query(request, json_part, sizeof(json_part));
    strcat(f->wind, degToDirection(atoi(json_part)));

    snprintf(request, sizeof(request), "%spop", prefix);
    io_json_query(request, f->pop, sizeof(f->pop));

    snprintf(request, sizeof(request), "%srain", prefix);
    io_json_query(request, f->rain, sizeof(f->rain));

    // snprintf(request,  sizeof(request),"%ssnow", prefix);
    // io_json_query(request, f->snow, sizeof(f->snow));
    strncpy2(f->snow, "", sizeof(f->snow));
}

#endif

void forecast(void)
{
  unsigned char bg, fg;
  bool  dayNight;
static  bool firstTime = true;    
static  FUJI_TIME future_time;
static  FUJI_TIME adjust_time;

  if (firstTime)
  {
      firstTime = false;
      io_time(&future_time);
  }

  if (time_reached(&future_time) || forceRefresh)
  {
    forceRefresh = false;

    memset(&adjust_time, 0, sizeof(FUJI_TIME));

    adjust_time.minute = (unsigned char) optData.refreshIntervalMinutes;
    add_time(&future_time, &future_time, &adjust_time);

    screen_forecast_init();
    
    screen_colors(dt, timezone_offset, &fg, &bg, &dayNight);
    screen_forecast_colors(dt, timezone_offset, &fg, &bg, &dayNight);


        if (forecast_open())
      screen_weather_could_not_get();
    else
    { 

      clear_all_sprites();  
      for (int i=0;i<4;i++)
      {
        forecast_parse((unsigned char) (i + forecast_offset), &forecastData);
        screen_forecast((unsigned char) i, &forecastData, fg, bg, true, &future_time);
      }
      display_sprites();
      forecast_close();
      
    }

  }

  input_init();

  screen_forecast_keys();

  timer = 65535;
  while (timer > 0)
  {
    if (input_forecast())
    {
      if (forceRefresh)
        io_time(&future_time);
      
      break; // allow input_init to be called again
    }

    if ((timer % CHECK_TIME_FREQUENCY) == 0)
    {
      if (time_reached(&future_time))
        break;
    }
    
    csleep(1);
    timer--;
   }

}
