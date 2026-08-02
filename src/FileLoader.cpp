#include <fstream>
#include <FileLoader.h>

void FileLoader::loadFile(std::string path) {
    fileString = "";
    std::fstream readFile(path);
    
    std::string fileLine = "";
    while (getline(readFile, fileLine)) {
        fileString += fileLine + "\n";
    }
    
    readFile.close();
}

std::string FileLoader::getFileAsString() {
    return fileString;
}
