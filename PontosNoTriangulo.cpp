#include <GL/gl.h>
#include <cstddef>
#include <iostream>
#include <cmath>
#include <ctime>
#include <fstream>

#ifdef WIN32
	#include <windows.h>
	#include <glut.h>
#else
	#include <sys/time.h>
#endif

#ifdef __APPLE__
	#include <GLUT/glut.h>
#endif

#ifdef __linux__
	#include <GL/glut.h>
#endif

using namespace std;

#include "ListaDeCoresRGB.h"
#include "Ponto.h"
#include "Poligono.h"
#include "Temporizador.h"
#include "Quadtree.h"

enum TesteDeColisao {
	NADA,
	FORCA_BRUTA,
	ENVELOPE,
	QUADTREE
};

Temporizador t;
double acumDeltaT=0;
double nFrames=0;
double tempoTotal=0;

Poligono pontosDoCenario;
Poligono campoDeVisao;
Poligono envelope;
Poligono trianguloBase;
Ponto posicaoDoCampoDeVisao;
float anguloDoCampoDeVisao = 0.0;
float proporcao = 0.25;
float envelopeMaiorX, envelopeMaiorY, envelopeMenorY, envelopeMenorX;

Ponto desenhoMin, desenhoMax;
Ponto tamanhoDaJanela, meioDaJanela;
Ponto pontoClicado;
bool foiClicado = false;

bool eixosDesenhados = true;
bool quadtreeDesenhada= false;
bool quadtreeCriada=true;
TesteDeColisao testeDeColisao = NADA;

int pontosQuadtree=9;

void display();
void animate();
void init(bool lerArquivo, char *nomeDoArquivo);
void teclado(unsigned char key, int x, int y);
void flechas(int flechas_, int x, int y);
void mouse(int button, int state, int x, int y);
void redimensiona( int w, int h );
Poligono testaColisaoPorForcaBruta(Poligono pontos);
Poligono testaColisaoPorEnvelope(Poligono pontos);
Poligono testaColisaoPorQuadtree(Poligono pontos);
void criaQuadtree(int pontos);
void posicionaEnvelope();
void desenhaEnvelope();
void desenhaEixos();
void desenhaVerticesColoridos(Poligono poligono, int cor);

int main (int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
	glutInitWindowPosition(0,0);
	glutInitWindowSize(500, 500);
	glutCreateWindow("Pontos no Triângulo");

	if (argc > 1) {
		init(true, argv[1]);
	} else {
		init(false, NULL);
	}

	// Define a função que será chamada automaticamente
	// quando for necessário redesenhar a janela.
	glutDisplayFunc(display);
	glutIdleFunc(animate);
	// Define a função que será chamada automaticamente
	// quando for necessário redimensionar a janela.
	glutReshapeFunc(redimensiona);
	// Define a função que será chamada automaticamente
	// sempre que o usuário pressionar uma tecla comum.
	glutKeyboardFunc(teclado);
	// Define a função que será chamada automaticamente
	// sempre que o usuário pressionar uma tecla especial.
	glutSpecialFunc(flechas);
	glutMouseFunc(mouse);
	// Inicia o tratamento dos eventos.
	glutMainLoop();

	return 0;
}

