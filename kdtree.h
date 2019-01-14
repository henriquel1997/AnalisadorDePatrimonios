//
// Created by Henrique on 14/01/2019.
//


#include <raylib.h>
#include "structs.h"
#include <vector>

enum Eixo {
    X, Y, Z
};

struct KDTree{
    KDTree* menor;
    KDTree* maior;
    BoundingBox regiao;
    Eixo eixo;
    float valorEixo;
    Patrimonio* patrimonio;
};

KDTree* BuildKDTree(BoundingBox regiao, std::vector<Patrimonio> patrimonios);