#include <vector>
#include <time.h>
#include "math.h"
#include "raylib.h"
#include "raymath.h"
#include "model_loading.h"

int screenWidth = 1600;
int screenHeight = 900;
Camera camera = { 0 };
Vector3 defaultTarget = {0.f, 0.f, 0.f};
Vector3 oldMouseDirection;
float mouseSpeedMultiplier = 5.f;
std::vector<Patrimonio> patrimonios;
const Vector3 centro = {0.f, 0.f, 0.f};
const Vector3 up = { 0.0f, 1.0f, 0.0f };

//Variáveis do algoritmo de visibilidade
Vector3 posPessoa = {0.f, .25f, 0.f}; //Y é a altura da pessoa
std::vector<Vector3> pontosPatrimonio;
std::vector<Vector3> raios;
std::vector<bool> raioAcertou;
std::vector<Vector2> pontosVisiveisChao;
int patrimonioIndex = -1;
bool executaAlgoritmo = false;
bool avancarAlgoritmo = false; //Passo a passo
bool animado = true;
bool mostrarRaios = false;
int passoAlgoritmo = 0;
int numeroQuadrados = 50; //Número de quadrados em uma linha da grid
float tamanhoGrid = 5; //Tamanho de uma linha da grid
int raiosPorPonto = 500;
float fov = 15.f;
int pontoPatrimonioCamera = 0;
time_t tempoInicio;

Texture2D texturaChao = {};
Model chaoModel = {};

enum TipoArvore {
    OCTREE, KDTREE, KDTREE_TRI
};

TipoArvore tipoArvore = KDTREE;
Octree* octree = nullptr;
KDTree* kdtree = nullptr;
bool desenhaArvore = false;
bool mostrarBoundingBox = false;

void getInput();
void inicializarArvore();
void inicializarKDTree();
void inicializarOctree();
void unloadArvore();
BoundingBox boundingBoxGrid();
bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray);
int indexPatrimonioMaisProximo(Ray ray);
void carregarChao();
Patrimonio getPatrimonio(int id);
int getModelHitIndex(Ray ray);
void algoritmoVisibilidade();
void desenharChao();
void desenharRaios();
void desenharModelos();
void desenharArvore();
void seguirPessoa();
bool cameraSeguindoPessoa();
bool estaDentroDeUmPatrimonio();
void initRand();
float float_rand(float min, float max);

