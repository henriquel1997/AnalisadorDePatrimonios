#include <vector>
#include <time.h>
#include "dirent.h"
#include "math.h"
#include "raylib.h"
#include "raymath.h"
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include "structs.cpp"

int screenWidth = 1600;
int screenHeight = 900;
Camera camera = { 0 };
Vector3 defaultTarget = {0.f, 0.f, 0.f};
Vector3 oldMouseDirection;
float mouseSpeedMultiplier = 5.f;
std::vector<Patrimonio> patrimonios;
std::vector<char*> nomeModelos;
const Vector3 centro = {0.f, 0.f, 0.f};
const Vector3 up = { 0.0f, 1.0f, 0.0f };

//Variáveis do algoritmo de visibilidade
Vector3 posPessoa = {0.f, .25f, 0.f}; //Y é a altura da pessoa
std::vector<Vector3> pontosPatrimonio;
std::vector<Vector3> raios;
std::vector<bool> raioAcertou;
std::vector<Vector2> pontosVisiveisChao;
std::vector<float> porcentagensPontoChao;
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

void getInput();
void carregarModelos(char* path);
void carregarChao();
int getModelHitIndex(Ray ray);
void algoritmoVisibilidade();
void desenharChao();
void desenharRaios();
void desenharModelos();
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
    camera.target = defaultTarget;                      // Camera looking at point
    camera.up = up;                                     // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.type = CAMERA_PERSPECTIVE;                   // Camera mode type

    carregarModelos((char *)R"(C:\Dev\raycast\models\centro\)");
    carregarChao();

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
    for(auto &patrimonio : patrimonios) {
        UnloadModel(patrimonio.model);
    }

    UnloadModel(chaoModel);
    UnloadTexture(texturaChao);

    for(auto &nome : nomeModelos) {
        free(nome);
    }

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
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
    auto pixels = (Color *) malloc(sizeof(Color) * numPixels);
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
    free(pixels);
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
    for (int i = 0; i < patrimonios.size(); i++){
        Color cor;
        if(patrimonioIndex == i){
            cor = RED;
        }else{
            cor = BLACK;
        }

        DrawModel(patrimonios[i].model, centro, 1.f, GRAY);
        DrawModelWires(patrimonios[i].model, centro, 1.f, cor);
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
        Patrimonio patrimonio = patrimonios[patrimonioIndex];

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

                    //TODO: Otimizar velocidade
                    bool acertou = false;
                    if(GetCollisionRayModel(raio, &patrimonio.model).hit && getModelHitIndex(raio) == patrimonioIndex){
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

        if(shiftPressed){
            SetMousePosition((Vector2){screenWidth*.5f, screenHeight*.5f});
        }

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
        int index = getModelHitIndex(ray);
        if(index >= 0){
            printf("Nome do arquivo do modelo: %s\n", nomeModelos.at(index));
            printf("Index: %i\n", index);
            Model model = patrimonios.at(index).model;
            patrimonioIndex = index;
            float* vertices = model.mesh.vertices;
            pontosPatrimonio.clear();
            for(int i = 0; i < model.mesh.vertexCount; i += 3){
                float x = vertices[i];
                float y = vertices[i+1];
                float z = vertices[i+2];
                pontosPatrimonio.push_back((Vector3){x, y, z});
                printf("Ponto Patrimonio: %i: %f, %f, %f\n", (i/3)+1, x, y, z);
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

    if(IsKeyPressed(KEY_A)){
        animado = !animado;
    }
}

int getModelHitIndex(Ray ray){
    float lowestDistance = -1.f;
    int modelHitIndex = -1;
    for(int i = 0; i < patrimonios.size(); i++){
        Model modelo = patrimonios.at(i).model;
        RayHitInfo hitInfo = GetCollisionRayModel(ray, &modelo);
        if(hitInfo.hit && (hitInfo.distance < lowestDistance || lowestDistance < 0)){
            lowestDistance = hitInfo.distance;
            modelHitIndex = i;
        }
    }
    return modelHitIndex;
}

RayHitInfo getModelHitInfo(Ray ray){
    float lowestDistance = -1.f;
    RayHitInfo closestHitInfo = {};
    for (auto patrimonio : patrimonios) {
        RayHitInfo hitInfo = GetCollisionRayModel(ray, &patrimonio.model);
        if(hitInfo.hit && (hitInfo.distance < lowestDistance || lowestDistance < 0)){
            lowestDistance = hitInfo.distance;
            closestHitInfo = hitInfo;
        }
    }
    return closestHitInfo;
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

bool hasEndingString(char* const &fullString, char* const &ending) {
    if (strlen(fullString) >= strlen(ending)) {

        int pos = strlen(fullString) - strlen(ending);
        bool igual = true;
        for(int i = 0;  i < strlen(ending); i++){
            if(fullString[i + pos] != ending[i]){
                igual = false;
                break;
            }
        }

        return igual;
    } else {
        return false;
    }
}

char* concat(char* dest, char* source){
    int destSize = sizeof(dest) * strlen(dest);
    int sourceSize = sizeof(source) * strlen(source);
    auto buffer = (char*)malloc(static_cast<size_t>(destSize + sourceSize));
    sprintf(buffer, "%s%s", dest, source);
    return buffer;
}

char* copy(char* str){
    auto buffer = (char*)malloc(static_cast<size_t>(sizeof(str) * strlen(str)));
    sprintf(buffer, "%s", str);
    return buffer;
}

void carregarModelos(char* path) {

    DIR *dir = opendir(path);
    struct dirent *ent;

    if (dir != nullptr) {
        // Lista todos os arquivos do diretório

        while ((ent = readdir(dir)) != nullptr) {
            char* nome = ent->d_name;
            if(hasEndingString(nome, (char*)".obj")){
                //É um arquivo .obj
                auto fullPath = concat(path, nome);
                Model model = LoadModel(fullPath);
                BoundingBox bBox = MeshBoundingBox(model.mesh);
                Patrimonio patrimonio = {model, bBox};
                patrimonios.push_back(patrimonio);
                nomeModelos.push_back(copy(nome));
                free(fullPath);
            }
        }
        closedir(dir);

        printf("Modelos carregados com sucesso.\n");
    }else{
        printf("Nao foi possivel abrir o caminho.\n");
    }
}

void initRand(){
    srand(time(NULL));
}

float float_rand( float min, float max ) {
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}