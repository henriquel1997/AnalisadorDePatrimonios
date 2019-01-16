//
// Created by Henrique on 09/01/2019.
//

#include "structs.h"
#include "raymath.h"
#include <vector>

enum Eixo {
    X, Y, Z
};

struct IndexDistance{
    int index;
    float distance;
};

/*--- Octree ---*/

struct Octree{
    Octree* pai;
    Octree* filhos[8];
    bool filhosAtivos[8];
    BoundingBox regiao;
    int numeroPatrimonios;
    Patrimonio* patrimonios;
};

void UnloadOctree(Octree* octree);
bool boxContainsBox(BoundingBox fora, BoundingBox dentro);
Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray, Octree *octree);
bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree);
int indexPatrimonioMaisProximo(Ray ray, Octree *octree);
IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, Octree *octree);
void desenharOctree(Octree* octree, Color corFilhos, Color corSemFilhos);

/*--- KD-Tree ---*/

struct KDTree{
    KDTree* menor;
    KDTree* maior;
    BoundingBox regiao;
    Eixo eixo;
    float valorEixo;
    Patrimonio* patrimonio;
    Triangulo* triangulo;
};

KDTree* BuildKDTree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
KDTree* BuildKDTreeTriangulos(BoundingBox regiao, std::vector<Patrimonio> patrimonios);
KDTree* BuildKDTree(BoundingBox regiao, std::vector<Triangulo> triangulos);
void UnloadKDTree(KDTree* kdtree);
bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray, KDTree* kdtree);
bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, KDTree* kdtree);
int indexPatrimonioMaisProximo(Ray ray, KDTree *kdtree);
IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, KDTree *kdtree);
void desenharKDTree(KDTree *kdtree, Color corFilhos, Color corSemFilhos);