int main() {
    // Initialization
    //--------------------------------------------------------------------------------------

    initRand();

    InitWindow(screenWidth, screenHeight, "Raycast");
    //SetTargetFPS(60);

    // Define the camera to look into our 3d world
    camera.position = (Vector3){ 7.0f, 7.0f, 7.0f };    // Camera position
    //camera.position = (Vector3){ 500.0f, 500.0f, 500.0f };    // Camera position
    camera.target = defaultTarget;                      // Camera looking at point
    camera.up = up;                                     // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.type = CAMERA_PERSPECTIVE;                   // Camera mode type

    patrimonios = importarModelo((char *)R"(../models/CentroFortaleza.fbx)", boundingBoxGrid());
    if(!patrimonios.empty()){
        printf("Modelo importado\n");
    }else{
        printf("Modelo nao importado\n");
    }

    carregarChao();

    inicializarArvore();

    //--------------------------------------------------------------------------------------


    // Main loop
    while (!WindowShouldClose())  {  // Detect window close button or ESC key
        if(!IsWindowMinimized()) {
            // Update
            //----------------------------------------------------------------------------------
            getInput();
            if(executaAlgoritmo || avancarAlgoritmo){
                algoritmoVisibilidade();
            }
            Vector3 position = posPessoa;
            position.y = 0;
            //----------------------------------------------------------------------------------

            // Draw
            //----------------------------------------------------------------------------------
            BeginDrawing();

            ClearBackground(DARKBLUE);

            UpdateCamera(&camera);

            BeginMode3D(camera);

            //Desenha a pessoa
            DrawLine3D(position, posPessoa, RED);

            //Desenha os modelos
            desenharModelos();

            desenharRaios();

            //Desenha na textura do chão para representar a visibilidade daquela posição
            desenharChao();

            desenharArvore();

            DrawGrid(numeroQuadrados, tamanhoGrid/numeroQuadrados);

            DrawGizmo(centro);

            EndMode3D();

            DrawFPS(10, 10);

            EndDrawing();
            //----------------------------------------------------------------------------------
        }
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    unloadArvore();

    for(auto &patrimonio : patrimonios) {
        UnloadModel(patrimonio.model);
        free(patrimonio.nome);
    }

    UnloadModel(chaoModel);
    UnloadTexture(texturaChao);

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void inicializarArvore(){
    switch (tipoArvore){
        case OCTREE:
            inicializarOctree();
            break;
        case KDTREE:
        case KDTREE_TRI:
            inicializarKDTree();
            break;
    }
}

void inicializarKDTree(){
    if(kdtree != nullptr){
        UnloadKDTree(kdtree);
        kdtree = nullptr;
        printf("KDTree desalocada\n");
    }

    printf("Comecando a construir a KDTree\n");
    time_t tempoInicio = time(nullptr);
    if(tipoArvore == KDTREE){
        kdtree = BuildKDTree(boundingBoxGrid(), patrimonios);
    }else{
        kdtree = BuildKDTreeTriangulos(boundingBoxGrid(), patrimonios);
    }
    printf("Tempo para gerar a KDTree: %f(s)\n", difftime(time(nullptr), tempoInicio));
}

void inicializarOctree(){
    if(octree != nullptr){
        UnloadOctree(octree);
        kdtree = nullptr;
        printf("Octree desalocada\n");
    }

    printf("Comecando a construir a Octree\n");
    time_t tempoInicio = time(nullptr);
    octree = BuildOctree(boundingBoxGrid(), patrimonios);
    printf("Tempo para gerar a Octree: %f(s)\n", difftime(time(nullptr), tempoInicio));
}

void unloadArvore(){
    switch (tipoArvore){
        case OCTREE:
            UnloadOctree(octree);
            octree = nullptr;
            printf("Octree desalocada\n");
            break;
        case KDTREE:
        case KDTREE_TRI:
            UnloadKDTree(kdtree);
            kdtree = nullptr;
            printf("KDTree desalocada\n");
            break;
    }
}

BoundingBox boundingBoxGrid(){
    float metade = tamanhoGrid/2;
    Vector3 min = (Vector3){-metade, -metade, -metade};
    Vector3 max = (Vector3){metade, metade, metade};
    return (BoundingBox){min, max};
}

bool isPatrimonioTheClosestHit(Patrimonio patrimonio, Ray ray){
    switch (tipoArvore){
        case OCTREE:
            return isPatrimonioTheClosestHit(patrimonio, ray, octree);
        case KDTREE:
        case KDTREE_TRI:
            return isPatrimonioTheClosestHit(patrimonio, ray, kdtree);
    }
}

int indexPatrimonioMaisProximo(Ray ray){
    switch (tipoArvore){
        case OCTREE:
            return indexPatrimonioMaisProximo(ray, octree);
        case KDTREE:
        case KDTREE_TRI:
            return indexPatrimonioMaisProximo(ray, kdtree);
    }
}

void carregarChao(){
    Image imagemChao = GenImageColor(numeroQuadrados, numeroQuadrados, GRAY);
    texturaChao = LoadTextureFromImage(imagemChao);
    UnloadImage(imagemChao);
    chaoModel = LoadModelFromMesh(GenMeshPlane(tamanhoGrid, tamanhoGrid, 5, 5));
    chaoModel.material.maps[MAP_DIFFUSE].texture = texturaChao;
}

void desenharChao(){
    int numPixels = numeroQuadrados * numeroQuadrados;
    Color pixels[numPixels];
    for (int i = 0; i < numPixels; i++) {
        pixels[i] = (Color){};
    }
    for (auto &ponto : pontosVisiveisChao) {
        int x = int(ponto.x);
        int y = int(ponto.y);
        pixels[x + (y*numeroQuadrados)] = YELLOW;
    }
    Image chao = LoadImageEx(pixels, numeroQuadrados, numeroQuadrados);
    UpdateTexture(texturaChao, chao.data);
    UnloadImage(chao);
    DrawModel(chaoModel, centro, 1.f, WHITE);
}

void desenharRaios(){
    if(mostrarRaios){
        for(int i = 0; i < raios.size(); i++){
            Color cor;
            if(raioAcertou[i]){
                cor = RED;
                DrawLine3D(posPessoa, raios[i], cor);
            }else{
                cor = ORANGE;
            }
        }
    }
}

void desenharModelos(){
    for (auto patrimonio : patrimonios) {
        Color cor;
        if(patrimonioIndex == patrimonio.id){
            cor = RED;
        }else{
            cor = BLACK;
        }

        if(mostrarBoundingBox){
            DrawBoundingBox(patrimonio.bBox, GREEN);
        }
        DrawModel(patrimonio.model, centro, 1.f, GRAY);
        DrawModelWires(patrimonio.model, centro, 1.f, cor);
    }
}

void desenharArvore(){
    if(desenhaArvore){
        switch (tipoArvore){
            case OCTREE:
                desenharOctree(octree, GREEN, RED);
                break;
            case KDTREE:
                desenharKDTree(kdtree, GREEN, RED);
                break;
        }
    }
}

void algoritmoVisibilidade(){
    if(patrimonioIndex >= 0 && tamanhoGrid >= 0 && !pontosPatrimonio.empty()) {

        if(passoAlgoritmo == 0){
            tempoInicio = time(nullptr);
        }

        bool seguindoPessoa = cameraSeguindoPessoa();

        float tamanhoQuadrado = tamanhoGrid/numeroQuadrados;
        float metadeGrid = tamanhoGrid/2;
        float metadeQuadrado = tamanhoQuadrado/2;
        int numeroQuadradosTotal = numeroQuadrados*numeroQuadrados;
        Patrimonio patrimonio = getPatrimonio(patrimonioIndex);

        if(passoAlgoritmo == 0){
            pontosVisiveisChao.clear();
        }

        raios.clear();
        raioAcertou.clear();

        for (int i = passoAlgoritmo; i < numeroQuadradosTotal; i++) {

            //Definindo a posição da pessoa na grid
            int quadradoX = i/numeroQuadrados;
            int quadradoY = i%numeroQuadrados;
            posPessoa.x = quadradoX*tamanhoQuadrado - metadeGrid + metadeQuadrado;
            posPessoa.z = quadradoY*tamanhoQuadrado - metadeGrid + metadeQuadrado;

            if(estaDentroDeUmPatrimonio()){
                passoAlgoritmo++;
                continue;
            }

            //Calculando a visibilidade aqui
            for(auto &ponto : pontosPatrimonio){
                Vector3 visao = Vector3Subtract(ponto, posPessoa);
                Vector3 vetorHorizontal = Vector3Normalize(Vector3CrossProduct(up, visao));
                Vector3 vetorVertical = Vector3Normalize(Vector3CrossProduct(visao, vetorHorizontal));
                float fovMul = Vector3Length(visao)*cos(fov/2);
                vetorHorizontal = Vector3Multiply(vetorHorizontal, fovMul);
                vetorVertical = Vector3Multiply(vetorVertical, fovMul);

                int cont = 0;
                for(int j = 0; j < raiosPorPonto; j++){
                    Vector3 vx = Vector3Multiply(vetorHorizontal, float_rand(-1.f, 1.f));
                    Vector3 vy = Vector3Multiply(vetorVertical, float_rand(-1.f, 1.f));
                    Vector3 pontoRaio = Vector3Add(Vector3Add(vx, vy), ponto);

                    Ray raio;
                    raio.position = posPessoa;
                    raio.direction = Vector3Subtract(pontoRaio, posPessoa);


                    bool acertou = false;
                    if(isPatrimonioTheClosestHit(patrimonio, raio)){
                        cont++;
                        acertou = true;
                    }

                    if(animado || avancarAlgoritmo){
                        raios.push_back(pontoRaio);
                        raioAcertou.push_back(acertou);
                    }

                    if(acertou){
                        break;
                    }
                }

                if(cont > 0){
                    Vector2 novoPonto = {(float)quadradoX, (float)quadradoY};
                    bool achou = false;
                    for(auto &pontoVisivel : pontosVisiveisChao){
                        if(pontoVisivel.x == novoPonto.x && pontoVisivel.y == novoPonto.y){
                            achou = true;
                            break;
                        }
                    }
                    if(!achou){
                        pontosVisiveisChao.push_back(novoPonto);
                    }
                }
            }

            //Incrementado o passo e verificando se é necessário continuar executando o algoritmo
            passoAlgoritmo++;

            if(i == numeroQuadradosTotal - 1){

                auto tempoFim = time(nullptr);

                executaAlgoritmo = false;

                printf("Pontos Visiveis:\n");
                for(auto &ponto : pontosVisiveisChao){
                    printf("%f, %f\n", ponto.x, ponto.y);
                }

                printf("Tempo algoritmo: %f(s)\n", difftime(tempoFim, tempoInicio));
            }

            if(animado || avancarAlgoritmo){
                if(seguindoPessoa){
                    seguirPessoa();
                }

                avancarAlgoritmo = false;
                break;
            }
        }
    }
}

void getInput(){
    Vector3 direction = Vector3Zero();

    //Move para frente
    if(IsKeyPressed(KEY_W)){
        Vector3 targetNormalized = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        direction = Vector3Add(targetNormalized, direction);
    }

    //Move para trás
    if(IsKeyPressed(KEY_S)){
        Vector3 targetNormalized = Vector3Normalize(Vector3Subtract(camera.position, camera.target));
        direction = Vector3Add(targetNormalized, direction);
    }

    //Move para a direita
    if(IsKeyPressed(KEY_D)){
        Vector3 targetVector = Vector3Subtract(camera.target, camera.position);
        Vector3 targetNormalized = Vector3Normalize(Vector3CrossProduct(targetVector, camera.up));
        direction = Vector3Add(targetNormalized, direction);
        camera.target = Vector3Add(targetNormalized, camera.target);
    }

    //Move para a esquerda
    if(IsKeyPressed(KEY_A)){
        Vector3 targetVector = Vector3Subtract(camera.target, camera.position);
        Vector3 targetNormalized = Vector3Normalize(Vector3CrossProduct(camera.up, targetVector));
        direction = Vector3Add(targetNormalized, direction);
        camera.target = Vector3Add(targetNormalized, camera.target);
    }

    //Move para cima
    if(IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)){
        direction = Vector3Add(up, direction);
        camera.target = Vector3Add(up, camera.target);
    }

    //Move para baixo
    if(IsKeyPressed(KEY_X)){
        Vector3 down = Vector3Negate(up);
        direction = Vector3Add(down, direction);
        camera.target = Vector3Add(down, camera.target);
    }


    camera.position = Vector3Add(direction, camera.position);

    //Controla a direção (target) pelo mouse quando o shift esquerdo estiver pressionado
    bool shiftPressed = IsKeyPressed(KEY_LEFT_SHIFT);
    if(IsKeyDown(KEY_LEFT_SHIFT) || shiftPressed){
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);

        if(shiftPressed){
            oldMouseDirection = mouseRay.direction;
        }

        Vector3 mouseVector = Vector3Multiply(Vector3Subtract(mouseRay.direction, oldMouseDirection), mouseSpeedMultiplier);
        camera.target = Vector3Add(camera.target, mouseVector);
        oldMouseDirection = mouseRay.direction;
    }

    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        Ray ray = GetMouseRay(GetMousePosition(), camera);
        int index = indexPatrimonioMaisProximo(ray);
        if(index >= 0){
            Patrimonio patrimonio;
            bool achou = false;
            for(auto p : patrimonios){
                if(p.id == index){
                    achou = true;
                    patrimonio = p;
                }
            }
            if(achou){
                printf("Nome do arquivo do modelo: %s\n", patrimonio.nome);
                printf("Index: %i\n", index);
                patrimonioIndex = index;
                float* vertices = patrimonio.model.mesh.vertices;
                pontosPatrimonio.clear();
                for(int i = 0; i < patrimonio.model.mesh.vertexCount; i += 3){
                    float x = vertices[i];
                    float y = vertices[i+1];
                    float z = vertices[i+2];
                    Vector3 vertex = Vector3Transform((Vector3){x, y, z}, patrimonio.model.transform);
                    pontosPatrimonio.push_back(vertex);
                    if(patrimonio.model.mesh.vertexCount < 50){
                        printf("Ponto Patrimonio: %i: %f, %f, %f\n", (i/3)+1, vertex.x, vertex.y, vertex.z);
                    }
                }
            }
        }else{
            printf("Nao atingiu um modelo");
        }
    }

    if(IsKeyPressed(KEY_R)){
        camera.target = defaultTarget;
    }

    if(IsKeyPressed(KEY_ENTER)){
        executaAlgoritmo = true;
        passoAlgoritmo = 0;
    }

    if(IsKeyPressed(KEY_RIGHT)){
        avancarAlgoritmo = true;
    }

    if(IsKeyPressed(KEY_UP)){
        avancarAlgoritmo = true;
        passoAlgoritmo = 0;
    }

    if(IsKeyPressed(KEY_C)){
        seguirPessoa();
    }

    if(IsKeyPressed(KEY_L)){
        mostrarRaios = !mostrarRaios;
    }

    if(IsKeyPressed(KEY_N)){
        animado = !animado;
    }

    if(IsKeyPressed(KEY_T)){
        desenhaArvore = !desenhaArvore;
    }

    if(IsKeyPressed(KEY_DELETE)){
        if(patrimonioIndex != -1){
            for(int i = 0; i < patrimonios.size(); i++){
                if(patrimonios[i].id == patrimonioIndex){
                    patrimonios.erase(patrimonios.begin() + i);
                    patrimonioIndex = -1;
                    inicializarArvore();
                    break;
                }
            }
        }
    }

    if(IsKeyPressed(KEY_B)){
        mostrarBoundingBox = !mostrarBoundingBox;
    }
}

