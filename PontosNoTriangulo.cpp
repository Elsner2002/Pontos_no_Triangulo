#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <cstddef>
#include <iostream>
#include <cmath>
#include <ctime>
#include <fstream>
#include <string>

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
double acumDeltaT = 0;
double nFrames = 0;
double tempoTotal = 0;

Poligono pontosDoCenario;
Poligono campoDeVisao;
Poligono envelope;
Poligono trianguloBase;
Ponto posicaoDoCampoDeVisao;
float anguloDoCampoDeVisao = 0.0;
float proporcao = 0.25;
float envelopeMaiorX, envelopeMaiorY, envelopeMenorY, envelopeMenorX;

Ponto minPontosDoCenario, maxPontosDoCenario, meioPontosDoCenario;
Ponto tamanhoDaJanela, meioDaJanela;
Ponto pontoClicado;
bool foiClicado = false;

bool eixosDesenhados = true;
bool quadtreeDesenhada = false;
bool quadtreeCriada = false;
TesteDeColisao testeDeColisao = NADA;

int pontosFolhaQuadtree = 10;
int niveisQuadtreeTestados = 0;
bool imprimeResultados = false;

void display();
void animate();
void init(bool lerArquivo, char *nomeDoArquivo);
void teclado(unsigned char key, int x, int y);
void flechas(int flechas_, int x, int y);
void mouse(int button, int state, int x, int y);
void redimensiona( int w, int h );
Poligono testaColisaoPorForcaBruta(Poligono pontos);
Poligono testaColisaoPorEnvelope(Poligono pontos);
Poligono* testaColisaoPorQuadtree(QuadtreeNode<Poligono> *quadtree);
QuadtreeNode<Poligono>* criaQuadtree(Poligono *pontos, Ponto min, Ponto max);
void desenhaQuadtree(QuadtreeNode<Poligono> *quadtree, size_t nivel);
void posicionaEnvelope();
void desenhaEixos();
void desenhaVerticesColoridos(Poligono poligono, int cor);

int main (int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
	glutInitWindowPosition(0,0);
	glutInitWindowSize(500, 500);
	glutCreateWindow("Pontos no Tri??ngulo");

	if (argc > 1) {
		init(true, argv[1]);
	} else {
		init(false, NULL);
	}

	// Define a fun????o que ser?? chamada automaticamente
	// quando for necess??rio redesenhar a janela.
	glutDisplayFunc(display);
	glutIdleFunc(animate);
	// Define a fun????o que ser?? chamada automaticamente
	// quando for necess??rio redimensionar a janela.
	glutReshapeFunc(redimensiona);
	// Define a fun????o que ser?? chamada automaticamente
	// sempre que o usu??rio pressionar uma tecla comum.
	glutKeyboardFunc(teclado);
	// Define a fun????o que ser?? chamada automaticamente
	// sempre que o usu??rio pressionar uma tecla especial.
	glutSpecialFunc(flechas);
	glutMouseFunc(mouse);
	// Inicia o tratamento dos eventos.
	glutMainLoop();

	return 0;
}

