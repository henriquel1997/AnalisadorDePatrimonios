//
// Created by Henrique on 08/01/2019.
//

#include "arvores.h"

/*--- Octree ---*/

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
            return !existeUmPatrimonioMaisProximo(patrimonio.id, patrimonioHitInfo.distance, ray, octree);
        }
    }

    return false;
}

bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, Octree *octree){

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
                if(existeUmPatrimonioMaisProximo(patrimonioIndex, patrimonioDistance, ray, octree->filhos[i])){
                    return true;
                }
            }
        }
    }

    return false;
}

int indexPatrimonioMaisProximo(Ray ray, Octree *octree){

    int indexMaisProximo = -1;
    float distanciaMaisProximo = 3.40282347E+38f;

    return indexDistanceMaisProximo((IndexDistance) {indexMaisProximo, distanciaMaisProximo}, ray, octree).index;
}

IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, Octree *octree){
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
                indexDistance = indexDistanceMaisProximo(indexDistance, ray, octree->filhos[i]);
            }
        }
    }

    return indexDistance;
}

void desenharOctree(Octree* octree, Color corFilhos, Color corSemFilhos){
    if(octree != nullptr){
        bool temFilhos = false;
        for(int i = 0; i < 8; i++){
            if(octree->filhosAtivos[i]){
                temFilhos = true;
                desenharOctree(octree->filhos[i], corFilhos, corSemFilhos);
            }
        }

        Color cor;
        if(temFilhos){
            cor = corFilhos;
        }else{
            cor = corSemFilhos;
        }
        DrawBoundingBox(octree->regiao, cor);
    }
}

/*--- KD-Tree ---*/

KDTree* BuildKDTree(BoundingBox regiao, std::vector<Patrimonio> patrimonios){

    if(patrimonios.empty()){
        //KD-Tree não tem patrimônios
        return nullptr;
    }

    auto kdtree = (KDTree*)malloc(sizeof(KDTree));

    if(patrimonios.size() == 1){
        //É um nó folha
        kdtree->regiao = regiao;
        kdtree->patrimonio = (Patrimonio*)malloc(sizeof(Patrimonio));
        *kdtree->patrimonio = patrimonios[0];
        kdtree->triangulo = nullptr;
        kdtree->menor = nullptr;
        kdtree->maior = nullptr;

    } else {
        //KD-Tree tem mais de um patrimônio, logo não é um nó folha
        int nPatrimonios = patrimonios.size();
        Vector3 media = {0.f, 0.f, 0.f};
        Vector3 centros[nPatrimonios];

        //Calcula a média dos centros
        for (int i = 0; i < nPatrimonios; i++) {
            BoundingBox bBox = patrimonios[i].bBox;
            centros[i] = Vector3Divide(Vector3Add(bBox.min, bBox.max), 2);
            media = Vector3Add(media, centros[i]);
        }

        media = Vector3Divide(media, nPatrimonios);

        //Calcula a variância dos centros
        Vector3 variancia = {0.f, 0.f, 0.f};
        for (auto &centro : centros) {
            Vector3 sub = Vector3Subtract(centro, media);
            variancia = Vector3Add(variancia, Vector3MultiplyV(sub, sub));
        }
        variancia = Vector3Divide(variancia, nPatrimonios-1);

        //Define o eixo de divisão
        Eixo eixo = X;
        float valorEixo = media.x;
        if(variancia.y > variancia.z && variancia.y > variancia.x){
            eixo = Y;
            valorEixo = media.y;
        }else if(variancia.z > variancia.y && variancia.z > variancia.x){
            eixo = Z;
            valorEixo = media.z;
        }

        //Divide os patrimônios a partir do eixo
        std::vector<Patrimonio> patrimoniosMenor;
        std::vector<Patrimonio> patrimoniosMaior;
        for(int i = 0; i < nPatrimonios; i++){
            switch(eixo){
                case X:
                    if(centros[i].x <= valorEixo){
                        patrimoniosMenor.push_back(patrimonios[i]);
                    }
                    if(centros[i].x >= valorEixo){
                        patrimoniosMaior.push_back(patrimonios[i]);
                    }
                    break;
                case Y:
                    if(centros[i].y <= valorEixo){
                        patrimoniosMenor.push_back(patrimonios[i]);
                    }
                    if(centros[i].y >= valorEixo){
                        patrimoniosMaior.push_back(patrimonios[i]);
                    }
                    break;
                case Z:
                    if(centros[i].z <= valorEixo){
                        patrimoniosMenor.push_back(patrimonios[i]);
                    }
                    if(centros[i].z >= valorEixo){
                        patrimoniosMaior.push_back(patrimonios[i]);
                    }
                    break;
            }
        }

        kdtree->eixo = eixo;
        kdtree->valorEixo = valorEixo;
        kdtree->regiao = regiao;
        BoundingBox regiaoMenor = regiao;
        BoundingBox regiaoMaior = regiao;
        switch (eixo){
            case X:
                regiaoMenor.max.x = valorEixo;
                regiaoMaior.min.x = valorEixo;
                break;
            case Y:
                regiaoMenor.max.y = valorEixo;
                regiaoMaior.min.y = valorEixo;
                break;
            case Z:
                regiaoMenor.max.z = valorEixo;
                regiaoMaior.min.z = valorEixo;
                break;
        }
        kdtree->menor = BuildKDTree(regiaoMenor, patrimoniosMenor);
        kdtree->maior = BuildKDTree(regiaoMaior, patrimoniosMaior);
        kdtree->patrimonio = nullptr;
        kdtree->triangulo = nullptr;
    }

    return kdtree;
}

