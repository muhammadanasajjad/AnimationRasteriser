#include <fstream>
#include <FileLoader.h>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void FileLoader::loadFile(std::string path) {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    fileString = "";
    std::fstream readFile(path);
    
    std::string fileLine = "";
    while (getline(readFile, fileLine)) {
        fileString += fileLine + "\n";
    }
    
    readFile.close();
}

std::string FileLoader::getFileAsString() {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    return fileString;
}
