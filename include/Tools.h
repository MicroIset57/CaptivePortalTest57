/*
    Tools.h
    JJTeam - 2021
*/

#pragma once

/**
 * Obtiene el ID del ESP32 (es la Mac Address, pero la muestro en Base 36)
 * 
 * En la pagina de https://macaddress.io/ se puede ver esto:
 * 
 * =============================================================
 *  Vendor details, OUI [7C:9E:BD], Company name: Espressif Inc
 * =============================================================
 * 
 * Por eso voy a usar como numero de serie solo los 3 bytes ultimos.
 */
const String getSerialNumber()
{
    struct
    {
        uint32_t h;
        uint32_t l;
    } u;
    esp_efuse_mac_get_default((uint8_t *)(&u));
    int id = ((u.h >> 8) & 0xff0000) | ((u.l << 8) & 0xff00) | ((u.l >> 8) & 0x00ff);
    return String(id, 36);
}

/**
 * Porqué se reseteó ?
 */
const char *getResetReason()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON:
        return "Power on Reset";
    case ESP_RST_SW:
        return "Reset via esp_restart";
    case ESP_RST_PANIC:
        return "Reset by exception or panic";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
        return "Reset by watchdog";
    case ESP_RST_DEEPSLEEP:
        return "Reset after sleep";
    case ESP_RST_BROWNOUT:
        return "Reset by Brownout";
    case ESP_RST_SDIO:
        return "Reset over SDIO";
    default:
        return "Reset reason can not be determined";
    }
}

/** Is this an IP? */
boolean isIp(String str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

/** IP to String? */
String toStringIp(IPAddress ip)
{
    String res = "";
    for (int i = 0; i < 3; i++)
    {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}