KDTree* BuildKDTreeTriangulos(BoundingBox regiao, std::vector<Patrimonio> patrimonios){
    std::vector<Triangulo> triangulos;
    for(auto &patrimonio: patrimonios){
        auto ponteiro = (Patrimonio*)malloc(sizeof(Patrimonio));
        *ponteiro = patrimonio;
        Model* model = &patrimonio.model;
        //Gambiarra por cause de um bug no Raylib, triangleCount vem sempre zero
        int nTriangulos = model->mesh.vertexCount/3;

        for(int i = 0; i < nTriangulos; i++){
            Vector3 a, b, c;
            auto vertdata = (Vector3 *)model->mesh.vertices;

            if (model->mesh.indices)
            {
                a = vertdata[model->mesh.indices[i*3 + 0]];
                b = vertdata[model->mesh.indices[i*3 + 1]];
                c = vertdata[model->mesh.indices[i*3 + 2]];
            }
            else
            {
                a = vertdata[i*3 + 0];
                b = vertdata[i*3 + 1];
                c = vertdata[i*3 + 2];
            }

            a = Vector3Transform(a, model->transform);
            b = Vector3Transform(b, model->transform);
            c = Vector3Transform(c, model->transform);

            triangulos.push_back((Triangulo){a, b, c, ponteiro});
        }
    }

    return BuildKDTree(regiao, triangulos);
}

