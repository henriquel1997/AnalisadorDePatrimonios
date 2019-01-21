//
// Created by henrique on 21/01/19.
//

#include "model_loading.h"

bool importarModelo(const char * pFile){

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( pFile,
                                              aiProcess_CalcTangentSpace       |
                                              aiProcess_Triangulate            |
                                              aiProcess_JoinIdenticalVertices  |
                                              aiProcess_SortByPType);

    if(!scene){
        printf(importer.GetErrorString());
        return false;
    }

    return true;
}