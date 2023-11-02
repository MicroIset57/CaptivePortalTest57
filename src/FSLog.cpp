/*
    FsLog.h
    Guarda archivos de logs en el File System.
    Usará la micro-SD si está disponible.
    Si no existe la micro-SD solo grabará ERRORES.

    Permite extraer o enviar los logs al puerto serie, wifi, etc.

    JJTeam - 2021
*/

#include "FSLog.h"

//-- unica instancia para todo el proyecto...
FsLog FSLOG;

// TAMAÑO DE LA LINEA DE LOG !!!
#define TAM_BUF 200

constexpr char STARTUP_FILENAME[] = "/startup.log";
constexpr char NotInitialized[] = "FsLog class not initialized";

void FsLog::begin(int pin_CS_microSD, HardwareSerial &out, uint32_t bytesPerFile)
{
    output = &out;
    if (!initialized)
    {
        FsBuffer::begin(pin_CS_microSD, bytesPerFile, 4, folder);
        initialized = true;
    }
    startupLogFileName = folder + STARTUP_FILENAME;
    mkdir(folder);
    remove(startupLogFileName);
}

void FsLog::SetModoDiagnostico(bool enable)
{
    modoDiagnostico = enable;
}

// escribe en un archivo separado, se pisa en cada RESET.
void FsLog::startup(const char *format, ...)
{
    if (!initialized)
        throw NotInitialized;

    va_list argptr;
    char buf[TAM_BUF];
    va_start(argptr, format);
    vsprintf(buf, format, argptr);

    File f = open(startupLogFileName, FILE_APPEND);
    if (f)
    {
        f.write((uint8_t *)buf, strlen(buf));
        f.close();
    }
    else
    {
        ESP_LOGE("*", "Can't write logs: %s", buf);
    }
    va_end(argptr);
}

// imprime los logs en una salida streameable
void FsLog::printStartupTo(Print &printer)
{
    printFromFile(startupLogFileName, printer);
}

void FsLog::forEachStartup(ForEachLineCallback callback)
{
    forEachLineFromFile(startupLogFileName, callback);
}

/**
 * Posibles tipos de log: [D,I,E] (debug, info, error)
 * LogTrace y LogInfoDetail solo funcionaran si esta el modoDiag=true.
 * 
 * LogDebug => solo Serie
 * LogInfo  => Serie y en la micro-SD (si existe)
 * LogError => Serie y en el FS que exista (microSD o ESP flash)
 */
void FsLog::log(const char *format, ...)
{
    if (!initialized)
        throw NotInitialized;

    va_list argptr;
    char buf[TAM_BUF];
    va_start(argptr, format);
    vsprintf(buf, format, argptr);
    int len = strlen(buf);

    // todo sale por el puerto serie...salvo que no este el ModoDiagnostico
    if (format[1] == 'D' || format[1] == 'I' || format[1] == 'E' ||
        (format[1] == 'T' && modoDiagnostico) ||
        (format[1] == 'V' && modoDiagnostico))
    {
        output->print(buf);
    }

    // algunas cosas se graban...y verifica el ModoDiagnostico
    if (format[1] == 'E' ||
        (microSDExists && format[1] == 'I') ||
        (microSDExists && modoDiagnostico && format[1] == 'V'))
    {
        write((uint8_t *)buf, len);
    }

    va_end(argptr);
}

String FsLog::getStatus()
{
    return String("micro-SD ") + (microSDExists ? "Exists" : "NOT Exists") +
           String(", modoDiagnostico=") + (modoDiagnostico ? "on" : "off") +
           "\nstartupLogFileName=" + startupLogFileName;
}
