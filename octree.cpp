//
// Created by Henrique on 08/01/2019.
//

#import "structs.cpp"
#include <vector>
#include <cstdlib>

struct Octree{
    Octree* parent;
    Octree* childs[8];
    unsigned char activeChilds;
    BoundingBox region;
    std::vector<Patrimonio> objects;
};

float min_octree_size = 0.1f;
Octree* root;

//Libera a memória da octree e de seus filhos
void UnloadOctree(Octree* octree){
    if(octree == nullptr){
        if(root != nullptr){
            octree = root;
        }else{
            return;
        }
    }

    free(octree->parent);
    for (auto &child : octree->childs) {
        UnloadOctree(child);
        free(child);
    }
}

