//
// Created by Henrique on 08/01/2019.
//

#include "octree.h"

//Libera a memória da octree e de seus filhos
void UnloadOctree(Octree* octree) {
    if (octree == nullptr) {
        return;
    }

    free(octree->patrimonios);

    for (auto &child : octree->filhos) {
        if(child != nullptr){
            UnloadOctree(child);
        }
    }

    free(octree);
}

bool boxContainsBox(BoundingBox fora, BoundingBox dentro){
    return  fora.min.x <= dentro.min.x &&
            fora.min.y <= dentro.min.y &&
            fora.min.z <= dentro.min.z &&
            fora.max.x >= dentro.max.x &&
            fora.max.y >= dentro.max.y &&
            fora.max.z >= dentro.max.z;
}

Octree* BuildOctree(BoundingBox regiao, std::vector<Patrimonio> patrimonios){

    Vector3 dimensoes = Vector3Subtract(regiao.max, regiao.min);

    auto *octree = (Octree*)malloc(sizeof(Octree));
    octree->pai = nullptr;
    octree->regiao = regiao;

    if(patrimonios.size() == 1){
        for (int i = 0; i < 8; i++) {
            octree->filhos[i] = nullptr;
            octree->filhosAtivos[i] = false;
        }
        octree->numeroPatrimonios = 1;
        auto p = (Patrimonio*)malloc(sizeof(Patrimonio));
        *p = patrimonios[0];
        octree->patrimonios = p;
        return octree;
    }

    Vector3 meio = Vector3Divide(dimensoes, 2.f);
    Vector3 centro = Vector3Add(regiao.min, meio);

    BoundingBox octantes[8];
    octantes[0] = (BoundingBox){regiao.min, centro};
    octantes[1] = (BoundingBox){(Vector3){centro.x, regiao.min.y, regiao.min.z}, (Vector3){regiao.max.x, centro.y, centro.z}};
    octantes[2] = (BoundingBox){(Vector3){centro.x, regiao.min.y, centro.z}, (Vector3){regiao.max.x, centro.y, regiao.max.z}};
    octantes[3] = (BoundingBox){(Vector3){regiao.min.x, regiao.min.y, centro.z}, (Vector3){centro.x, centro.y, regiao.max.z}};
    octantes[4] = (BoundingBox){(Vector3){regiao.min.x, centro.y, regiao.min.z}, (Vector3){centro.x, regiao.max.y, centro.z}};
    octantes[5] = (BoundingBox){(Vector3){centro.x, centro.y, regiao.min.z}, (Vector3){regiao.max.x, regiao.max.y, centro.z}};
    octantes[6] = (BoundingBox){centro, regiao.max};
    octantes[7] = (BoundingBox){(Vector3){regiao.min.x, centro.y, centro.z}, (Vector3){centro.x, regiao.max.y, regiao.max.z}};

    std::vector<Patrimonio> octList[8];
    std::vector<Patrimonio> delist;

    for(auto &patrimonio : patrimonios){
        for(int i = 0; i < 8; i++){
            if(boxContainsBox(octantes[i], patrimonio.bBox)){
                octList[i].push_back(patrimonio);
                delist.push_back(patrimonio);
                break;
            }
        }
    }

    //Remove os patrimonios da lista que foram colocados em alguma octList, os que ficaram serão os pertecentes a este nó
    for(auto &patrimonioRemover : delist){
        for(int i = 0; i < patrimonios.size(); i++){
            if(patrimonios[i].id == patrimonioRemover.id){
                patrimonios.erase(patrimonios.begin() + i);
                break;
            }
        }
    }

    octree->numeroPatrimonios = patrimonios.size();
    octree->patrimonios = (Patrimonio*)malloc(sizeof(Patrimonio)*patrimonios.size());
    for(int i = 0; i < patrimonios.size(); i++){
        octree->patrimonios[i] = patrimonios[i];
    }

    for(int i = 0; i < 8; i++){
        if(!octList[i].empty()){
            Octree* filho = BuildOctree(octantes[i], octList[i]);
            if(filho != nullptr){
                filho->pai = octree;
                octree->filhosAtivos[i] = true;
            }else{
                octree->filhosAtivos[i] = false;
            }
            octree->filhos[i] = filho;
        }else{
            octree->filhos[i] = nullptr;
            octree->filhosAtivos[i] = false;
        }
    }

    return octree;
}

bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray, Octree *octree){

    if(octree != nullptr && CheckCollisionRayBox(ray, patrimonio.bBox)){

        RayHitInfo patrimonioHitInfo = GetCollisionRayModel(ray, &patrimonio.model);

        if(patrimonioHitInfo.hit){
            return !existeUmPatrimonioMaisProximoNaOctree(patrimonio.id, patrimonioHitInfo.distance, ray, octree);
        }
    }

    return false;
}

bool existeUmPatrimonioMaisProximoNaOctree(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree){

    if(CheckCollisionRayBox(ray, octree->regiao)){
        for(int i = 0; i < octree->numeroPatrimonios; i++){
            Patrimonio patrimonio = octree->patrimonios[i];
            if(patrimonio.id != patrimonioIndex && CheckCollisionRayBox(ray, patrimonio.bBox)){
                RayHitInfo newHitInfo = GetCollisionRayModel(ray, &patrimonio.model);
                if(newHitInfo.distance < patrimonioDistance){
                    return true;
                }
            }
        }

        for(int i = 0; i < 8; i++){
            if(octree->filhosAtivos[i]){
                if(existeUmPatrimonioMaisProximoNaOctree(patrimonioIndex, patrimonioDistance, ray, octree->filhos[i])){
                    return true;
                }
            }
        }
    }

    return false;
}

int indexPatrimonioMaisProximoNaOctree(Ray ray, Octree *octree){

    int indexMaisProximo = -1;
    float distanciaMaisProximo = 3.40282347E+38f;

    return indexDistanceMaisProximoNaOctree((IndexDistance) {indexMaisProximo, distanciaMaisProximo}, ray, octree).index;
}

IndexDistance indexDistanceMaisProximoNaOctree(IndexDistance indexDistance, Ray ray, Octree *octree){
    if(CheckCollisionRayBox(ray, octree->regiao)){
        for(int i = 0; i < octree->numeroPatrimonios; i++){
            Patrimonio patrimonio = octree->patrimonios[i];
            if(patrimonio.id != indexDistance.index && CheckCollisionRayBox(ray, patrimonio.bBox)){
                RayHitInfo newHitInfo = GetCollisionRayModel(ray, &patrimonio.model);
                if(newHitInfo.distance < indexDistance.distance){
                    indexDistance = {patrimonio.id, newHitInfo.distance};
                }
            }
        }

        for(int i = 0; i < 8; i++){
            if(octree->filhosAtivos[i]){
                indexDistance = indexDistanceMaisProximoNaOctree(indexDistance, ray, octree->filhos[i]);
            }
        }
    }

    return indexDistance;
}

void desenharOctree(Octree* octree, Color corFolha, Color corNaoFolha){
    bool temFilhos = false;
    for(int i = 0; i < 8; i++){
        if(octree->filhosAtivos[i]){
            temFilhos = true;
            desenharOctree(octree->filhos[i], corFolha, corNaoFolha);
        }
    }

    Color cor;
    if(temFilhos){
        cor = corNaoFolha;
    }else{
        cor = corFolha;
    }
    DrawBoundingBox(octree->regiao, cor);
}