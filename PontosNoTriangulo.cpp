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

#include "Ponto.h"
#include "Poligono.h"
#include "Temporizador.h"

enum TesteDeColisao {
	FORCA_BRUTA,
	ENVELOPE,
	QUADTREE
};

Temporizador T;
double acumDeltaT=0;
double nFrames=0;
double tempoTotal=0;

Poligono pontosDoCenario;
Poligono campoDeVisao;
Poligono trianguloBase;
Ponto posicaoDoCampoDeVisao;
float anguloDoCampoDeVisao = 0.0;
float envelopeMaiorX, envelopeMaiorY, envelopeMenorY, envelopeMenorX;

Ponto desenhoMin, desenhoMax;
Ponto tamanhoDaJanela, meioDaJanela;
Ponto pontoClicado;
bool foiClicado = false;

bool eixosDesenhados = true;
TesteDeColisao testeDeColisao = FORCA_BRUTA;

void display();
void animate();
void init();
void teclado(unsigned char key, int x, int y);
void flechas(int flechas_, int x, int y);
void mouse(int button, int state, int x, int y);
void redimensiona( int w, int h );
Poligono testaColisaoPorForcaBruta(Poligono pontosDoCenario);
Poligono testaColisaoPorEnvelope(Poligono pontosDoCenario);
void criaEnvelope();
void desenhaEnvelope();
void desenhaEixos();
void pintaPoligono(
	Poligono poligono, GLfloat red, GLfloat green, GLfloat blue
);

int main (int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
	glutInitWindowPosition(0,0);
	glutInitWindowSize(500, 500);
	glutCreateWindow("Pontos no Triângulo");
	init();

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
		glColor3f(1,1,1);
		desenhaEixos();
	}

	glColor3f(1,1,0);
	pontosDoCenario.desenhaVertices();

	glLineWidth(3);
	glColor3f(1,0,0);
	campoDeVisao.desenhaPoligono();

	Poligono dentro;

	switch (testeDeColisao) {
		case FORCA_BRUTA:
			dentro = testaColisaoPorForcaBruta(pontosDoCenario);
			break;
		case ENVELOPE:
			criaEnvelope();
			dentro = testaColisaoPorEnvelope(pontosDoCenario);
			pintaPoligono(dentro, 1.0, 1.0, 0.0);
			cout << "Dentro do envelope há " << dentro.getNVertices()
				<< " pontos\n";
			dentro = testaColisaoPorForcaBruta(dentro);
			break;
		case QUADTREE:
			break;
	}

	pintaPoligono(dentro, 0.0, 1.0, 0.0);
	cout << "Dentro do triângulo há " << dentro.getNVertices()
		<< " pontos\n";

	if (foiClicado)
	{
		pontoClicado.imprime("- Ponto no universo: ", "\n");
		foiClicado = false;
	}

	glutSwapBuffers();
}

Poligono testaColisaoPorForcaBruta(Poligono pontosDoCenario){
	Poligono pontosDentroDoTriangulo;
	Ponto p1, p2, p3;

	for (std::size_t i = 0; i < pontosDoCenario.getNVertices(); i++) {
		ProdVetorial(
			pontosDoCenario.getVertice(i), trianguloBase.getVertice(0), p1
		);
		ProdVetorial(
			pontosDoCenario.getVertice(i), trianguloBase.getVertice(1), p2
		);
		ProdVetorial(
			pontosDoCenario.getVertice(i), trianguloBase.getVertice(2), p3
		);

		if(p1.z < 0 && p2.z < 0 && p3.z < 0){
			pontosDentroDoTriangulo.insereVertice(
				pontosDoCenario.getVertice(i)
			);
		}
	}

	return pontosDentroDoTriangulo;
}

