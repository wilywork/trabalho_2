#include <iostream>
#include <fstream>
#include <vector>
#include <GL/freeglut.h>
#include <string>
#include <sstream>

using namespace std;

// Variaveis globais
unsigned int modeloElefante;
vector<vector<float>> vertices, normais, coordenadasTextura;
vector<vector<int>> faces; // Armazena indices de vertices, texturas e normais
float rotacaoElefante;

const double PI = 3.14159265358979323846;
float deslocamentoX = 0.0f;
float deslocamentoY = 0.0f;
float escala = 1.0f;
float angulo = 0.0f;

void teclado(unsigned char tecla, int x, int y);

// Funcao para carregar o arquivo OBJ
void carregarObjeto(string nomeArquivo) {
    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cout << "Arquivo não encontrado: " << nomeArquivo << endl;
        exit(1);
    }

    string tipo;
    while (arquivo >> tipo) {
        if (tipo == "v") {
            // Leitura de vértices
            vector<float> vertice(3);
            arquivo >> vertice[0] >> vertice[1] >> vertice[2];
            vertices.push_back(vertice);
        }
        else if (tipo == "vn") {
            // Leitura de normais
            vector<float> normal(3);
            arquivo >> normal[0] >> normal[1] >> normal[2];
            normais.push_back(normal);
        }
        else if (tipo == "vt") {
            // Leitura de coordenadas de textura
            vector<float> coordenadaTextura(2);
            arquivo >> coordenadaTextura[0] >> coordenadaTextura[1];
            coordenadasTextura.push_back(coordenadaTextura);
        }
        else if (tipo == "f") {
            // Leitura de faces (armazenar índices de vértices, texturas e normais)
            string vertice_str;
            vector<int> faceVertices; // Armazena temporariamente todos os índices de uma face

            // Ler o restante da linha após 'f'
            getline(arquivo, vertice_str);
            if (vertice_str.empty()) {
                // Caso a face esteja na mesma linha que o tipo, ler novamente
                getline(arquivo, vertice_str);
            }

            // Variáveis para armazenar os índices temporariamente
            vector<int> indicesTemporarios;

            // Processar cada vértice da face
            size_t pos = 0;
            size_t len = vertice_str.length();
            while (pos < len) {
                // Ignorar espaços
                while (pos < len && vertice_str[pos] == ' ') pos++;

                if (pos >= len) break;

                // Encontrar o próximo espaço ou fim da string
                size_t start = pos;
                while (pos < len && vertice_str[pos] != ' ') pos++;
                size_t end = pos;

                // Extrair o token v/vt/vn
                string token = vertice_str.substr(start, end - start);

                // Inicializar índices
                int indiceVertice = -1, indiceTextura = -1, indiceNormal = -1;

                // Encontrar as posições dos '/'
                size_t pos1 = token.find('/');
                if (pos1 == string::npos) {
                    // Somente índice de vértice
                    indiceVertice = stoi(token);
                }
                else {
                    size_t pos2 = token.find('/', pos1 + 1);
                    if (pos2 == string::npos) {
                        // Formato v/vt
                        string verticeIdxStr = token.substr(0, pos1);
                        string texturaIdxStr = token.substr(pos1 + 1);

                        if (!verticeIdxStr.empty()) {
                            indiceVertice = stoi(verticeIdxStr);
                        }
                        if (!texturaIdxStr.empty()) {
                            indiceTextura = stoi(texturaIdxStr);
                        }
                    }
                    else {
                        // Formato v/vt/vn ou v//vn
                        string verticeIdxStr = token.substr(0, pos1);
                        string texturaIdxStr = "";
                        if (pos2 > pos1 + 1) {
                            texturaIdxStr = token.substr(pos1 + 1, pos2 - pos1 - 1);
                        }
                        string normalIdxStr = token.substr(pos2 + 1);

                        if (!verticeIdxStr.empty()) {
                            indiceVertice = stoi(verticeIdxStr);
                        }
                        if (!texturaIdxStr.empty()) {
                            indiceTextura = stoi(texturaIdxStr);
                        }
                        if (!normalIdxStr.empty()) {
                            indiceNormal = stoi(normalIdxStr);
                        }
                    }
                }

                // Ajustar índices negativos
                if (indiceVertice < 0) {
                    indiceVertice = vertices.size() + indiceVertice;
                }
                else {
                    indiceVertice -= 1; // OBJ é 1-based
                }

                if (indiceTextura < 0) {
                    indiceTextura = coordenadasTextura.size() + indiceTextura;
                }
                else if (indiceTextura > 0) {
                    indiceTextura -= 1; // OBJ é 1-based
                }

                if (indiceNormal < 0) {
                    indiceNormal = normais.size() + indiceNormal;
                }
                else if (indiceNormal > 0) {
                    indiceNormal -= 1; // OBJ é 1-based
                }

                // Armazenar os índices
                faceVertices.push_back(indiceVertice);
                faceVertices.push_back(indiceTextura);
                faceVertices.push_back(indiceNormal);
            }

            // Triangulação da face caso tenha mais de 3 vértices
            size_t numVertices = faceVertices.size() / 3;
            if (numVertices < 3) {
                cout << "Face com número de vértices inválido ou não suportado: " << numVertices << " vértices." << endl;
                continue;
            }

            // Triangulação em formato de leque (fan triangulation)
            for (size_t i = 1; i < numVertices - 1; ++i) {
                vector<int> triangulo(9); // 3 vértices, cada um com v/vt/vn

                // Primeiro vértice do triângulo
                triangulo[0] = faceVertices[0];
                triangulo[1] = faceVertices[1];
                triangulo[2] = faceVertices[2];

                // Segundo vértice do triângulo
                triangulo[3] = faceVertices[i * 3];
                triangulo[4] = faceVertices[i * 3 + 1];
                triangulo[5] = faceVertices[i * 3 + 2];

                // Terceiro vértice do triângulo
                triangulo[6] = faceVertices[(i + 1) * 3];
                triangulo[7] = faceVertices[(i + 1) * 3 + 1];
                triangulo[8] = faceVertices[(i + 1) * 3 + 2];

                // Adicionar o triângulo à lista de faces
                faces.push_back(triangulo);
            }
        }
        else {
            // Ignorar outros tipos de linhas (como "g", "o", "usemtl", etc.)
            string restoLinha;
            getline(arquivo, restoLinha);
            continue;
        }
    }
    arquivo.close();

    // Centralizar o objeto
    if (!vertices.empty()) {
        // Inicializar min e max com o primeiro vértice
        float minX = vertices[0][0], maxX = vertices[0][0];
        float minY = vertices[0][1], maxY = vertices[0][1];
        float minZ = vertices[0][2], maxZ = vertices[0][2];

        // Encontrar os limites
        for (const auto& vertice : vertices) {
            if (vertice[0] < minX) minX = vertice[0];
            if (vertice[0] > maxX) maxX = vertice[0];
            if (vertice[1] < minY) minY = vertice[1];
            if (vertice[1] > maxY) maxY = vertice[1];
            if (vertice[2] < minZ) minZ = vertice[2];
            if (vertice[2] > maxZ) maxZ = vertice[2];
        }

        // Calcular o centro da bounding box
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        // Centralizar os vértices
        for (auto& vertice : vertices) {
            vertice[0] -= centerX;
            vertice[1] -= centerY;
            vertice[2] -= centerZ;
        }

        cout << "Objeto centralizado na origem." << endl;
    }
    else {
        cout << "Nenhum vértice encontrado para centralizar." << endl;
    }

    // Compilar a lista de display para desenhar o objeto
    modeloElefante = glGenLists(1);
    glNewList(modeloElefante, GL_COMPILE);
    {
        glBegin(GL_TRIANGLES);  // Renderizar as faces como triângulos
        for (const auto& face : faces) {
            for (int i = 0; i < 3; i++) {
                int indiceVertice = face[i * 3];
                int indiceTextura = face[i * 3 + 1];
                int indiceNormal = face[i * 3 + 2];

                // Se houver coordenada de textura, aplica-la
                if (indiceTextura >= 0 && indiceTextura < coordenadasTextura.size()) {
                    glTexCoord2f(coordenadasTextura[indiceTextura][0], coordenadasTextura[indiceTextura][1]);
                }
                else {
                    glColor3f(1.0f, 1.0f, 1.0f);  // Cor padrão se não houver textura
                }

                // Aplicar a normal correspondente
                if (indiceNormal >= 0 && indiceNormal < normais.size()) {
                    glNormal3f(normais[indiceNormal][0], normais[indiceNormal][1], normais[indiceNormal][2]);
                }

                // Renderizar o vértice
                if (indiceVertice >= 0 && indiceVertice < vertices.size()) {
                    glVertex3f(vertices[indiceVertice][0], vertices[indiceVertice][1], vertices[indiceVertice][2]);
                }
            }
        }
        glEnd();
    }
    glEndList();
}

