//
// Created by Henrique on 09/01/2019.
//

#include "structs.h"
#include "raymath.h"
#include <vector>
#include <cstdlib>

struct Octree{
    Octree* pai;
    Octree* filhos[8];
    //unsigned char filhosAtivos;
    bool filhosAtivos[8];
    BoundingBox regiao;
    int numeroPatrimonios;
    Patrimonio* patrimonios;
};

void UnloadOctree(Octree* octree);
bool boxContainsBox(BoundingBox fora, BoundingBox dentro);
Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray, Octree *octree);
bool existeUmPatrimonioMaisProximoNaOctree(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree);