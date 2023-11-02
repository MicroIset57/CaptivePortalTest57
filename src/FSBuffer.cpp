/*
    FsBuffer.hpp
    Guarda archivos en el File System a modo de buffer.
    Usa varios archivos para guardar en forma circular.
    Si existe una micro-SD la usa, sino abre en el FS de la flash del ESP32.
    Usa stream para guardar o leer.

    JJTeam - 2021
*/
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "FSBuffer.h"
#include <SPIFFS.h>
#include <SPI.h>
#include <SD.h>
#define FORMAT_LITTLEFS_IF_FAILED true
//#define SPIFFS FFat
#define FSBUFFER_STREAM_SIZE 64

constexpr char CONFIG_FILENAME[] = "/buffers.cfg";

//----------------------------------------------------------------------------

// crea el archivo a usar
void FsBuffer::initFile()
{
    if (fileSystemError)
        return;

    File f = open(cfgFileName, FILE_READ);
    if (!f)
    {
        // esta guardado en formato .ini
        fileIndexActual = filesCount - 1; // fuerzo al archivo 1.
        storeConfig();                    //
        nextFile();                       // dejo preparado el nombre a usar.
    }
    else
    {
        // la primer linea contiene el indice, ej: "fileIndexActual=2\n"
        f.readStringUntil('=');
        fileIndexActual = f.readStringUntil('\n').toInt();
        f.close();
        bufFileName = getFileName(fileIndexActual);
    }
}

// guarda cosas en flash (por si un reset)
void FsBuffer::storeConfig()
{
    if (fileSystemError)
        return;

    File f = open(cfgFileName, FILE_WRITE);
    if (f)
    {
        // dejo constancia en el archivo de configuracion, como leer los logs,
        // por las dudas que se levanten desde una PC.
        f.printf("fileIndexActual=%d\ntotalFiles=%d\n", fileIndexActual, filesCount);
    }
}

// Cambio de archivo. Voy al siguiente circularmente.
void FsBuffer::nextFile()
{
    if (fileSystemError)
        return;

    fileIndexActual++;
    if (fileIndexActual == filesCount)
    {
        fileIndexActual = 0;
        ESP_LOGW("*", "Ya se usaron todos los archivos de log. Se rota al primero!");
    }

    // ejemplo: /log/buf.1
    bufFileName = getFileName(fileIndexActual);
    remove(bufFileName);
}

String FsBuffer::getFileName(int index)
{
    return baseFilename + "/buf." + index;
}

File FsBuffer::open(const String &path, const char *mode)
{
    if (microSDExists)
        return SD.open(path, mode);
    else
        return SPIFFS.open(path, mode);
}

void FsBuffer::mkdir(const String &folder)
{
    if (microSDExists)
        SD.mkdir(folder);
    else
        SPIFFS.mkdir(folder);
}

void FsBuffer::remove(const String &path)
{
    if (microSDExists)
        SD.remove(path);
    else
        SPIFFS.remove(path);
}

void FsBuffer::printFromFile(String filename, Print &printer)
{
    if (fileSystemError)
        return;

    char buffer[FSBUFFER_STREAM_SIZE];
    size_t bytes;
    File f = open(filename, FILE_READ);
    if (f)
    {
        do
        {
            bytes = f.readBytes(buffer, FSBUFFER_STREAM_SIZE);
            printer.write(buffer, bytes);
        } while (bytes == FSBUFFER_STREAM_SIZE);
        f.close();
    }
}

/**
     * Se necesita configurar el tamaño de los archivos (en bytes),
     * la cantidad de archivos,
     * y el nombre que distingue los diferentes buffers que puedan existir.
     * 
     * Se grabará en la micro-SD si la memoria está disponible, sino usará la flash del ESP32.
     * 
     * El folder es una carpeta para separar varios posibles FsBuffers.
     */
void FsBuffer::begin(int pin_CS_microSD, uint32_t bytesPerFile, uint8_t filesQuantity, const String &folder)
{
    microSDExists = pin_CS_microSD == -1 ? false : SD.begin(pin_CS_microSD);

    if (!microSDExists)
    {
        Serial.println("No existe la micro-SD");

        ESP_LOGW("*", "No existe la micro-SD");
        if (!SPIFFS.begin(FORMAT_LITTLEFS_IF_FAILED))
        {
            Serial.println("LITTLEFS Mount Failed");

            ESP_LOGE("*", "FS ERROR! No puedo grabar logs");
            fileSystemError = true;
            return;
        }
        else
        {
            Serial.println("Se grabarán solo ERRORES en la flash");
            ESP_LOGI("*", "Se grabarán solo ERRORES en la flash");
        }
    }
    else
    {
        Serial.println("Se encontró una micro-SD");
        ESP_LOGI("*", "Se encontró una micro-SD, se grabarán Info y Errores");
    }

    fileSystemError = false;
    filesCount = filesQuantity;
    maxFileSize = bytesPerFile;
    baseFilename = folder;
    cfgFileName = folder + CONFIG_FILENAME;
    mkdir(folder);
    initFile();
}

inline size_t FsBuffer::write(uint8_t c)
{
    return write(&c, 1);
}

inline size_t FsBuffer::write(const uint8_t *txt)
{
    return write(txt, strlen((char *)txt));
}

/**
     * Agrega texto al archivo actual.
     * Si se excede el tamaño máximo, sigue con otro archivo.
     */
inline size_t FsBuffer::write(const uint8_t *txt, size_t len)
{
    if (fileSystemError)
        return 0;

    size_t size = 0;
    File f = open(bufFileName, FILE_APPEND);
    if (f)
    {
        size = f.write(txt, len);
        f.flush();
        int filesize = f.size();
        f.close();
        // si se excede el tamaño del archivo, cambia a siguiente.
        if (filesize > maxFileSize)
            nextFile();
        storeConfig();
    }
    return size;
}

/**
     * Envía todos los archivos al Printable que sea...(otro stream)
     */
void FsBuffer::printTo(Print &printer)
{
    if (fileSystemError)
        return;

    // Ejemplo: 2-3-0-1
    // (si el actual es el 2, empiezo con el 3 (el mas viejo) y sigo en forma circular)
    int index = fileIndexActual;
    do
    {
        printFromFile(getFileName(index), printer);
        if (++index == filesCount)
            index = 0;
    } while (index != fileIndexActual);
}

// abre un archivo, y por cada linea llama el callback.
void FsBuffer::forEachLineFromFile(String filename, ForEachLineCallback callback)
{
    File f = open(filename, FILE_READ);
    if (f)
    {
        String line;
        do
        {
            line = f.readStringUntil('\n');
            callback(line);
        } while (!line.isEmpty());
        f.close();
    }
}

// por cada archivo llama a recorrer las lineas...
void FsBuffer::forEachLine(ForEachLineCallback callback)
{
    if (fileSystemError)
        return;

    // Ejemplo: 2-3-0-1
    // (si el actual es el 2, empiezo con el 3 (el mas viejo) y sigo en forma circular)
    int index = fileIndexActual;
    do
    {
        forEachLineFromFile(getFileName(index), callback);
        if (++index == filesCount)
            index = 0;
    } while (index != fileIndexActual);
}

// elimina todos los archivos!
void FsBuffer::clear()
{
    if (fileSystemError)
        return;

    for (size_t i = 0; i < filesCount; i++)
    {
        bufFileName = getFileName(i);
        remove(bufFileName);
    }
    fileIndexActual = 0;
    storeConfig();
    initFile();
    ESP_LOGW("*", "ALL LOGS DELETED!");
}