KDTree* BuildKDTree(BoundingBox regiao, std::vector<Triangulo> triangulos){

    if(triangulos.empty()){
        //KD-Tree não tem triângulos
        return nullptr;
    }

    auto kdtree = (KDTree*)malloc(sizeof(KDTree));

    if(triangulos.size() == 1){
        //É um nó folha
        kdtree->regiao = regiao;
        kdtree->triangulo = (Triangulo*)malloc(sizeof(Triangulo));
        *kdtree->triangulo = triangulos[0];
        kdtree->patrimonio = nullptr;
        kdtree->menor = nullptr;
        kdtree->maior = nullptr;

    } else {
        //KD-Tree tem mais de um triângulo, logo não é um nó folha
        int nTriangulos = triangulos.size();
        Vector3 media = {0.f, 0.f, 0.f};
        Vector3 centros[nTriangulos];

        //Calcula a média dos centros
        for (int i = 0; i < nTriangulos; i++) {
            Triangulo t = triangulos[i];
            centros[i] = Vector3Divide(Vector3Add(Vector3Add(t.v1, t.v2), t.v3), 3);
            media = Vector3Add(media, centros[i]);
        }

        media = Vector3Divide(media, nTriangulos);

        //Calcula a variância dos centros
        Vector3 variancia = {0.f, 0.f, 0.f};
        for (auto &centro : centros) {
            Vector3 sub = Vector3Subtract(centro, media);
            variancia = Vector3Add(variancia, Vector3MultiplyV(sub, sub));
        }
        variancia = Vector3Divide(variancia, nTriangulos-1);

        //Define o eixo de divisão
        Eixo eixo = X;
        float valorEixo = media.x;
        if(variancia.y > variancia.z && variancia.y > variancia.x){
            eixo = Y;
            valorEixo = media.y;
        }else if(variancia.z > variancia.y && variancia.z > variancia.x){
            eixo = Z;
            valorEixo = media.z;
        }

        //Divide os triângulos a partir do eixo
        std::vector<Triangulo> triangulosMenor;
        std::vector<Triangulo> triangulosMaior;
        for(int i = 0; i < nTriangulos; i++){
            switch(eixo){
                case X:
                    if(centros[i].x <= valorEixo){
                        triangulosMenor.push_back(triangulos[i]);
                    }
                    if(centros[i].x >= valorEixo){
                        triangulosMaior.push_back(triangulos[i]);
                    }
                    break;
                case Y:
                    if(centros[i].y <= valorEixo){
                        triangulosMenor.push_back(triangulos[i]);
                    }
                    if(centros[i].y >= valorEixo){
                        triangulosMaior.push_back(triangulos[i]);
                    }
                    break;
                case Z:
                    if(centros[i].z <= valorEixo){
                        triangulosMenor.push_back(triangulos[i]);
                    }
                    if(centros[i].z >= valorEixo){
                        triangulosMaior.push_back(triangulos[i]);
                    }
                    break;
            }
        }

        kdtree->eixo = eixo;
        kdtree->valorEixo = valorEixo;
        kdtree->regiao = regiao;
        BoundingBox regiaoMenor = regiao;
        BoundingBox regiaoMaior = regiao;
        switch (eixo){
            case X:
                regiaoMenor.max.x = valorEixo;
                regiaoMaior.min.x = valorEixo;
                break;
            case Y:
                regiaoMenor.max.y = valorEixo;
                regiaoMaior.min.y = valorEixo;
                break;
            case Z:
                regiaoMenor.max.z = valorEixo;
                regiaoMaior.min.z = valorEixo;
                break;
        }
        kdtree->menor = BuildKDTree(regiaoMenor, triangulosMenor);
        kdtree->maior = BuildKDTree(regiaoMaior, triangulosMaior);
        kdtree->patrimonio = nullptr;
        kdtree->triangulo = nullptr;
    }

    return kdtree;
}

void UnloadKDTree(KDTree* kdtree){
    if(kdtree != nullptr) {
        if (kdtree->menor != nullptr) {
            UnloadKDTree(kdtree->menor);
        }
        if (kdtree->maior != nullptr) {
            UnloadKDTree(kdtree->maior);
        }
        if (kdtree->patrimonio != nullptr) {
            free(kdtree->patrimonio);
        }
        if (kdtree->triangulo != nullptr) {
            free(kdtree->triangulo->patrimonio);
            free(kdtree->triangulo);
        }

        free(kdtree);
    }
}

bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray, KDTree* kdtree){
    if(kdtree != nullptr && CheckCollisionRayBox(ray, patrimonio.bBox)) {

        RayHitInfo patrimonioHitInfo = GetCollisionRayModel(ray, &patrimonio.model);
        if (patrimonioHitInfo.hit) {
            return !existeUmPatrimonioMaisProximo(patrimonio.id, patrimonioHitInfo.distance, ray, kdtree);
        }
    }
    return false;
}

