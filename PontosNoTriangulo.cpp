// **********************************************************************
// PUCRS/Escola Polit�cnica
// COMPUTA��O GR�FICA
//
// Programa basico para criar aplicacoes 2D em OpenGL
//
// Marcio Sarroglia Pinho
// pinho@pucrs.br
// **********************************************************************

// Para uso do Windows, sugere-se a versao 17 do Code::Blocks

// Para uso no Xcode:
// Abra o menu Product -> Scheme -> Edit Scheme -> Use custom working directory
// Selecione a pasta onde voce descompactou o ZIP que continha este arquivo.

#include <cstddef>
#include <iostream>
#include <cmath>
#include <ctime>
#include <fstream>

using namespace std;

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

#include "Ponto.h"
#include "Poligono.h"
#include "Temporizador.h"

Temporizador T;
double AccumDeltaT=0;

// Variaveis que controlam o triangulo do campo de visao
Poligono PontosDoCenario, CampoDeVisao, TrianguloBase, eDentro, dentroEnvelope;
float AnguloDoCampoDeVisao=0.0;
float envelopeMaiorX, envelopeMaiorY, envelopeMenorY, envelopeMenorX;

// Limites logicos da area de desenho
Ponto Min, Max, Tamanho, Meio;
Ponto PosicaoDoCampoDeVisao, PontoClicado;
//se desenhar envelope nao funcionar apagar aqui
Ponto envelopeSD, envelopeSE, envelopeID, envelopeIE; 

bool desenhaEixos = true;
bool FoiClicado = false;
bool fb=false;
bool envelope=false;

void forcaBruta(Poligono pontos);
void criaEnvelope();
void desenhaEnvelope();
void pontosEnvelope(Poligono pontos);
void pintaPoligono(
	Poligono poligono, GLfloat red, GLfloat green, GLfloat blue
);

// **********************************************************************
// GeraPontos(int qtd)
//      M�todo que gera pontos aleat�rios no intervalo [Min..Max]
// **********************************************************************
void GeraPontos(unsigned long int qtd, Ponto Min, Ponto Max)
{
	time_t t;
	Ponto Escala;
	Escala = (Max - Min) * (1.0/1000.0);
	srand((unsigned) time(&t));
	for (int i = 0;i<qtd; i++)
	{
		float x = rand() % 1000;
		float y = rand() % 1000;
		x = x * Escala.x + Min.x;
		y = y * Escala.y + Min.y;
		PontosDoCenario.insereVertice(Ponto(x,y));
	}
}

// **********************************************************************
// void CriaTrianguloDoCampoDeVisao()
//  Cria um triangulo a partir do vetor (1,0,0), girando este vetor
//  em 45 e -45 graus.
//  Este vetor fica armazenado nas vari�veis "TrianguloBase" e
//  "CampoDeVisao"
// **********************************************************************
void CriaTrianguloDoCampoDeVisao()
{
	Ponto vetor = Ponto(1,0,0);

	TrianguloBase.insereVertice(Ponto(0,0,0));
	CampoDeVisao.insereVertice(Ponto(0,0,0));

	vetor.rotacionaZ(45);
	TrianguloBase.insereVertice(vetor);
	CampoDeVisao.insereVertice(vetor);

	vetor.rotacionaZ(-90);
	TrianguloBase.insereVertice(vetor);
	CampoDeVisao.insereVertice(vetor);
}

