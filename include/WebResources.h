/**
 * Acceso a los recursos de la carpeta web, que se linkean y terminan en la flash del ESP32.
 * 
 * JJTeam
 */

#pragma once
#include <Arduino.h>

constexpr char HTML_BODY_START[] = "<!DOCTYPE html><html><head>"
                                   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                                   "<link rel=\"stylesheet\" href=\"style.css\">"
                                   "</head><body>";

constexpr char HTML_BODY_END[] = "</body></html>";

constexpr char HTML_MENU_INDEX[] = "<ul><li><a href='/wifi'>Configurar la conexion de WiFi</a></li>"
                                   "<li><a href='/logs'>Acceso a los logs</a></li></ul>";

//-- archivos linkeados (se suben a la flash en el linker, y queda esta referencia para usarlos)
//-- agregar los archivos en el platformio.ini
// @see: https://docs.platformio.org/en/latest/platforms/espressif32.html#embedding-binary-data
#define WEB_RESOURCE(_resource)                                              \
  extern const char _resource[] asm("_binary_web_" #_resource "_start");     \
  extern const char _resource##_end[] asm("_binary_web_" #_resource "_end"); \
  extern const size_t _resource##_size = _resource##_end - _resource;

WEB_RESOURCE(style_css)
WEB_RESOURCE(logo_jpg)