void display()
{
	// Limpa a tela com a cor de fundo.
	glClear(GL_COLOR_BUFFER_BIT);
	// Define os limites lógicos da área OpenGL dentro da janela.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (eixosDesenhados)
	{
		glLineWidth(1);
		glColor3f(0,0,0);
		desenhaEixos();
	}

	if (quadtreeDesenhada)
	{
		glLineWidth(1);
		glColor3f(1,1,1);
		/* desenhaQuadtree(); */
	}

	if(!quadtreeCriada){
		//chama a função para criar a quadtree
		criaQuadtree(pontosQuadtree);
		quadtreeCriada=true;
	}

	glPointSize(2);
	desenhaVerticesColoridos(pontosDoCenario, Firebrick);

	glLineWidth(3);
	glColor3f(1,0,0);
	campoDeVisao.desenhaPoligono();

	static Poligono dentro;

	switch (testeDeColisao) {
		case NADA:
		    desenhaVerticesColoridos(pontosDoCenario, DarkWood);
			break;
		case FORCA_BRUTA:
			dentro = testaColisaoPorForcaBruta(pontosDoCenario);
			cout << "Fora do triângulo há " << pontosDoCenario.getNVertices()-dentro.getNVertices()
				<< " pontos\n";
			break;
		case ENVELOPE:
			glLineWidth(1);
			glColor3f(1,1,0);
			envelope.desenhaPoligono();
			dentro = testaColisaoPorEnvelope(pontosDoCenario);
			desenhaVerticesColoridos(dentro, Gold);
			cout << "Fora do envelope há " << pontosDoCenario.getNVertices()-dentro.getNVertices()
				<< " pontos\n";
			cout << "Dentro do envelope há " << dentro.getNVertices()
				<< " pontos\n";
			dentro = testaColisaoPorForcaBruta(dentro);
			break;
		case QUADTREE:
			dentro = testaColisaoPorQuadtree(pontosDoCenario);
			desenhaVerticesColoridos(dentro, Gold);
			cout << "Fora da Quadtree há " << pontosDoCenario.getNVertices()-dentro.getNVertices()
				<< " pontos\n";
			cout << "Dentro da Quadtree há " << dentro.getNVertices()
				<< " pontos\n";
			dentro = testaColisaoPorForcaBruta(dentro);
			break;
	}

	if (testeDeColisao != NADA) {
		desenhaVerticesColoridos(dentro, ForestGreen);
		cout << "Dentro do triângulo há " << dentro.getNVertices()
			<< " pontos\n";
	}


	if (foiClicado)
	{
		pontoClicado.imprime("- Ponto no universo: ", "\n");
		foiClicado = false;
	}

	glutSwapBuffers();
}

Poligono testaColisaoPorForcaBruta(Poligono pontos)
{
	Poligono pontosDentroDoTriangulo;

	Ponto verticesDoTriangulo[] = {
		campoDeVisao.getVertice(0), campoDeVisao.getVertice(1),
		campoDeVisao.getVertice(2)
	};

	Ponto arestasDoTriangulo[] = {
		campoDeVisao.getVertice(0) - campoDeVisao.getVertice(1),
		campoDeVisao.getVertice(1) - campoDeVisao.getVertice(2),
		campoDeVisao.getVertice(2) - campoDeVisao.getVertice(0),
	};

	Ponto vp, prodVetorial;

	for (size_t i = 0; i < pontos.getNVertices(); i++) {
		for (size_t j = 0; j < 3; j++) {
			vp = pontos.getVertice(i) - verticesDoTriangulo[j];
			ProdVetorial(vp, arestasDoTriangulo[j], prodVetorial);

			if (prodVetorial.z > 0) {
				break;
			}
		}

		if (prodVetorial.z < 0) {
			pontosDentroDoTriangulo.insereVertice(pontos.getVertice(i));
		}
	}

	return pontosDentroDoTriangulo;
}

Poligono testaColisaoPorEnvelope(Poligono pontos)
{
	Poligono pontosDentroDoEnvelope;

	for (std::size_t i = 0; i < pontos.getNVertices(); i++) {
		if (
			pontos.getVertice(i).x > envelopeMenorX &&
			pontos.getVertice(i).x < envelopeMaiorX &&
			pontos.getVertice(i).y > envelopeMenorY &&
			pontos.getVertice(i).y < envelopeMaiorY
		){
			pontosDentroDoEnvelope.insereVertice(pontos.getVertice(i));
		}
	}

	return pontosDentroDoEnvelope;
}

Poligono* testaColisaoPorQuadtree(QuadtreeNode<Poligono> *quadtree) {
}