bool existeUmPatrimonioMaisProximo(int patrimonioIndex, float patrimonioDistance, Ray ray, KDTree* kdtree){
    if(kdtree != nullptr && CheckCollisionRayBox(ray, kdtree->regiao)){

        Triangulo* triangulo = kdtree->triangulo;
        Patrimonio* patrimonio = kdtree->patrimonio;
        if(triangulo != nullptr && triangulo->patrimonio->id != patrimonioIndex){
            //Se tiver um triângulo, é um nó folha
            auto hitInfo = GetCollisionRayTriangle(ray, triangulo->v1, triangulo->v2, triangulo->v3);
            if(hitInfo.hit && hitInfo.distance < patrimonioDistance){
                return true;
            }
        }else if(patrimonio != nullptr && patrimonio->id != patrimonioIndex){
            //Se tiver um patrimônio, é um nó folha
            if(CheckCollisionRayBox(ray, patrimonio->bBox)){
                auto hitInfo = GetCollisionRayModel(ray, &patrimonio->model);
                if(hitInfo.hit && hitInfo.distance < patrimonioDistance){
                    return true;
                }
            }
        }

        return existeUmPatrimonioMaisProximo(patrimonioIndex, patrimonioDistance, ray, kdtree->menor) ||
               existeUmPatrimonioMaisProximo(patrimonioIndex, patrimonioDistance, ray, kdtree->maior);

    }

    return false;
}

int indexPatrimonioMaisProximo(Ray ray, KDTree *kdtree){

    IndexDistance indexDistance = {-1, 3.40282347E+38f};
    return indexDistanceMaisProximo(indexDistance, ray, kdtree).index;
}

IndexDistance indexDistanceMaisProximo(IndexDistance indexDistance, Ray ray, KDTree *kdtree){
    if(kdtree != nullptr && CheckCollisionRayBox(ray, kdtree->regiao)){

        Triangulo* triangulo = kdtree->triangulo;
        Patrimonio* patrimonio = kdtree->patrimonio;
        if(triangulo != nullptr && triangulo->patrimonio->id != indexDistance.index){
            //Se tiver um triângulo, é um nó folha
            auto hitInfo = GetCollisionRayTriangle(ray, triangulo->v1, triangulo->v2, triangulo->v3);
            if(hitInfo.hit && hitInfo.distance < indexDistance.distance){
                indexDistance = (IndexDistance){triangulo->patrimonio->id, hitInfo.distance};
            }
        }else if(patrimonio != nullptr && patrimonio->id != indexDistance.index){
            //Se tiver um patrimônio, é um nó folha
            if(CheckCollisionRayBox(ray, patrimonio->bBox)){
                auto hitInfo = GetCollisionRayModel(ray, &patrimonio->model);
                if(hitInfo.hit && hitInfo.distance < indexDistance.distance){
                    indexDistance = (IndexDistance){patrimonio->id, hitInfo.distance};
                }
            }
        }

        IndexDistance indexMenor = indexDistanceMaisProximo(indexDistance, ray, kdtree->menor);
        if(indexMenor.distance < indexDistance.distance){
            indexDistance = indexMenor;
        }

        IndexDistance indexMaior = indexDistanceMaisProximo(indexDistance, ray, kdtree->maior);
        if(indexMaior.distance < indexDistance.distance){
            indexDistance = indexMaior;
        }

    }

    return indexDistance;
}

void desenharKDTree(KDTree *kdtree, Color corFilhos, Color corSemFilhos){
    if(kdtree != nullptr){
        desenharKDTree(kdtree->menor, corFilhos, corSemFilhos);
        desenharKDTree(kdtree->maior, corFilhos, corSemFilhos);

        Color cor;
        if(kdtree->patrimonio == nullptr){
            cor = corSemFilhos;
        }else{
            cor = corFilhos;
        }
        DrawBoundingBox(kdtree->regiao, cor);
    }
}