void display()
{
	static QuadtreeNode<Poligono> *quadTree;
	// Limpa a tela com a cor de fundo.
	glClear(GL_COLOR_BUFFER_BIT);
	// Define os limites l??gicos da ??rea OpenGL dentro da janela.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (eixosDesenhados)
	{
		glLineWidth(1);
		glColor3f(0,0,0);
		desenhaEixos();
	}

	if(!quadtreeCriada){
		Ponto min, max;
		pontosDoCenario.obtemLimites(min, max);
		delete quadTree;
		quadTree = criaQuadtree(&pontosDoCenario, min, max);
		quadtreeCriada = true;
	}

	if (quadtreeDesenhada) {
		desenhaQuadtree(quadTree, 0);
	}

	glPointSize(4);
	desenhaVerticesColoridos(pontosDoCenario, Firebrick);

	glLineWidth(3);
	glColor3f(1,0,0);
	campoDeVisao.desenhaPoligono();

	static Poligono dentro;

	if (imprimeResultados && testeDeColisao != NADA) {
		std::string modo = "";

		switch (testeDeColisao) {
			case FORCA_BRUTA:
				modo = "For??a Bruta";
				break;
			case ENVELOPE:
				modo = "Envelope";
				break;
			case QUADTREE:
				modo = "Quadtree";
				break;
		}

		cout << "Modo: " << modo << std::endl;
	}

	switch (testeDeColisao) {
		case NADA:
		    desenhaVerticesColoridos(pontosDoCenario, DarkWood);
			break;
		case FORCA_BRUTA:
			dentro = testaColisaoPorForcaBruta(pontosDoCenario);
			break;
		case ENVELOPE:
			glLineWidth(1);
			glColor3f(0,0,0);
			envelope.desenhaPoligono();

			dentro = testaColisaoPorEnvelope(pontosDoCenario);
			desenhaVerticesColoridos(dentro, Gold);
			dentro = testaColisaoPorForcaBruta(dentro);
			break;
		case QUADTREE:
			glLineWidth(1);
			glColor3f(0,0,0);
			envelope.desenhaPoligono();

			niveisQuadtreeTestados = 0;
			dentro = *testaColisaoPorQuadtree(quadTree);
			desenhaVerticesColoridos(dentro, Gold);

			if (imprimeResultados) {
				cout << "Testes Envelope X Envelope: " << niveisQuadtreeTestados << std::endl;
			}

			dentro = testaColisaoPorForcaBruta(dentro);
			break;
	}

	if (testeDeColisao != NADA) {
		desenhaVerticesColoridos(dentro, ForestGreen);
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
	if (imprimeResultados) {
		cout << "Testes For??a Bruta: " << pontos.getNVertices() << std::endl << std::endl;
		imprimeResultados = false;
	}

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
	if (imprimeResultados) {
		std::cout << "Testes Envelope X Ponto: " << pontos.getNVertices() << std::endl;
	}

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

bool testaColisaoDeEnvelopes(Poligono a, Poligono b)
{
	Ponto aMin, aMax, bMin, bMax;
	a.obtemLimites(aMin, aMax);
	b.obtemLimites(bMin, bMax);

	return (
		aMin.x <= bMax.x &&
		aMax.x >= bMin.x &&
		aMin.y <= bMax.y &&
		aMax.y >= bMin.y
	);
}

Poligono* testaColisaoPorQuadtree(QuadtreeNode<Poligono> *quadtree) {
	Poligono *pontosDentroDoEnvelope = new Poligono();
	Poligono envelopeQuadtree = Poligono();
	envelopeQuadtree.insereVertice(quadtree->min);
	envelopeQuadtree.insereVertice(quadtree->max);

	if (testaColisaoDeEnvelopes(envelope, envelopeQuadtree)) {
		niveisQuadtreeTestados++;

		if (quadtree->isLeaf()) {
			return quadtree->data;
		} else {
			Poligono *poligonos[] = {
				testaColisaoPorQuadtree(quadtree->nw),
				testaColisaoPorQuadtree(quadtree->ne),
				testaColisaoPorQuadtree(quadtree->sw),
				testaColisaoPorQuadtree(quadtree->se),
			};

			for (auto poligono : poligonos) {
				for (size_t i = 0; i < poligono->getNVertices(); i++) {
					pontosDentroDoEnvelope->insereVertice(
						poligono->getVertice(i)
					);
				}
			}
		}
	}

	return pontosDentroDoEnvelope;
}

QuadtreeNode<Poligono>* criaQuadtree(Poligono *pontos, Ponto min, Ponto max) {
	if (pontos->getNVertices() > pontosFolhaQuadtree) {
		Poligono pontosNw, pontosNe, pontosSw, pontosSe;
		float meioX = (min.x + max.x) / 2;
		float meioY = (min.y + max.y) / 2;
		Ponto ponto;

		for (size_t i = 0; i < pontos->getNVertices(); i++) {
			ponto = pontos->getVertice(i);

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

		Ponto minNw = Ponto(min.x, meioY);
		Ponto maxNw = Ponto(meioX, max.y);
		Ponto meio = Ponto(meioX, meioY);
		Ponto minSe = Ponto(meioX, min.y);
		Ponto maxSe = Ponto(max.x, meioY);

	    return new QuadtreeNode<Poligono>(
			criaQuadtree(&pontosNw, minNw, maxNw),
			criaQuadtree(&pontosNe, meio, max),
			criaQuadtree(&pontosSw, min, meio),
			criaQuadtree(&pontosSe, minSe, maxSe),
			min, max
		);
	}

	return new QuadtreeNode<Poligono>(pontos, min, max);
}

void desenhaQuadtree(QuadtreeNode<Poligono> *quadtree, size_t nivel)
{
	static const size_t NUM_CORES = 6;
	static const float CORES[NUM_CORES][3] = {
		{ 0.0, 0.0, 1.0 },
		{ 1.0, 1.0, 0.0 },
		{ 0.0, 1.0, 0.0 },
		{ 1.0, 0.0, 1.0 },
		{ 1.0, 0.0, 0.0 },
		{ 0.0, 1.0, 1.0 },
	};

	if (quadtree->isLeaf()) {
		return;
	}

	const float *cores = CORES[nivel % NUM_CORES];

	glBegin(GL_LINES);
	glColor3f(cores[0], cores[1], cores[2]);

	float meioX = (quadtree->min.x + quadtree->max.x) / 2;
	float meioY = (quadtree->min.y + quadtree->max.y) / 2;
	glVertex2f(quadtree->min.x, meioY);
	glVertex2f(quadtree->max.x, meioY);
	glVertex2f(meioX, quadtree->min.y);
	glVertex2f(meioX, quadtree->max.y);

	nivel++;
	desenhaQuadtree(quadtree->nw, nivel);
	desenhaQuadtree(quadtree->ne, nivel);
	desenhaQuadtree(quadtree->sw, nivel);
	desenhaQuadtree(quadtree->se, nivel);

	glEnd();
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
 * Cria um tri??ngulo a partir do vetor (1, 0, 0),
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
 * Posiciona o campo de vis??o em posicaoDoCampoDeVisao,
 * com a orienta????o de anguloDoCampoDeVisao.
 * O tamanho do campo de vis??o ?? de 25% a largura da janela.
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
 * Gera pontos aleat??rios em um intervalo.
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

	pontosDoCenario.obtemLimites(minPontosDoCenario, maxPontosDoCenario);
	meioPontosDoCenario = (minPontosDoCenario + maxPontosDoCenario) * 0.5;
}

/**
 * Move o campo de vis??o um determinado n??mero de unidades
 * para frente ou para tr??s.
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

	if (acumDeltaT > 1.0/30) // fixa a atualiza??????o da tela em 30
	{
		acumDeltaT = 0;
		glutPostRedisplay();
	}
	if (tempoTotal > 5.0)
	{
		//cout << "Tempo Acumulado: "  << tempoTotal << " segundos. " ;
		//cout << "Nros de Frames sem desenho: " << nFrames << endl;
		//cout << "FPS(sem desenho): " << nFrames/tempoTotal << endl;
		tempoTotal = 0;
		nFrames = 0;
	}
}

/**
 * Inicializa as vari??veis de estado da aplica????o.
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
		geraPontos(100, Ponto(0,0), Ponto(500,500));
	}

	pontosDoCenario.obtemLimites(minPontosDoCenario, maxPontosDoCenario);
	meioPontosDoCenario = (minPontosDoCenario + maxPontosDoCenario) * 0.5;
	meioDaJanela = (maxPontosDoCenario + minPontosDoCenario) * 0.5;
	tamanhoDaJanela = (maxPontosDoCenario - minPontosDoCenario);
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
	glVertex2f(minPontosDoCenario.x,meioDaJanela.y);
	glVertex2f(maxPontosDoCenario.x,meioDaJanela.y);
	// Eixo vertical.
	glVertex2f(meioDaJanela.x,minPontosDoCenario.y);
	glVertex2f(meioDaJanela.x,maxPontosDoCenario.y);
	glEnd();
}

/**
 * Espera um certo n??mero de segundos e informa
 * quantos frames se passaram neste per??odo.
 */
void contaTempo(double tempo)
{
	Temporizador T;

	unsigned long cont = 0;
	//cout << "Inicio contagem de " << tempo << "segundos ..." << flush;
	while(true)
	{
		tempo -= T.getDeltaT();
		cont++;
		if (tempo <= 0.0)
		{
			//cout << "fim! - Passaram-se " << cont << " frames." << endl;
			break;
		}
	}
}

void PosicionaCampoDeVisao(int n)
{
	switch (n) {
		case 1:
			anguloDoCampoDeVisao = 0;
			posicaoDoCampoDeVisao = meioPontosDoCenario;
			break;
		case 2:
			anguloDoCampoDeVisao = 90;
			posicaoDoCampoDeVisao = meioPontosDoCenario;
			break;
		case 3:
			anguloDoCampoDeVisao = 90;
			posicaoDoCampoDeVisao = meioPontosDoCenario * 0.5;
			break;
		case 4:
			anguloDoCampoDeVisao = 0;
			posicaoDoCampoDeVisao = meioPontosDoCenario + meioPontosDoCenario * 0.5;
			break;
		default:
			break;
	}

	posicionaTrianguloDoCampoDeVisao(proporcao);
	posicionaEnvelope();
}

void teclado(unsigned char key, int x, int y)
{
	static bool modificandoPontosFolhaQuadtree = false;
	static std::string inputPontosFolhaQuadtree = "";

	if (modificandoPontosFolhaQuadtree) {
		if (key >= '0' && key <= '9') {
			inputPontosFolhaQuadtree += key;
			return;
		} else {
			pontosFolhaQuadtree = stoi(inputPontosFolhaQuadtree);
			inputPontosFolhaQuadtree = "";
			modificandoPontosFolhaQuadtree = false;
			quadtreeCriada = false;
		}
	}

	switch (key) {
		// Tecla ESC
		case 27:
			exit (0);
			break;
		case '1':
		case '2':
		case '3':
		case '4':
			cout << "Posi????o " << key<< "\n";
			imprimeResultados = true;
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
		case 'r':
			imprimeResultados = true;
			break;
		case ' ':
			eixosDesenhados ^= true;
			break;
		case 'd':
			quadtreeDesenhada = !quadtreeDesenhada;
			break;
		case 'i':
			modificandoPontosFolhaQuadtree = true;
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
 * Captura o clique do bot??o direito do mouse sobre a ??rea de desenho
 * e converte a coordenada para o sistema de refer??ncia definida na glOrtho.
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
	glOrtho(minPontosDoCenario.x, maxPontosDoCenario.x, minPontosDoCenario.y, maxPontosDoCenario.y, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
