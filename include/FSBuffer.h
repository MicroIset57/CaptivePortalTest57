
#ifndef _Fs_Buffer_h
#define _Fs_Buffer_h

#include <Print.h>
#include "FS.h"

//----------------------------------------------------------------------------

typedef void (*ForEachLineCallback)(String line);

class FsBuffer : public Print
{
private:
    uint32_t maxFileSize;    // tamaño maximo de cada archivo
    uint8_t filesCount;      // cantidad de archivos que usaremos
    uint8_t fileIndexActual; // archivo que se esta escribiendo
    String baseFilename;     // puntero al string constante que se paso en el constructor
    String bufFileName;      // filename= "8.3\0"
    String cfgFileName;      // configuraciones
    void initFile();         // crea el archivo a usar
    void storeConfig();      // guarda cosas en flash (por si un reset)
    void nextFile();         // Cambio de archivo. Voy al siguiente circularmente.
    String getFileName(int index);

protected:
    bool microSDExists = false;   // se grabará en SD si está disponible, sino usa la flash solo para ERROR.
    bool fileSystemError = false; // true si no puedo grabar en SD ni flash!
    File open(const String &path, const char *mode);
    void mkdir(const String &folder);
    void remove(const String &path);
    void printFromFile(String filename, Print &printer);
    void forEachLineFromFile(String filename, ForEachLineCallback callback);

public:
    void begin(int pin_CS_microSD, uint32_t bytesPerFile, uint8_t filesQuantity, const String &folder);
    size_t write(uint8_t c);
    size_t write(const uint8_t *txt);
    size_t write(const uint8_t *txt, size_t len);
    void printTo(Print &printer);
    void forEachLine(ForEachLineCallback callback);
    void clear();
};

//----------------------------------------------------------------------------
#endif // Fs_Buffer_h