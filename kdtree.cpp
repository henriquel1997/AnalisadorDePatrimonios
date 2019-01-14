//
// Created by Henrique on 14/01/2019.
//

#include <raymath.h>
#include <cstdlib>
#include "kdtree.h"

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
        kdtree->patrimonio = &patrimonios[0];
        kdtree->menor = nullptr;
        kdtree->maior = nullptr;

    }else if(!patrimonios.empty()){
        //KD-Tree tem mais de um patrimônio, logo não é um nó folha
        int nPatrimonios = patrimonios.size();
        Vector3 media = {0.f, 0.f, 0.f};
        Vector3 centros[nPatrimonios];

        //Calcula a média dos centros
        for (int i =0; i < nPatrimonios; i++) {
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
        //TODO: Decidir como dividir as regiões a partir do eixo
        BoundingBox regiaoMenor;
        BoundingBox regiaoMaior;
        kdtree->menor = BuildKDTree(regiaoMenor, patrimoniosMenor);
        kdtree->maior = BuildKDTree(regiaoMaior, patrimoniosMaior);
    }

    return kdtree;
}