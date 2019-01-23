//
// Created by henrique on 21/01/19.
//

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include "string.h"
#include "arvores.h"
#include "raylib.h"
#include <rlgl.h>

std::vector<Patrimonio> importarModelo(const char * pFile, BoundingBox boundingBox);
Patrimonio PatrimonioFromMesh(int id, const char* nome, aiMatrix4x4 transform, aiMatrix4x4 scaleMatrix, aiMesh* mesh);
std::vector<Patrimonio> PatrimonioFromNode(int initialID, const aiScene* scene, aiMatrix4x4 parentTransform, aiMatrix4x4 scaleMatrix, aiNode* node);
aiVector3D multiplyByMatrix(aiVector3D vec, aiMatrix4x4 mat);
aiMatrix4x4 getScaleMatrix(BoundingBox boundingBox, const aiScene* scene);