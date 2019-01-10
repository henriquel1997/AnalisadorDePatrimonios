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
    unsigned char filhosAtivos;
    BoundingBox regiao;
    std::vector<Patrimonio> patrimonios;
};

void UnloadOctree(Octree* octree);
void definirTamanhoMinimoOctree(float novo_min);
bool boxContainsBox(BoundingBox fora, BoundingBox dentro);
Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);