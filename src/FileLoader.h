#pragma once

#include <string>

class FileLoader {
    public:
        void loadFile(std::string path);
        std::string getFileAsString();
        
    private:
        std::string fileString;
};