Patrimonio getPatrimonio(int id){
    if(id > -1){
        for(int i = 0; i < patrimonios.size(); i++){
            if(patrimonios[i].id == patrimonioIndex){
                return patrimonios[i];
            }
        }
    }
}

int getModelHitIndex(Ray ray){
    float lowestDistance = -1.f;
    int modelHitIndex = -1;
    for (auto patrimonio : patrimonios) {
        if(CheckCollisionRayBox(ray, patrimonio.bBox)){
            RayHitInfo hitInfo = GetCollisionRayModel(ray, &patrimonio.model);
            if(hitInfo.hit && (hitInfo.distance < lowestDistance || lowestDistance < 0)){
                lowestDistance = hitInfo.distance;
                modelHitIndex = patrimonio.id;
            }
        }
    }
    return modelHitIndex;
}

void seguirPessoa(){
    if(!pontosPatrimonio.empty()) {
        camera.position = posPessoa;
        camera.target = pontosPatrimonio[pontoPatrimonioCamera];

        pontoPatrimonioCamera++;
        if (pontoPatrimonioCamera > pontosPatrimonio.size()) {
            pontoPatrimonioCamera = 0;
        }
    }
}

bool cameraSeguindoPessoa(){
    return camera.position.x == posPessoa.x && camera.position.y == posPessoa.y && camera.position.z == posPessoa.z;
}

bool estaDentroDeUmPatrimonio(){
    Ray raio;
    raio.position = posPessoa;
    raio.direction = Vector3Negate(up);

    return getModelHitIndex(raio) >= 0;
}

void initRand(){
    srand(time(NULL));
}

float float_rand( float min, float max ) {
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}