// **********************************************************************
// void PosicionaTrianguloDoCampoDeVisao()
//  Posiciona o campo de vis�o na posicao PosicaoDoCampoDeVisao,
//  com a orientacao "AnguloDoCampoDeVisao".
//  O tamanho do campo de vis�o eh de 25% da largura da janela.
// **********************************************************************
void PosicionaTrianguloDoCampoDeVisao()
{
	float tamanho = Tamanho.x * 0.25;

	Ponto temp;
	for (int i=0;i<TrianguloBase.getNVertices();i++)
	{
		temp = TrianguloBase.getVertice(i);
		temp.rotacionaZ(AnguloDoCampoDeVisao);
		CampoDeVisao.alteraVertice(i, PosicaoDoCampoDeVisao + temp*tamanho);
	}
}
// **********************************************************************
// void AvancaCampoDeVisao(float distancia)
//  Move o campo de vis�o "distancia" unidades pra frente ou pra tras.
// **********************************************************************
void AvancaCampoDeVisao(float distancia)
{
	Ponto vetor = Ponto(1,0,0);
	vetor.rotacionaZ(AnguloDoCampoDeVisao);
	PosicaoDoCampoDeVisao = PosicaoDoCampoDeVisao + vetor * distancia;
}
// **********************************************************************
// void init()
//  Faz as inicializacoes das variaveis de estado da aplicacao
// **********************************************************************
void init()
{
	// Define a cor do fundo da tela (AZUL)
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

	// Gera ou Carrega os pontos do cenario.
	// Note que o "aspect ratio" dos pontos deve ser o mesmo
	// da janela.

	// PontosDoCenario.LePoligono("PontosDenteDeSerra.txt");
	GeraPontos(1000, Ponto(0,0), Ponto(500,500));

	PontosDoCenario.obtemLimites(Min,Max);
	Min.x--;Min.y--;
	Max.x++;Max.y++;

	Meio = (Max+Min) * 0.5; // Ponto central da janela
	Tamanho = (Max-Min);  // Tamanho da janela em X,Y

	// Ajusta variaveis do triangulo que representa o campo de visao
	PosicaoDoCampoDeVisao = Meio;
	AnguloDoCampoDeVisao = 0;

	// Cria o triangulo que representa o campo de visao
	CriaTrianguloDoCampoDeVisao();
	PosicionaTrianguloDoCampoDeVisao();
}

