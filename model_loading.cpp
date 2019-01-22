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
    Matrix matrix = {transform.a1, transform.a2, transform.a3, transform.a4,
                     transform.b1, transform.b2, transform.b3, transform.b4,
                     transform.c1, transform.c2, transform.c3, transform.c4,
                     transform.d1, transform.d2, transform.d3, transform.d4};
    //Matrix matrix = MatrixIdentity();
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

            aiVector3D vertex;

            for(int j = 0; j < 3; j++){
                vertex = mesh->mVertices[face.mIndices[j]];
                //vertex = multiplyByMatrix(vertex, transform);
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
    patrimonio.bBox = MeshBoundingBox(model.mesh);

    return patrimonio;
}

aiVector3D multiplyByMatrix(aiVector3D vec, aiMatrix4x4 mat){
    aiVector3D result;
    aiVector3D scaling;
    aiVector3D rotation;
    aiVector3D position;
    mat.Decompose(scaling, rotation, position);
    result = scaling * rotation * position;
//    float w = 1;
//    result.x = mat.a1*vec.x + mat.b1*vec.y + mat.c1*vec.z + mat.d1*w;
//    result.y = mat.a2*vec.x + mat.b2*vec.y + mat.c2*vec.z + mat.d2*w;
//    result.z = mat.a3*vec.x + mat.b3*vec.y + mat.c3*vec.z + mat.d3*w;
//    float resultW = mat.a4*vec.x + mat.b4*vec.y + mat.c4*vec.z + mat.d4*w;
//    result.x /= resultW;
//    result.y /= resultW;
//    result.z /= resultW;
    return result;
}