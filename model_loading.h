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

std::vector<Patrimonio> importarModelo(const char * pFile);
Patrimonio PatrimonioFromMesh(int id, aiMesh* mesh);