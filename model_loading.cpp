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

    for(int i = 0; i < scene->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[i];

        printf("Nome modelo[%d]: ", i);
        printf(mesh->mName.C_Str());
        printf("\n");

        patrimonios.push_back(PatrimonioFromMesh(i, mesh));
    }


    return patrimonios;
}

Patrimonio PatrimonioFromMesh(int id, aiMesh* mesh){

    Model model;
    int nFaces = mesh->mNumFaces;
    model.mesh.triangleCount = nFaces;
    model.mesh.vertexCount = nFaces * 3;
    model.transform = MatrixIdentity();
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
        for(int j = 0; j < nFaces; j++){

            aiFace face = mesh->mFaces[j];

            aiVector3D vertex;

            vertex = mesh->mVertices[face.mIndices[0]];
            vertices[vCounter + 0] = vertex.x;
            vertices[vCounter + 1] = vertex.y;
            vertices[vCounter + 2] = vertex.z;
            vCounter += 3;

            vertex = mesh->mVertices[face.mIndices[1]];
            vertices[vCounter + 0] = vertex.x;
            vertices[vCounter + 1] = vertex.y;
            vertices[vCounter + 2] = vertex.z;
            vCounter += 3;

            vertex = mesh->mVertices[face.mIndices[2]];
            vertices[vCounter + 0] = vertex.x;
            vertices[vCounter + 1] = vertex.y;
            vertices[vCounter + 2] = vertex.z;
            vCounter += 3;
        }
        model.mesh.vertices = vertices;

        rlLoadMesh(&model.mesh, false);
    }

    Patrimonio patrimonio = {};
    patrimonio.id = id;
    patrimonio.nome = copy((char*)mesh->mName.C_Str());
    patrimonio.model = model;
    patrimonio.bBox = MeshBoundingBox(model.mesh);

    return patrimonio;
}