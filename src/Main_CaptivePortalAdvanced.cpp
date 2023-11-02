

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#include "WifiCheck.h"
#include <Arduino.h>
#include "FSLog.h"

/*
  Ejemplo tomado de :
  https://github.com/esp8266/Arduino/tree/master/libraries/DNSServer/examples/CaptivePortalAdvanced

   El server maneja 2 loops simultaneamente:
   - WLAN 
   - SoftAP 

   El SoftAP te permite configurar la WLAN (guarda params en la EEPROM)

   Cuando el celu o la PC se conecta a este server AP, salta a una pagina de configuracion.
   - sino ir a :  http://192.168.4.1/wifi

   Si estan bien las credenciales se sonecta a la wifi (la guarda en EEPROM para volver a conectar en un reset)

   Podes acceder al web server desde 192.168.0.x
   o desde http://esp8266.local 
   
 */

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\nINIT TEST\n");

  FSLOG.begin(5, Serial);
  FSLOG.SetModoDiagnostico(true);
  delay(500);
  WifiSetup();

  LogAtStartUp("idf version:%s", esp_get_idf_version()); //3.10006.210326 (1.0.6)
  LogAtStartUp("start %X", random(0xfff));
  LogInfo("hola %X", random(0xfff));
  LogError("esto es un error %X", random(0xfff));
}

void loop()
{
  WifiLoop();
}
