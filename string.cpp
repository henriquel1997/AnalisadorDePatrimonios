//
// Created by henrique on 21/01/19.
//


#include "string.h"

bool hasEndingString(char* const &fullString, char* const &ending) {
    if (strlen(fullString) >= strlen(ending)) {

        size_t pos = strlen(fullString) - strlen(ending);
        bool igual = true;
        for(int i = 0;  i < strlen(ending); i++){
            if(fullString[i + pos] != ending[i]){
                igual = false;
                break;
            }
        }

        return igual;
    } else {
        return false;
    }
}

char* concat(char* dest, char* source){
    int destSize = sizeof(dest) * strlen(dest);
    int sourceSize = sizeof(source) * strlen(source);
    auto buffer = (char*)malloc(static_cast<size_t>(destSize + sourceSize));
    sprintf(buffer, "%s%s", dest, source);
    return buffer;
}

char* copy(char* str){
    auto buffer = (char*)malloc(static_cast<size_t>(sizeof(str) * strlen(str)));
    sprintf(buffer, "%s", str);
    return buffer;
}