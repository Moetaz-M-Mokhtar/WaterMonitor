#ifndef _SD_MANAGER_H_
#define _SD_MANAGER_H_

#include "SD.h"
#include "FS.h"
#include "ArduinoJson.h"
#include <Arduino.h>
#include "sqlite3.h"

#define serialLog true
#define configurationPath "/configs"

#define analogSensorCount 5
#define digitalSensorCount 5

#define Sprint(a)  \
    if (serialLog) \
    (Serial.print(a))

#define Sprintf(a, ...) \
    if (serialLog)      \
    (Serial.printf(a, __VA_ARGS__))

#define Sprintln(a) \
    if (serialLog)  \
    (Serial.println(a))

struct sensorMetaData
{
    String name;
    String description;
    float calibration;
    uint8_t id;
    bool active;
};

DynamicJsonDocument senMetaData2Doc(sensorMetaData *digitalSenData, sensorMetaData *analogSenData, uint8_t digitalSenCount = digitalSensorCount, uint8_t analogSenCount = analogSensorCount);

class SDManager
{
private:
    uint8_t chipSelect;
    int serialBaudRate;

public:
    sqlite3 *SensorReadings;
    SDManager(uint8_t chipSelect = 5, int serialBaudRate = 9600);
    bool SDInit();
    bool readConfig(DynamicJsonDocument *document, String configFileName);
    bool saveConfig(DynamicJsonDocument *document, String filename);
    bool factoryResetCard();
    bool rmdirRec(fs::File dir);
    int db_exec(sqlite3 *db, const char *sql);
};

#endif