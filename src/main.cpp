#include <SD.h>
#include <SDManager.h>

const int chipSelect = 5;

void setup()
{
  SDManager SDFileOperation = SDManager();

  if (SDFileOperation.SDInit())
  {
    Serial.println(SDFileOperation.factoryResetCard());
    //Serial.println(SD.open("TEST.txt"));
    Serial.println("done!");
  }
  else
  {
    Serial.println("FAILED");
  }
}

void loop()
{

  // nothing happens after setup finishes.
}