// Funcao para ajustar o tamanho da janela
void redimensionarJanela(int largura, int altura) {
    glViewport(0, 0, largura, altura);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (GLfloat)largura / (GLfloat)altura, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

// Funcao para desenhar o elefante
void desenharObjeto() {
    glPushMatrix();
    glColor3f(1.0, 0.23, 0.27);
    glTranslatef(deslocamentoX, deslocamentoY, -105);
    glScalef(escala, escala, escala);
    glRotatef(angulo, 0, 1, 0);
    glCallList(modeloElefante);
    glPopMatrix();
}

// Funcao de exibicao
void exibirCena(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    desenharObjeto();
    glutSwapBuffers();
}

// Funcao de controle de teclado
void teclado(unsigned char tecla, int x, int y) {
    switch (tecla) {
    case 27: // Esc para sair
        exit(0);
        break;
    case 'w': // Deslocar para cima
        deslocamentoY += 10.0f;
        break;
    case 's': // Deslocar para baixo
        deslocamentoY -= 10.0f;
        break;
    case 'a': // Deslocar para a esquerda
        deslocamentoX -= 10.0f;
        break;
    case 'd': // Deslocar para a direita
        deslocamentoX += 10.0f;
        break;
    case '+': // Aumentar escala
        escala += 0.1f;
        break;
    case '-': // Diminuir escala, mas com limite
        escala -= 0.1f;
        if (escala < 0.01f) {
            escala = 0.01f;  // Limite minimo da escala
        }
        break;
    case 'r': // Rotacionar no sentido horario
        angulo += 10.0f;
        break;
    case 'l': // Rotacionar no sentido anti-horario
        angulo -= 10.0f;
        break;
    case '1': // Ativar/Desativar Luz 1 (vermelha)
        if (glIsEnabled(GL_LIGHT0)) {
            glDisable(GL_LIGHT0);
        }
        else {
            glEnable(GL_LIGHT0);
        }
        break;
    case '2': // Ativar/Desativar Luz 2 (verde)
        if (glIsEnabled(GL_LIGHT1)) {
            glDisable(GL_LIGHT1);
        }
        else {
            glEnable(GL_LIGHT1);
        }
        break;
    case '3': // Ativar/Desativar Luz 3 (azul)
        if (glIsEnabled(GL_LIGHT2)) {
            glDisable(GL_LIGHT2);
        }
        else {
            glEnable(GL_LIGHT2);
        }
        break;
    }
    glutPostRedisplay(); // Redesenhar a cena com as novas configuracoes
}

// Funcao para configurar as luzes na cena
void configurarLuzes() {
    // ************ LUZ 1 (Vermelha) ************
    // Luz ambiente da Luz 1 - Define o brilho geral do ambiente, independente da direcao
    GLfloat luzAmbiente1[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    // Luz difusa da Luz 1 - Define a cor e intensidade da luz que atinge diretamente os objetos
    GLfloat luzDifusa1[] = { 1.0f, 0.0f, 0.0f, 1.0f };  // Luz vermelha

    // Luz especular da Luz 1 - Define os reflexos brilhantes, como se fossem de uma superficie brilhante
    GLfloat luzEspecular1[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Posicao da Luz 1 - Coordenadas x, y, z para posicionar a luz
    GLfloat posicaoLuz1[] = { 50.0f, 50.0f, 50.0f, 1.0f };

    // Configura os componentes da Luz 1 no OpenGL
    glLightfv(GL_LIGHT0, GL_AMBIENT, luzAmbiente1);  // Define a luz ambiente
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luzDifusa1);    // Define a luz difusa
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular1); // Define a luz especular
    glLightfv(GL_LIGHT0, GL_POSITION, posicaoLuz1);  // Define a posicao da luz

    // ************ LUZ 2 (Verde) ************
    // Luz ambiente da Luz 2 - Define o brilho geral do ambiente, independente da direcao
    GLfloat luzAmbiente2[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    // Luz difusa da Luz 2 - Define a cor e intensidade da luz que atinge diretamente os objetos
    GLfloat luzDifusa2[] = { 0.0f, 1.0f, 0.0f, 1.0f };  // Luz verde

    // Luz especular da Luz 2 - Define os reflexos brilhantes, como se fossem de uma superficie brilhante
    GLfloat luzEspecular2[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Posicao da Luz 2 - Coordenadas x, y, z para posicionar a luz
    GLfloat posicaoLuz2[] = { -50.0f, 50.0f, 50.0f, 1.0f };

    // Configura os componentes da Luz 2 no OpenGL
    glLightfv(GL_LIGHT1, GL_AMBIENT, luzAmbiente2);  // Define a luz ambiente
    glLightfv(GL_LIGHT1, GL_DIFFUSE, luzDifusa2);    // Define a luz difusa
    glLightfv(GL_LIGHT1, GL_SPECULAR, luzEspecular2); // Define a luz especular
    glLightfv(GL_LIGHT1, GL_POSITION, posicaoLuz2);  // Define a posicao da luz

    // ************ LUZ 3 (Azul) ************
    // Luz ambiente da Luz 3 - Define o brilho geral do ambiente, independente da direcao
    GLfloat luzAmbiente3[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    // Luz difusa da Luz 3 - Define a cor e intensidade da luz que atinge diretamente os objetos
    GLfloat luzDifusa3[] = { 0.0f, 0.0f, 1.0f, 1.0f };  // Luz azul

    // Luz especular da Luz 3 - Define os reflexos brilhantes, como se fossem de uma superficie brilhante
    GLfloat luzEspecular3[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Posicao da Luz 3 - Coordenadas x, y, z para posicionar a luz
    GLfloat posicaoLuz3[] = { 0.0f, 50.0f, -50.0f, 1.0f };

    // Configura os componentes da Luz 3 no OpenGL
    glLightfv(GL_LIGHT2, GL_AMBIENT, luzAmbiente3);  // Define a luz ambiente
    glLightfv(GL_LIGHT2, GL_DIFFUSE, luzDifusa3);    // Define a luz difusa
    glLightfv(GL_LIGHT2, GL_SPECULAR, luzEspecular3); // Define a luz especular
    glLightfv(GL_LIGHT2, GL_POSITION, posicaoLuz3);  // Define a posicao da luz

    // ************ Ativacao das Luzes ************
    // Ativar o modelo de iluminacao do OpenGL (habilitar o uso de iluminacao na cena)
    glEnable(GL_LIGHTING);

    // Ativar cada uma das luzes configuradas
    glEnable(GL_LIGHT0);    // Ativa a Luz 1 (Vermelha)
    glEnable(GL_LIGHT1);    // Ativa a Luz 2 (Verde)
    glEnable(GL_LIGHT2);    // Ativa a Luz 3 (Azul)
}


// Funcao principal
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // Habilitar teste de profundidade
    glutInitWindowSize(800, 450);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Carregar OBJ");

    glEnable(GL_DEPTH_TEST); // Habilitar teste de profundidade

    configurarLuzes();  // Configurar as luzes

    glutReshapeFunc(redimensionarJanela);
    glutDisplayFunc(exibirCena);
    glutKeyboardFunc(teclado);

    carregarObjeto("data/elepham.obj");  // Carregar o objeto OBJ

    glutMainLoop();
    return 0;
}