QuadtreeNode<Poligono>* criaQuadtree(Poligono pontos) {
	if (pontos.getNVertices() > pontosQuadtree) {
		Poligono pontosNw, pontosNe, pontosSw, pontosSe;
		Ponto min, max, ponto;
		pontos.obtemLimites(min, max);
		float meioX = (min.x + max.x) / 2;
		float meioY = (min.x + max.x) / 2;

		for (size_t i = 0; i < pontos.getNVertices(); i++) {
			ponto = pontos.getVertice(i);

			if (ponto.x < meioX) {
				if (ponto.y < meioY) {
					pontosSw.insereVertice(ponto);
				} else {
					pontosNw.insereVertice(ponto);
				}
			} else {
				if (ponto.y < meioY) {
					pontosSe.insereVertice(ponto);
				} else {
					pontosNe.insereVertice(ponto);
				}
			}
		}

	    return new QuadtreeNode<Poligono>(
			criaQuadtree(pontosNw), criaQuadtree(pontosNe),
			criaQuadtree(pontosSw), criaQuadtree(pontosSe)
		);
	}

	return new QuadtreeNode<Poligono>(&pontos);
}

void criaEnvelope() {
	for (size_t i = 0; i < 4; i++) {
		envelope.insereVertice(Ponto(0, 0, 0));
	}
}

void posicionaEnvelope() {
	envelopeMaiorX = campoDeVisao.getVertice(0).x;
	envelopeMenorX = campoDeVisao.getVertice(0).x;
	envelopeMaiorY = campoDeVisao.getVertice(0).y;
	envelopeMenorY = campoDeVisao.getVertice(0).y;

	for (int i = 1; i < campoDeVisao.getNVertices(); i++) {
		if (campoDeVisao.getVertice(i).x > envelopeMaiorX) {
			envelopeMaiorX = campoDeVisao.getVertice(i).x;
		}

		if (campoDeVisao.getVertice(i).x < envelopeMenorX) {
			envelopeMenorX = campoDeVisao.getVertice(i).x;
		}

		if (campoDeVisao.getVertice(i).y > envelopeMaiorY) {
			envelopeMaiorY = campoDeVisao.getVertice(i).y;
		}

		if (campoDeVisao.getVertice(i).y < envelopeMenorY) {
			envelopeMenorY = campoDeVisao.getVertice(i).y;
		}
	}

	envelope.alteraVertice(0, Ponto(envelopeMenorX, envelopeMaiorY));
	envelope.alteraVertice(1, Ponto(envelopeMaiorX, envelopeMaiorY));
	envelope.alteraVertice(2, Ponto(envelopeMaiorX, envelopeMenorY));
	envelope.alteraVertice(3, Ponto(envelopeMenorX, envelopeMenorY));
}

void desenhaVerticesColoridos(Poligono poligono, int cor) {
	for (std::size_t i = 0; i < poligono.getNVertices(); i++) {
		Ponto vertice = poligono.getVertice(i);
		glBegin(GL_POINTS);
		defineCor(cor);
		glVertex3f(vertice.x, vertice.y, vertice.z);
	}

	glEnd();
}

/**
 * Cria um triângulo a partir do vetor (1, 0, 0),
 * girando este vetor em 45 e -45 graus.
 *
 * @return em TrianguloBase e CampoDeVisao.
 */
void criaTrianguloDoCampoDeVisao()
{
	Ponto vetor = Ponto(1,0,0);

	trianguloBase.insereVertice(Ponto(0,0,0));
	campoDeVisao.insereVertice(Ponto(0,0,0));

	vetor.rotacionaZ(45);
	trianguloBase.insereVertice(vetor);
	campoDeVisao.insereVertice(vetor);

	vetor.rotacionaZ(-90);
	trianguloBase.insereVertice(vetor);
	campoDeVisao.insereVertice(vetor);
}

/**
 * Posiciona o campo de visão em PosicaoDoCampoDeVisao,
 * com a orientação de AnguloDoCampoDeVisao.
 * O tamanho do campo de visão é de 25% a largura da janela.
 */
void posicionaTrianguloDoCampoDeVisao(float proporcao)
{
	float tamanho = tamanhoDaJanela.x * proporcao;

	Ponto temp;
	for (int i=0;i<trianguloBase.getNVertices();i++)
	{
		temp = trianguloBase.getVertice(i);
		temp.rotacionaZ(anguloDoCampoDeVisao);
		campoDeVisao.alteraVertice(i, posicaoDoCampoDeVisao + temp*tamanho);
	}
}

/**
 * Gera pontos aleatórios em um intervalo.
 */
