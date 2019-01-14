//
// Created by Henrique on 09/01/2019.
//

#include "structs.h"
#include "raymath.h"
#include <vector>

enum Eixo {
    X, Y, Z
};

struct Octree{
    Octree* pai;
    Octree* filhos[8];
    bool filhosAtivos[8];
    BoundingBox regiao;
    int numeroPatrimonios;
    Patrimonio* patrimonios;
};

struct KDTree{
    KDTree* menor;
    KDTree* maior;
    BoundingBox regiao;
    Eixo eixo;
    float valorEixo;
    Patrimonio* patrimonio;
};

struct IndexDistance{
    int index;
    float distance;
};

void UnloadOctree(Octree* octree);
bool boxContainsBox(BoundingBox fora, BoundingBox dentro);
Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray, Octree *octree);
bool existeUmPatrimonioMaisProximoNaOctree(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree);
int indexPatrimonioMaisProximoNaOctree(Ray ray, Octree *octree);
IndexDistance indexDistanceMaisProximoNaOctree(IndexDistance indexDistance, Ray ray, Octree *octree);
void desenharOctree(Octree* octree, Color corFilhos, Color corSemFilhos);
KDTree* BuildKDTree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
void UnloadKDTree(KDTree* kdtree);