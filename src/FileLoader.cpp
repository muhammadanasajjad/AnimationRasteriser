#include <fstream>
#include <FileLoader.h>
#include <tracy/Tracy.hpp>

void FileLoader::loadFile(std::string path) {
    ZoneScoped;
    fileString = "";
    std::fstream readFile(path);
    
    std::string fileLine = "";
    while (getline(readFile, fileLine)) {
        fileString += fileLine + "\n";
    }
    
    readFile.close();
}

std::string FileLoader::getFileAsString() {
    ZoneScoped;
    return fileString;
}
