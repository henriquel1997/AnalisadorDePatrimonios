//
// Created by henrique on 21/01/19.
//

#include "model_loading.h"

std::vector<Patrimonio> importarModelo(const char * pFile, BoundingBox boundingBox){

    std::vector<Patrimonio> patrimonios;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( pFile,
                                              aiProcess_FlipWindingOrder       |
                                              aiProcess_Triangulate            |
                                              aiProcess_JoinIdenticalVertices  |
                                              aiProcess_SortByPType);
    if(!scene){
        printf(importer.GetErrorString());
        return patrimonios;
    }

    aiMatrix4x4 identity;
    patrimonios = PatrimonioFromNode(0, scene, identity, getScaleMatrix(boundingBox, scene), scene->mRootNode);

    return patrimonios;
}

std::vector<Patrimonio> PatrimonioFromNode(int initialID, const aiScene* scene, aiMatrix4x4 parentTransform, Matrix modelTransform, aiNode* node){

    std::vector<Patrimonio> patrimonios;

    aiMatrix4x4 transform = node->mTransformation * parentTransform;
    for(int i = 0; i < node->mNumMeshes; i++){

        int meshIndex = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[meshIndex];

        printf("Nome modelo[%d]: ", i);
        printf(node->mName.C_Str());
        printf("\n");

        patrimonios.push_back(PatrimonioFromMesh(initialID + (int)patrimonios.size(), node->mName.C_Str(), transform, modelTransform, mesh));
    }

    for(int i = 0; i < node->mNumChildren; i++){
        auto childVector = PatrimonioFromNode(initialID + (int)patrimonios.size(), scene, transform,  modelTransform, node->mChildren[i]);
        patrimonios.insert( patrimonios.end(), childVector.begin(), childVector.end() );
    }

    return patrimonios;
}

Patrimonio PatrimonioFromMesh(int id, const char* nome, aiMatrix4x4 transform, Matrix modelTransform, aiMesh* mesh){

    Model model;
    int nFaces = mesh->mNumFaces;
    model.mesh.triangleCount = nFaces;
    model.mesh.vertexCount = nFaces * 3;
    model.transform = modelTransform;
    model.material = LoadMaterialDefault();

    model.mesh.normals = NULL;
    model.mesh.colors = NULL;
    model.mesh.indices = NULL;
    model.mesh.baseVertices = NULL;
    model.mesh.baseNormals = NULL;
    model.mesh.weightBias = NULL;
    model.mesh.weightId = NULL;
    model.mesh.tangents = NULL;
    model.mesh.texcoords = NULL;
    model.mesh.texcoords2 = NULL;

    if(nFaces > 0){
        auto vertices = (float*)malloc(sizeof(float) * nFaces * 9);
        int vCounter = 0;
        for(int i = 0; i < nFaces; i++){

            aiFace face = mesh->mFaces[i];

            for(int j = 0; j < 3; j++){
                auto vertex = multiplyByMatrix(mesh->mVertices[face.mIndices[j]], transform);
                vertices[vCounter + 0] = vertex.x;
                vertices[vCounter + 1] = vertex.y;
                vertices[vCounter + 2] = vertex.z;

                vCounter += 3;
            }
        }
        model.mesh.vertices = vertices;

        rlLoadMesh(&model.mesh, false);
    }

    Patrimonio patrimonio = {};
    patrimonio.id = id;
    patrimonio.nome = copy((char*)nome);
    patrimonio.model = model;
    auto bBox = MeshBoundingBox(patrimonio.model.mesh);
    bBox.min = Vector3Transform(bBox.min, modelTransform);
    bBox.max = Vector3Transform(bBox.max, modelTransform);
    patrimonio.bBox = bBox;

    return patrimonio;
}

aiVector3D multiplyByMatrix(aiVector3D vec, aiMatrix4x4 mat){
    aiVector3D result;
    result.x = mat.a1*vec.x + mat.a2*vec.y + mat.a3*vec.z + mat.a4;
    result.y = mat.b1*vec.x + mat.b2*vec.y + mat.b3*vec.z + mat.b4;
    result.z = mat.c1*vec.x + mat.c2*vec.y + mat.c3*vec.z + mat.c4;
    float resultW = mat.d1*vec.x + mat.d2*vec.y + mat.d3*vec.z + mat.d4;
    result.x /= resultW;
    result.y /= resultW;
    result.z /= resultW;
    return result;
}

Matrix getScaleMatrix(BoundingBox boundingBox, const aiScene* scene){
    Vector3 maior = {};
    Vector3 menor = {};
    for(int i = 0; i < scene->mNumMeshes; i++){

        auto mesh = scene->mMeshes[i];
        for(int j = 0; j < mesh->mNumVertices; j++){
            auto vertex = mesh->mVertices[j];

            if(vertex.x < menor.x){
                menor.x = vertex.x;
            }else if(vertex.x > maior.x){
                maior.x = vertex.x;
            }

            if(vertex.y < menor.y){
                menor.y = vertex.y;
            }else if(vertex.y > maior.y){
                maior.y = vertex.y;
            }

            if(vertex.z < menor.z){
                menor.z = vertex.z;
            }else if(vertex.z > maior.z){
                maior.z = vertex.z;
            }
        }
    }

    auto difAtual = Vector3Subtract(maior, menor);

    auto difBound = Vector3Subtract(boundingBox.max, boundingBox.min);

    float scaleFactor = Vector3Length(difBound)/Vector3Length(difAtual);

    return (Matrix){
            scaleFactor, 0.f, 0.f, 0.f,
            0.f, scaleFactor, 0.f, 0.f,
            0.f, 0.f, scaleFactor, 0.f,
            0.f, 0.f,         0.f, 1.f
    };
}