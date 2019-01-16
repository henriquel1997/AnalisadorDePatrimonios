//
// Created by Henrique on 08/01/2019.
//
#include "raylib.h"

struct Patrimonio{
    int id;
    char* nome;
    Model model;
    BoundingBox bBox;
};

struct Triangulo{
    Vector3 v1;
    Vector3 v2;
    Vector3 v3;
    Patrimonio* patrimonio;
};