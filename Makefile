# Makefile para Linux e macOS

PROG = Triangulo
FONTES = Ponto.cpp Poligono.cpp Temporizador.cpp ListaDeCoresRGB.cpp PontosNoTriangulo.cpp

OBJETOS = $(FONTES:.cpp=.o)
CPPFLAGS = -g -O3 -DGL_SILENCE_DEPRECATION -Wall -g

UNAME = `uname`

all: $(TARGET)
	-@make $(UNAME)

Darwin: $(OBJETOS)
	g++ $(OBJETOS) -O3 -framework OpenGL -framework Cocoa -framework GLUT -lm -o $(PROG)

Linux: $(OBJETOS)
	g++ $(OBJETOS) -O3 -lGL -lGLU -lglut -lm -lstdc++ -o $(PROG)

clean:
	-@ rm -f $(OBJETOS) $(PROG)
