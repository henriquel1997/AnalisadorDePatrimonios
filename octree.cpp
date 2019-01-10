//
// Created by Henrique on 08/01/2019.
//

#include "octree.h"

float min_octree_size = 0.1f;

//Libera a memória da octree e de seus filhos
void UnloadOctree(Octree* octree){
    if(octree == nullptr){
        return;
    }

    free(octree->pai);
    for (auto &child : octree->filhos) {
        UnloadOctree(child);
        free(child);
    }
}

void definirTamanhoMinimoOctree(float novo_min){
    min_octree_size = novo_min;
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

    bool dimensaoMenor = dimensoes.x <= min_octree_size || dimensoes.y <= min_octree_size || dimensoes.z <= min_octree_size;

    if(patrimonios.empty() || dimensaoMenor){
        return nullptr;
    }

    auto *octree = (Octree*)malloc(sizeof(Octree));
    octree->pai = nullptr;
    octree->regiao = regiao;

    if(patrimonios.size() == 1){
        for (auto &filho : octree->filhos) {
            filho = nullptr;
        }
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
                //TODO: Testar se dá pra remover o patrimônio da lista direto aqui ao invés de fazer depois
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

    octree->patrimonios = patrimonios;

    for(int i = 0; i < 8; i++){
        if(!octList[i].empty()){
            Octree* filho = BuildOctree(octantes[i], octList[i]);
            if(filho != nullptr){
                filho->pai = octree;
                octree->filhosAtivos |= (char)(1 << i);
            }
            octree->filhos[i] = filho;
        }else{
            octree->filhos[i] = nullptr;
        }
    }

    return octree;
}