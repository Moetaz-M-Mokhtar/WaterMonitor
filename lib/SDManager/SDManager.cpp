
#include "SDManager.h"
#include <string.h>

const char *data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    Serial.printf("%s: ", (const char *)data);
    for (i = 0; i < argc; i++)
    {
        Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.printf("\n");
    return 0;
}

char *zErrMsg = 0;
int SDManager::db_exec(sqlite3 *db, const char *sql)
{
    Serial.println(sql);
    long start = micros();
    int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        Serial.printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        Serial.printf("Operation done successfully\n");
    }
    Serial.print(F("Time taken:"));
    Serial.println(micros() - start);
    return rc;
}

SDManager::SDManager(uint8_t chipSelect, int serialBaudRate) : chipSelect(chipSelect), serialBaudRate(serialBaudRate) {}

bool SDManager::SDInit()
{
    Serial.begin(serialBaudRate);

    Sprint("\nInitializing SD card...\n");

    if (!SD.begin(chipSelect))
    {
        Sprintln("Card Mount Failed");
        return false;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Sprintln("No SD card attached");
        return false;
    }

    Sprint("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Sprintln("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Sprintln("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Sprintln("SDHC");
    }
    else
    {
        Sprintln("UNKNOWN");
    }

    sqlite3_initialize();

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Sprintf("SD Card Size: %lluMB\n", cardSize);
    return true;
}

bool SDManager::readConfig(DynamicJsonDocument *document, String configFileName)
{
    String filename = configurationPath + configFileName + ".json";
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    Sprintln(F("Open file in write mode"));
    File configFile = SD.open(filename);
    if (configFile)
    {
        Sprint(F("Filename --> "));
        Sprintln(filename);

        Sprint(F("Start Read..."));
        DeserializationError error = deserializeJson(*document, configFile);

        Sprint(F("..."));
        Sprintln("Data: ");
        if (serialLog)
            serializeJsonPretty(*document, Serial);

        if (error)
        {
            // if the file didn't open, print an error:
            Sprint(F("Error parsing JSON "));
            Sprintln(error.c_str());
            return false;
        }
        configFile.close();
        Sprintln(F("done."));
        return true;
    }
    else
    {
        // if the file didn't open, print an error:
        Sprint(F("Error opening (or file not exists) "));
        Sprintln(filename);

        return false;
    }
}

bool SDManager::saveConfig(DynamicJsonDocument *document, String configFileName)
{
    String filename = configurationPath + configFileName + ".json";
    SD.remove(filename);

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    Sprintln(F("Open file in write mode"));
    File configFile = SD.open(filename, FILE_WRITE);
    if (configFile)
    {
        Sprint(F("Filename --> "));
        Sprintln(filename);

        Sprint(F("Start write..."));

        serializeJsonPretty(*document, configFile);

        Sprint(F("..."));
        // close the file:
        configFile.close();
        Sprintln(F("done."));

        return true;
    }
    else
    {
        // if the file didn't open, print an error:
        Sprint(F("Error opening "));
        Sprintln(filename);

        return false;
    }
}

bool SDManager::factoryResetCard()
{
    fs::File root = SD.open("/");
    rmdirRec(root);
    root.close();
    if (!SD.mkdir(configurationPath))
    {
        //faile to create config file
        Serial.println("Create config folder failed");
        return false;
    }

    //create config files
    DynamicJsonDocument WiFiDoc(100);
    WiFiDoc["SSID"] = "Wifi_SSID";
    WiFiDoc["Password"] = "Password";

    saveConfig(&WiFiDoc, "/Communication");
    readConfig(&WiFiDoc, "/Communication");

    struct sensorMetaData digitalMetaData[digitalSensorCount];

    for (uint8_t index = 0; index < digitalSensorCount; index++)
    {
        digitalMetaData[index].name = "Sen0" + String(index);
        digitalMetaData[index].description = "digital Sensor No. " + String(index);
        digitalMetaData[index].calibration = 1;
        digitalMetaData[index].id = index;
        digitalMetaData[index].active = index % 2 == 0;
    }

    struct sensorMetaData analogMetaData[analogSensorCount];

    for (uint8_t index = 0; index < analogSensorCount; index++)
    {
        analogMetaData[index].name = "Sen0" + String(index);
        analogMetaData[index].description = "digital Sensor No. " + String(index);
        analogMetaData[index].calibration = 1;
        analogMetaData[index].id = digitalSensorCount + index;
        analogMetaData[index].active = index % 2 == 0;
    }

    File dbFile = SD.open("/SensorReadings.db", FILE_WRITE);
    dbFile.close();

    DynamicJsonDocument senMetatData = senMetaData2Doc(digitalMetaData, analogMetaData);
    saveConfig(&senMetatData, "/Sensors");
    readConfig(&senMetatData, "/Sensors");

    //create readings database
    int rc = sqlite3_open(dbFile.name(), &SensorReadings);
    if (rc)
    {
        Sprintf("Can't open database: %s\n", sqlite3_errmsg(SensorReadings));
        return false;
    }
    else
    {
        Sprint("Opened database successfully\n");
        //create DB Table
        String sql = "CREATE TABLE Reading(TimeStamp datetime";
        for (int count = 0; count < digitalSensorCount; count++)
        {
            sql = sql + ", DSen0" + String(count) + " numeric";
        }
        for (int count = 0; count < analogSensorCount; count++)
        {
            sql = sql + ", ASen0" + String(count) + " numeric";
        }
        sql = sql + ");";
        rc = db_exec(SensorReadings, sql.c_str());
    }

    return true;
}

bool SDManager::rmdirRec(fs::File dir)
{
    dir.rewindDirectory();
    while (true)
    {
        File nextDir = dir.openNextFile();
        if (!nextDir)
        {
            //no more, delete the directory
            Sprint("Removing folder ");
            Sprint(dir.name());
            if (SD.rmdir(dir.name()))
            {
                Sprintln(" - Sucess");
            }
            else
            {
                Sprintln(" - Fail");
            }
            return true;
        }
        else
        {
            if (nextDir.isDirectory())
            {
                rmdirRec(nextDir);
            }
            else
            {
                //delete the file
                Sprint("Removing file ");
                Sprint(nextDir.name());
                if (SD.remove(nextDir.name()))
                {
                    Sprintln(" - Sucess");
                }
                else
                {
                    Sprintln(" - Fail");
                }
            }
        }
    }
}

DynamicJsonDocument senMetaData2Doc(sensorMetaData *digitalSenData, sensorMetaData *analogSenData, uint8_t digitalSenCount, uint8_t analogSenCount)
{
    DynamicJsonDocument senDocument(2100);

    JsonArray digital = senDocument.createNestedArray("digital");
    for (uint8_t index = 0; index < digitalSenCount; index++)
    {
        sensorMetaData data = digitalSenData[index];
        JsonObject metaDataObject = digital.createNestedObject();
        metaDataObject["name"] = data.name;
        metaDataObject["description"] = data.description;
        metaDataObject["Calibration"] = data.calibration;
        metaDataObject["id"] = data.id;
        metaDataObject["active"] = data.active;
    }

    JsonArray analog = senDocument.createNestedArray("analog");
    for (uint8_t index = 0; index < analogSenCount; index++)
    {
        sensorMetaData data = analogSenData[index];
        JsonObject metaDataObject = analog.createNestedObject();
        metaDataObject["name"] = data.name;
        metaDataObject["description"] = data.description;
        metaDataObject["Calibration"] = data.calibration;
        metaDataObject["id"] = data.id;
        metaDataObject["active"] = data.active;
    }

    return senDocument;
}