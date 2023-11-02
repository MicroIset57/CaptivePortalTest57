/*
    FsLog.h
    Guarda archivos de logs en el File System.
    Usará la micro-SD si está disponible.
    Si no existe la micro-SD solo grabará ERRORES.

    Permite extraer o enviar los logs al puerto serie, wifi, etc.

    JJTeam - 2021
*/

#ifndef _Fs_Log_h
#define _Fs_Log_h

#include "FSBuffer.h"

#define FSLOG_FORMAT(letter, format) "[" #letter "]: " format "\n"
#define FSLOG_FORMAT2(format) format "\n"

#ifndef NO_USAR_FS_LOG

//// Todas estas formas de llamar al Logger:
#define LogDebug(format, ...) FSLOG.log(FSLOG_FORMAT(D, format), ##__VA_ARGS__)
#define LogDebugDiag(format, ...) FSLOG.log(FSLOG_FORMAT(T, format), ##__VA_ARGS__)
#define LogTrace(format, ...) FSLOG.log(FSLOG_FORMAT(T, format), ##__VA_ARGS__)
//// LogDebugDiag() y LogTrace() son iguales
#define LogInfo(format, ...) FSLOG.log(FSLOG_FORMAT(I, format), ##__VA_ARGS__)
#define LogInfoDiag(format, ...) FSLOG.log(FSLOG_FORMAT(V, format), ##__VA_ARGS__)
#define LogInfoDetail(format, ...) FSLOG.log(FSLOG_FORMAT(V, format), ##__VA_ARGS__)
//// LogInfoDiag() y LogInfoDetail() son iguales
#define LogError(format, ...) FSLOG.log(FSLOG_FORMAT(E, format), ##__VA_ARGS__)
#define LogAtStartUp(format, ...) FSLOG.startup(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogBegin(a, b) FSLOG.begin(a, b)
#define LogModoDiagnostico(b) FSLOG.SetModoDiagnostico(b)

#else // NO_USAR_FS_LOG

#define LogDebug(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogDebugDiag(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogTrace(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogInfo(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogInfoDiag(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogInfoDetail(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogError(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogAtStartUp(format, ...) Serial.printf(FSLOG_FORMAT2(format), ##__VA_ARGS__)
#define LogBegin(a, b)
#define LogModoDiagnostico(b)

#endif // NO_USAR_FS_LOG

class FsLog : public FsBuffer
{
private:
    bool initialized = false;
    bool modoDiagnostico = false;
    HardwareSerial *output;
    String startupLogFileName;
    String folder = "/logger"; // solo una carpeta!

public:
    void begin(int pin_CS_microSD, HardwareSerial &out, uint32_t bytesPerFile = 1000);
    void SetModoDiagnostico(bool enable);
    void startup(const char *format, ...); // escribe en un archivo separado, se pisa en cada RESET.
    void printStartupTo(Print &printer);   // imprime los logs en una salida streameable
    void forEachStartup(ForEachLineCallback callback);
    void log(const char *format, ...);
    String getStatus();
};

//-- unica instancia para todo el proyecto...
extern FsLog FSLOG;

#endif // Fs_Log_h