void geraPontos(unsigned long int qtd, Ponto min, Ponto max)
{
	time_t t;
	Ponto escala = (max - min) * (1.0/1000.0);
	srand((unsigned) time(&t));

	for (int i = 0;i<qtd; i++)
	{
		float x = rand() % 1000;
		float y = rand() % 1000;
		x = x * escala.x + min.x;
		y = y * escala.y + min.y;
		pontosDoCenario.insereVertice(Ponto(x,y));
	}
}

/**
 * Move o campo de visão um determinado número de unidades
 * para frente ou para trás.
 */
void avancaCampoDeVisao(float distancia)
{
	Ponto vetor = Ponto(1, 0, 0);
	vetor.rotacionaZ(anguloDoCampoDeVisao);
	posicaoDoCampoDeVisao = posicaoDoCampoDeVisao + vetor * distancia;
}

void animate()
{
	double dt;
	dt = t.getDeltaT();
	acumDeltaT += dt;
	tempoTotal += dt;
	nFrames++;

	if (acumDeltaT > 1.0/30) // fixa a atualiza��o da tela em 30
	{
		acumDeltaT = 0;
		glutPostRedisplay();
	}
	if (tempoTotal > 5.0)
	{
		cout << "Tempo Acumulado: "  << tempoTotal << " segundos. " ;
		cout << "Nros de Frames sem desenho: " << nFrames << endl;
		cout << "FPS(sem desenho): " << nFrames/tempoTotal << endl;
		tempoTotal = 0;
		nFrames = 0;
	}
}

/**
 * Inicializa as variáveis de estado da aplicação.
 */
void init(bool lerArquivo, char *nomeDoArquivo)
{
	// Define a cor do fundo da tela.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	if (lerArquivo) {
		pontosDoCenario.LePoligono(nomeDoArquivo);
	} else {
		// Gera ou Carrega os pontos do cenario.
		// Note que o "aspect ratio" dos pontos deve ser o mesmo da janela.
		geraPontos(1000, Ponto(0,0), Ponto(500,500));
	}

	pontosDoCenario.obtemLimites(desenhoMin, desenhoMax);
	meioDaJanela = (desenhoMax + desenhoMin) * 0.5;
	tamanhoDaJanela = (desenhoMax - desenhoMin);
	posicaoDoCampoDeVisao = meioDaJanela;
	anguloDoCampoDeVisao = 0;
	criaTrianguloDoCampoDeVisao();
	posicionaTrianguloDoCampoDeVisao(proporcao);
	criaEnvelope();
	posicionaEnvelope();
}

void desenhaLinha(Ponto P1, Ponto P2)
{
	glBegin(GL_LINES);
	glVertex3f(P1.x,P1.y,P1.z);
	glVertex3f(P2.x,P2.y,P2.z);
	glEnd();
}

void desenhaEixos()
{
	glBegin(GL_LINES);
	// Eixo horizontal.
	glVertex2f(desenhoMin.x,meioDaJanela.y);
	glVertex2f(desenhoMax.x,meioDaJanela.y);
	// Eixo vertical.
	glVertex2f(meioDaJanela.x,desenhoMin.y);
	glVertex2f(meioDaJanela.x,desenhoMax.y);
	glEnd();
}

void desenhaQuadtree()
{
	glBegin(GL_LINES);
	//colocar quadtree para ser desenhada
	glEnd();
}

/**
 * Espera um certo número de segundos e informa
 * quantos frames se passaram neste período.
 */
void contaTempo(double tempo)
{
	Temporizador T;

	unsigned long cont = 0;
	cout << "Inicio contagem de " << tempo << "segundos ..." << flush;
	while(true)
	{
		tempo -= T.getDeltaT();
		cont++;
		if (tempo <= 0.0)
		{
			cout << "fim! - Passaram-se " << cont << " frames." << endl;
			break;
		}
	}
}