Poligono testaColisaoPorEnvelope(Poligono pontosDoCenario){
	Poligono pontosDentroDoEnvelope;

	for (std::size_t i = 0; i < pontosDoCenario.getNVertices(); i++) {
		if (
			pontosDoCenario.getVertice(i).x > envelopeMenorX &&
			pontosDoCenario.getVertice(i).x < envelopeMaiorX &&
			pontosDoCenario.getVertice(i).y > envelopeMenorY &&
			pontosDoCenario.getVertice(i).y < envelopeMaiorY
		){
			pontosDentroDoEnvelope.insereVertice(
				pontosDoCenario.getVertice(i)
			);
		}
	}

	return pontosDentroDoEnvelope;
}

void criaEnvelope(){
	envelopeMaiorX=trianguloBase.getVertice(0).x;
	envelopeMenorX=trianguloBase.getVertice(0).x;
	envelopeMaiorY=trianguloBase.getVertice(0).y;
	envelopeMenorY=trianguloBase.getVertice(0).y;
	for(int i=1; i<3;i++){
		if(trianguloBase.getVertice(i).x>envelopeMaiorX){
			envelopeMaiorX=trianguloBase.getVertice(i).x;
		}
		if(trianguloBase.getVertice(i).x<envelopeMenorX){
			envelopeMenorX=trianguloBase.getVertice(i).x;
		}
		if(trianguloBase.getVertice(i).y>envelopeMaiorY){
			envelopeMaiorY=trianguloBase.getVertice(i).y;
		}
		if(trianguloBase.getVertice(i).y<envelopeMenorY){
			envelopeMenorY=trianguloBase.getVertice(i).y;
		}
	}
}

void pintaPoligono(
	Poligono poligono, GLfloat red, GLfloat green, GLfloat blue
) {
	for (std::size_t i = 0; i < poligono.getNVertices(); i++) {
		Ponto vertice = poligono.getVertice(i);
		glColor3f(red, green, blue);
		glVertex3f(vertice.x, vertice.y, vertice.z);
	}
}

/**
 * Cria um triângulo a partir do vetor (1, 0, 0),
 * girando este vetor em 45 e -45 graus.
 *
 * @return em TrianguloBase e CampoDeVisao.
 */
void CriaTrianguloDoCampoDeVisao()
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
void PosicionaTrianguloDoCampoDeVisao()
{
	float tamanho = tamanhoDaJanela.x * 0.25;

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
void GeraPontos(unsigned long int qtd, Ponto min, Ponto max)
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
void AvancaCampoDeVisao(float distancia)
{
	Ponto vetor = Ponto(1, 0, 0);
	vetor.rotacionaZ(anguloDoCampoDeVisao);
	posicaoDoCampoDeVisao = posicaoDoCampoDeVisao + vetor * distancia;
}

void animate()
{
	double dt;
	dt = T.getDeltaT();
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
void init()
{
	// Define a cor do fundo da tela.
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

	// Gera ou Carrega os pontos do cenario.
	// Note que o "aspect ratio" dos pontos deve ser o mesmo da janela.
	GeraPontos(1000, Ponto(0,0), Ponto(500,500));

	pontosDoCenario.obtemLimites(desenhoMin, desenhoMax);
	meioDaJanela = (desenhoMax + desenhoMin) * 0.5;
	tamanhoDaJanela = (desenhoMax - desenhoMin);
	posicaoDoCampoDeVisao = meioDaJanela;
	anguloDoCampoDeVisao = 0;
	CriaTrianguloDoCampoDeVisao();
	PosicionaTrianguloDoCampoDeVisao();
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

void teclado(unsigned char key, int x, int y)
{
	switch (key) {
		// Tecla ESC
		case 27:
			exit (0);
			break;
		case 't':
			contaTempo(3);
			break;
		case 'f':
			testeDeColisao = FORCA_BRUTA;
			break;
		case 'e':
			testeDeColisao = ENVELOPE;
			break;
		case ' ':
			eixosDesenhados ^= true;
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
			AvancaCampoDeVisao(2);
			break;
		case GLUT_KEY_DOWN:
			AvancaCampoDeVisao(-2);
			break;
		default:
			break;
	}

	PosicionaTrianguloDoCampoDeVisao();
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
