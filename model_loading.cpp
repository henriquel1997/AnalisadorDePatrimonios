//
// Created by henrique on 21/01/19.
//

#include "model_loading.h"

std::vector<Patrimonio> importarModelo(const char * pFile){

    std::vector<Patrimonio> patrimonios;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( pFile,
                                              aiProcess_CalcTangentSpace       |
                                              aiProcess_Triangulate            |
                                              aiProcess_JoinIdenticalVertices  |
                                              aiProcess_SortByPType);

    if(!scene){
        printf(importer.GetErrorString());
        return patrimonios;
    }

//    aiMatrix4x4 transform = aiMatrix4x4(
//            1.f, 0.f, 0.f, 0.f,
//            0.f, 0.f, 1.f, 0.f,
//            0.f, -1.f, 0.f, 0.f,
//            0.f, 0.f, 0.f, 1.f
//    );

    aiMatrix4x4 identity;

    return PatrimonioFromNode(0, scene, identity, scene->mRootNode);
}

std::vector<Patrimonio> PatrimonioFromNode(int initialID, const aiScene* scene, aiMatrix4x4 parentTransform, aiNode* node){

    std::vector<Patrimonio> patrimonios;

    aiMatrix4x4 transform = node->mTransformation * parentTransform;

    int cont = initialID;
    while(cont < node->mNumMeshes){

        int meshIndex = node->mMeshes[cont];
        aiMesh* mesh = scene->mMeshes[meshIndex];

        printf("Nome modelo[%d]: ", cont);
        printf(node->mName.C_Str());
        printf("\n");

        patrimonios.push_back(PatrimonioFromMesh(cont, node->mName.C_Str(), transform, mesh));
        cont++;
    }

    for(int i = 0; i < node->mNumChildren; i++){
        auto childVector = PatrimonioFromNode(cont, scene, transform, node->mChildren[i]);
        patrimonios.insert( patrimonios.end(), childVector.begin(), childVector.end() );
    }

    return patrimonios;
}

Patrimonio PatrimonioFromMesh(int id, const char* nome, aiMatrix4x4 transform, aiMesh* mesh){

    Model model;
    int nFaces = mesh->mNumFaces;
    model.mesh.triangleCount = nFaces;
    model.mesh.vertexCount = nFaces * 3;
    Matrix matrix = MatrixIdentity();
    model.transform = matrix;
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
                vertices[vCounter + 1] = vertex.z;
                vertices[vCounter + 2] = -vertex.y;
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
    patrimonio.bBox = MeshBoundingBox(model.mesh);

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