void PosicionaCampoDeVisao(int n)
{
	switch (n) {
		case 1:
			AnguloDoCampoDeVisao = 0;
			PosicaoDoCampoDeVisao = Meio;
			break;
		case 2:
			AnguloDoCampoDeVisao = 90;
			PosicaoDoCampoDeVisao = Meio;
			break;
		case 3:
			AnguloDoCampoDeVisao = 90;
			PosicaoDoCampoDeVisao = Meio*0.5;
			break;
		case 4:
			AnguloDoCampoDeVisao = 0;
			PosicaoDoCampoDeVisao = Meio + Meio*0.5;
			break;
		default:
			break;
	}
	PosicionaTrianguloDoCampoDeVisao();
}

void teclado(unsigned char key, int x, int y)
{
	switch (key) {
		// Tecla ESC
		case 27:
			exit (0);
			break;
		case '1':
		case '2':
		case '3':
		case '4':
			int i;
			i = key - '0';
			PosicionaCampoDeVisao(i);
			break;
		case 't':
			contaTempo(3);
			break;
		case 'f':
			if (testeDeColisao == FORCA_BRUTA) {
				testeDeColisao = NADA;
			} else {
				testeDeColisao = FORCA_BRUTA;
			}
			break;
		case 'e':
			if (testeDeColisao == ENVELOPE) {
				testeDeColisao = NADA;
			} else {
				testeDeColisao = ENVELOPE;
			}
			break;
		case 'q':
			if (testeDeColisao == QUADTREE) {
				testeDeColisao = NADA;
			} else {
				testeDeColisao = QUADTREE;
			}
			break;
		case ' ':
			eixosDesenhados ^= true;
			break;
		case 'd':
			quadtreeDesenhada = !quadtreeDesenhada;
			break;
		case 'i':
			//colocar comunicação teclado/programa para o usuário inserir quantos pontos
			//tem cada parte da quadtree
			// pontosQuadtree;
			quadtreeCriada=false;
			break;
		case 'a':
			if (proporcao > 0) {
				proporcao -= 0.01;
			}

			posicionaTrianguloDoCampoDeVisao(proporcao);
			posicionaEnvelope();
			break;
		case 's':
			proporcao += 0.01;
			posicionaTrianguloDoCampoDeVisao(proporcao);
			posicionaEnvelope();
			break;
		default:
			break;
	}

	glutPostRedisplay();
}

void flechas(int flechas_, int x, int y)
{
	switch (flechas_) {
		case GLUT_KEY_LEFT:
			anguloDoCampoDeVisao += 2;
			break;
		case GLUT_KEY_RIGHT:
			anguloDoCampoDeVisao -= 2;
			break;
		case GLUT_KEY_UP:
			avancaCampoDeVisao(2);
			break;
		case GLUT_KEY_DOWN:
			avancaCampoDeVisao(-2);
			break;
		default:
			break;
	}

	posicionaTrianguloDoCampoDeVisao(proporcao);
	posicionaEnvelope();
	glutPostRedisplay();
}

/**
 * Captura o clique do botão direito do mouse sobre a área de desenho
 * e converte a coordenada para o sistema de referência definida na glOrtho.
 * Baseado em http://hamala.se/forums/viewtopic.php?t=20.
 */
void mouse(int button, int state, int x, int y)
{
	GLint viewport[4];
	GLdouble modelview[16], projection[16];
	GLfloat wx = x, wy, wz;
	GLdouble ox = 0.0, oy=0.0, oz=0.0;

	if(state != GLUT_DOWN)
		return;
	if(button != GLUT_RIGHT_BUTTON)
		return;

	glGetIntegerv(GL_VIEWPORT, viewport);
	y = viewport[3] - y;
	wy = y;
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &wz);
	gluUnProject(wx, wy, wz, modelview, projection, viewport, &ox, &oy, &oz);
	pontoClicado = Ponto(ox, oy, oz);
	foiClicado = true;
}

/**
 * Trata o redimensionamento da janela em OpenGL.
 */
void redimensiona( int w, int h )
{
	// Reinicia o sistema de coordenadas.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Define a area a ser ocupada pela area OpenGL dentro da Janela.
	glViewport(0, 0, w, h);
	// Define os limites logicos da area OpenGL dentro da Janela.
	glOrtho(desenhoMin.x, desenhoMax.x, desenhoMin.y, desenhoMax.y, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
