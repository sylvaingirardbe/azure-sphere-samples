#pragma once

#include <stdbool.h>
#include <applibs/eventloop.h>

#define LSM6DSO_ID         0x6C   // register value
#define LSM6DSO_ADDRESS	   0x6A	  // I2C Address

typedef struct {
    float x;
    float y;
    float z;
} xl_data;

typedef struct {
    float x;
    float y;
    float z;
} ang_data;

typedef struct {
    float temp;
} temp_data;

typedef struct {
    float pressure;
} press_data;

int initI2c();
void closeI2c();
int readSensorData();
ang_data getAngBuffer();
xl_data getXlData();
temp_data getTempData();
press_data getPressData();