double nFrames=0;
double TempoTotal=0;
// **********************************************************************
//
// **********************************************************************
void animate()
{
	double dt;
	dt = T.getDeltaT();
	AccumDeltaT += dt;
	TempoTotal += dt;
	nFrames++;

	if (AccumDeltaT > 1.0/30) // fixa a atualiza��o da tela em 30
	{
		AccumDeltaT = 0;
		glutPostRedisplay();
	}
	if (TempoTotal > 5.0)
	{
		cout << "Tempo Acumulado: "  << TempoTotal << " segundos. " ;
		cout << "Nros de Frames sem desenho: " << nFrames << endl;
		cout << "FPS(sem desenho): " << nFrames/TempoTotal << endl;
		TempoTotal = 0;
		nFrames = 0;
	}
}
// **********************************************************************
//  void reshape( int w, int h )
//  trata o redimensionamento da janela OpenGL
// **********************************************************************
void reshape( int w, int h )
{
	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Define a area a ser ocupada pela area OpenGL dentro da Janela
	glViewport(0, 0, w, h);
	// Define os limites logicos da area OpenGL dentro da Janela
	glOrtho(Min.x,Max.x,
			Min.y,Max.y,
			0,1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
// **********************************************************************
//
// **********************************************************************
void DesenhaEixos()
{

	glBegin(GL_LINES);
	//  eixo horizontal
	glVertex2f(Min.x,Meio.y);
	glVertex2f(Max.x,Meio.y);
	//  eixo vertical
	glVertex2f(Meio.x,Min.y);
	glVertex2f(Meio.x,Max.y);
	glEnd();
}

void DesenhaLinha(Ponto P1, Ponto P2)
{
	glBegin(GL_LINES);
	glVertex3f(P1.x,P1.y,P1.z);
	glVertex3f(P2.x,P2.y,P2.z);
	glEnd();
}
// **********************************************************************
//  void display( void )
//
// **********************************************************************
void display( void )
{

	// Limpa a tela coma cor de fundo
	glClear(GL_COLOR_BUFFER_BIT);

	// Define os limites l�gicos da �rea OpenGL dentro da Janela
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	// Coloque aqui as chamadas das rotinas que desenham os objetos
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	if (desenhaEixos)
	{
		glLineWidth(1);
		glColor3f(1,1,1); // R, G, B  [0..1]
		DesenhaEixos();
	}

	//glPointSize(5);
	glColor3f(1,1,0); // R, G, B  [0..1]
	PontosDoCenario.desenhaVertices();

	glLineWidth(3);
	glColor3f(1,0,0); // R, G, B  [0..1]
	CampoDeVisao.desenhaPoligono();

	if (fb==true){
		pintaPoligono(PontosDoCenario, 1.0, 0.0, 0.0);
		forcaBruta(PontosDoCenario);
		pintaPoligono(eDentro, 0.0, 1.0, 0.0);
		//printa numero de pontos dentro do triangulo
		cout << "Dentro do trianulo tem " << eDentro.getNVertices() << " pontos." << endl;
		cout << "Triangulo Base: " << endl;
	} else if (envelope==true){
		criaEnvelope();
        //se desenhar envelope nao funcionar apagar aqui
        DesenhaLinha(envelopeSD,envelopeSE);
        DesenhaLinha(envelopeID,envelopeIE);
        DesenhaLinha(envelopeSD,envelopeID);
        DesenhaLinha(envelopeSE,envelopeIE);
		pintaPoligono(PontosDoCenario, 1.0, 0.0, 0.0);
		pontosEnvelope(PontosDoCenario);
		pintaPoligono(dentroEnvelope, 1.0, 1.0, 0.0);
		forcaBruta(dentroEnvelope);
		pintaPoligono(eDentro, 0.0, 1.0, 0.0);
		//printa numero de pontos dentro do triangulo
		cout << "Dentro do trianulo tem " << eDentro.getNVertices() << " pontos." << endl;
	}

	if (FoiClicado)
	{
		PontoClicado.imprime("- Ponto no universo: ", "\n");
		FoiClicado = false;
	}

	glutSwapBuffers();
}
// **********************************************************************
// ContaTempo(double tempo)
//      conta um certo n�mero de segundos e informa quanto frames
// se passaram neste per�odo.
// **********************************************************************
void ContaTempo(double tempo)
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
// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
//
// **********************************************************************
void keyboard ( unsigned char key, int x, int y )
{
	switch ( key )
	{
		case 27:        // Termina o programa qdo
			exit ( 0 );   // a tecla ESC for pressionada
			break;
		case 't':
			ContaTempo(3);
			break;
		case 'f':
			if(envelope){
				envelope=false;
			}
			fb =! fb;
			break;
		case 'e':
			if(fb){
				fb=false;
			}
			envelope=!envelope;
			break;
		case ' ':
			desenhaEixos = !desenhaEixos;
			break;
		default:
			break;
	}

	glutPostRedisplay();
}
// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )
// **********************************************************************
void arrow_keys ( int a_keys, int x, int y )
{
	switch ( a_keys )
	{
		case GLUT_KEY_LEFT:       // Se pressionar LEFT
			AnguloDoCampoDeVisao+=2;
			break;
		case GLUT_KEY_RIGHT:       // Se pressionar RIGHT
			AnguloDoCampoDeVisao-=2;
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
	cout << "Triangulo Base: " << endl;
	TrianguloBase.imprimeVertices();
	glutPostRedisplay();
}
// **********************************************************************
// Esta fun��o captura o clique do botao direito do mouse sobre a �rea de
// desenho e converte a coordenada para o sistema de refer�ncia definido
// na glOrtho (ver fun��o reshape)
// Este c�digo � baseado em http://hamala.se/forums/viewtopic.php?t=20
// **********************************************************************
void Mouse(int button,int state,int x,int y)
{
	GLint viewport[4];
	GLdouble modelview[16],projection[16];
	GLfloat wx=x,wy,wz;
	GLdouble ox=0.0,oy=0.0,oz=0.0;

	if(state!=GLUT_DOWN)
		return;
	if(button!=GLUT_RIGHT_BUTTON)
		return;
	cout << "Botao da direita! ";

	glGetIntegerv(GL_VIEWPORT,viewport);
	y=viewport[3]-y;
	wy=y;
	glGetDoublev(GL_MODELVIEW_MATRIX,modelview);
	glGetDoublev(GL_PROJECTION_MATRIX,projection);
	glReadPixels(x,y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&wz);
	gluUnProject(wx,wy,wz,modelview,projection,viewport,&ox,&oy,&oz);
	PontoClicado = Ponto(ox,oy,oz);
	FoiClicado = true;
}

void forcaBruta(Poligono pontos){
	eDentro.clear();
	Ponto p1, p2, p3;

	for (std::size_t i = 0; i < pontos.getNVertices(); i++) {
		ProdVetorial(pontos.getVertice(i), TrianguloBase.getVertice(0), p1);
		ProdVetorial(pontos.getVertice(i), TrianguloBase.getVertice(1), p2);
		ProdVetorial(pontos.getVertice(i), TrianguloBase.getVertice(2), p3);

		if(p1.z < 0 && p2.z < 0 && p3.z < 0){
			eDentro.insereVertice(pontos.getVertice(i));
		}
	}
}

void criaEnvelope(){
	envelopeMaiorX=TrianguloBase.getVertice(0).x;
	envelopeMenorX=TrianguloBase.getVertice(0).x;
	envelopeMaiorY=TrianguloBase.getVertice(0).y;
	envelopeMenorY=TrianguloBase.getVertice(0).y;
	for(int i=1; i<3;i++){
		if(TrianguloBase.getVertice(i).x>envelopeMaiorX){
			envelopeMaiorX=TrianguloBase.getVertice(i).x;
		}
		if(TrianguloBase.getVertice(i).x<envelopeMenorX){
			envelopeMenorX=TrianguloBase.getVertice(i).x;
		}
		if(TrianguloBase.getVertice(i).y>envelopeMaiorY){
			envelopeMaiorY=TrianguloBase.getVertice(i).y;
		}
		if(TrianguloBase.getVertice(i).y<envelopeMenorY){
			envelopeMenorY=TrianguloBase.getVertice(i).y;
		}
	}
    //se desenhar envelope nao funcionar apagar aqui
    envelopeSD=Ponto(envelopeMaiorX,envelopeMaiorY,0);
    envelopeSE=Ponto(envelopeMenorX,envelopeMaiorY,0);
    envelopeID=Ponto(envelopeMaiorX,envelopeMenorY,0);
    envelopeIE=Ponto(envelopeMenorX,envelopeMenorY,0);
}

void pontosEnvelope(Poligono pontos){
	dentroEnvelope.clear();

	for (std::size_t i = 0; i < pontos.getNVertices(); i++) {
		if (
			pontos.getVertice(i).x > envelopeMenorX &&
			pontos.getVertice(i).x < envelopeMaiorX &&
			pontos.getVertice(i).y > envelopeMenorY &&
			pontos.getVertice(i).y < envelopeMaiorY
		){
			dentroEnvelope.insereVertice(pontos.getVertice(i));
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

// **********************************************************************
//  void main ( int argc, char** argv )
//
// **********************************************************************
int  main ( int argc, char** argv )
{
	cout << "Programa OpenGL" << endl;

	glutInit            ( &argc, argv );
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
	glutInitWindowPosition (0,0);

	// Define o tamanho inicial da janela grafica do programa
	glutInitWindowSize  ( 500, 500);

	// Cria a janela na tela, definindo o nome da
	// que aparecera na barra de titulo da janela.
	glutCreateWindow    ( "Poligonos em OpenGL" );

	// executa algumas inicializa��es
	init ();

	// Define que o tratador de evento para
	// o redesenho da tela. A funcao "display"
	// ser� chamada automaticamente quando
	// for necess�rio redesenhar a janela
	glutDisplayFunc ( display );

	// Define que o tratador de evento para
	// o invalida��o da tela. A funcao "display"
	// ser� chamada automaticamente sempre que a
	// m�quina estiver ociosa (idle)
	glutIdleFunc(animate);

	// Define que o tratador de evento para
	// o redimensionamento da janela. A funcao "reshape"
	// ser� chamada automaticamente quando
	// o usu�rio alterar o tamanho da janela
	glutReshapeFunc ( reshape );

	// Define que o tratador de evento para
	// as teclas. A funcao "keyboard"
	// ser� chamada automaticamente sempre
	// o usu�rio pressionar uma tecla comum
	glutKeyboardFunc ( keyboard );

	// Define que o tratador de evento para
	// as teclas especiais(F1, F2,... ALT-A,
	// ALT-B, Teclas de Seta, ...).
	// A funcao "arrow_keys" ser� chamada
	// automaticamente sempre o usu�rio
	// pressionar uma tecla especial
	glutSpecialFunc ( arrow_keys );

	glutMouseFunc(Mouse);

	// inicia o tratamento dos eventos
	glutMainLoop ( );

	return 0;
}
