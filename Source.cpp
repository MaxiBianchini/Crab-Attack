#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/Audio.hpp"
#include <iostream>
#include <Windows.h>
#include <math.h>
#include <fstream>
using namespace sf;
using namespace std;

class Nivel {
public:
	int cantidad, objetivo;
	int* prob, * probobj;
	~Nivel(void) { delete[]prob, delete[]probobj; }
};

#define PI 3.14159265359
bool leftp, leftr, rightp, multicolor, nivel_terminado;
char nivel = 0, cantidad_de_niveles = 10, objetivo_aux, tiempo_aux;
int cantidad_cangrejos1, cantidad_cangrejos2;
Nivel* niveles;
Time tiempo_partida;
Sound sonido[3];

//int2string
int largo_num(int numero) {
	int largo = 0;
	if (numero < 0) { largo++; }
	while (numero != 0) { largo++, numero /= 10; }
	return largo;
}///int2string

int separa_numero(int numero, int posicion) {
	if (posicion > largo_num(numero) - 1) { return -1; }
	return (numero / (int)pow(10, largo_num(numero) - 1 - posicion)) % 10;
}///int2string

string int2string(int numero) {
	string cadena;
	if (numero <= 0) { return string("0"); }
	for (int i = 0; i < largo_num(numero); i++) {
		cadena += (char)separa_numero(numero, i) + 48;
	}
	return cadena;
}///int2string

bool adentro(FloatRect rect_crab, FloatRect castillo) {
	return rect_crab.left > castillo.left && rect_crab.top > castillo.top && rect_crab.left + rect_crab.width < castillo.left + castillo.width &&
		rect_crab.top + rect_crab.height < castillo.top + castillo.height;
}

bool adentro(FloatRect tacho, Vector2i mouse) {
	return mouse.x > tacho.left && mouse.x < tacho.left + tacho.width && mouse.y > tacho.top && mouse.y < tacho.top + tacho.height;
}

bool afuera_pantalla(FloatRect rect_crab) {
	return rect_crab.left > 1466 || rect_crab.left + rect_crab.width < -100 || rect_crab.top > 868 || rect_crab.top + rect_crab.height < -100;
}

void escalar(Sprite& sprite, int ancho, int alto) {
	sprite.setScale((float)ancho / sprite.getTexture()->getSize().x, (float)alto / sprite.getTexture()->getSize().y);
}

class Crab {
	friend class Cangrejos;
	Vector2f cords, cords_ola, direccion, direccionP;
	Clock reloj;
	Time time, muerte;
	Sprite sprite, ola, sangre;
	float angulo, radio, velocidad, frecuencia, delta, tiempo, escala;
	Crab* next;
	bool aparecio, desaparecio_ola, can_back, has_backed, murio;
	void sacar_angulo(void);
	void actualizar_direccion(void);
	void manejar_ola(void);
	char tipo;
public:
	bool eliminar;
	Crab(Vector2f, Texture*, Texture*, Texture*, float, float, char, char);//bool can_back = false
	void andar(void);
	void mostrar(RenderWindow*);
};

Crab* agarrado = NULL;

void Crab::sacar_angulo(void) { angulo = atan2f(384 - cords.y, 683 - cords.x) * 180 / PI; }
void Crab::actualizar_direccion(void) {
	direccion = Vector2f(1366 / 2 - cords.x, 768 / 2 - cords.y);
	float modulo = sqrt(pow(1366 / 2 - cords.x, 2) + pow(768 / 2 - cords.y, 2));
	direccion.x /= modulo;
	direccion.y /= modulo;
	direccionP.x = direccion.y;
	direccionP.y = -direccion.x;
}

Crab::Crab(Vector2f _cords, Texture* textura1, Texture* textura2, Texture* _sangre, float _radio, float _velocidad, char tipo, char nsprite) {//,bool can_back
	cords = _cords, radio = _radio, velocidad = _velocidad * (1 + ((int)tipo == 1) ? 0.5 : 0), frecuencia = _velocidad / 40, sprite.setTexture(*textura1);
	eliminar = false, aparecio = false, actualizar_direccion(), ola.setTexture(*textura2);
	tiempo = 0, sangre.setTexture(*_sangre), sangre.setOrigin(Vector2f(sangre.getTexture()->getSize()) / 2.0f), sangre.setRotation(rand() % 360);
	next = NULL, sangre.setScale(0.2f, 0.2f);
	escala = 3.0f / 11, this->tipo = tipo;
	desaparecio_ola = false, has_backed = false;
	if (tipo % 2 == 0 && rand() % 100 < 20) { can_back = true; }
	else if (tipo % 2 == 1 && rand() % 100 < 40) { can_back = true; }
	//mouse_over = has_backed = false;
	//this->can_back = can_back;

	cords_ola = Vector2f(1366 * (int)(cords.x > 683), cords.y);
	ola.setScale(1 - (int)(cords.x > 683) * 2, 1);
	ola.setOrigin(ola.getTexture()->getSize().x, 30);

	switch (nsprite) {
	case 0: cords_ola.y = 0, ola.setOrigin(ola.getOrigin().x, 0); break;
	case 2: cords_ola.y = 468, ola.setOrigin(ola.getOrigin().x, 0); break;
	}
}

void Crab::manejar_ola(void) {
	if (!aparecio) {
		if (cords_ola.x > 1366 - ola.getTexture()->getSize().x) { cords_ola.x -= delta * 400; }
		else if (cords_ola.x < ola.getTexture()->getSize().x) { cords_ola.x += delta * 400; }
		else { aparecio = true; }
	}
	else {
		if (cords_ola.x > 683 && cords_ola.x < 1366) { cords_ola.x += delta * 400; }
		else if (cords_ola.x < 683 && cords_ola.x > 0) { cords_ola.x -= delta * 400; }
		else { desaparecio_ola = true; }
	}
}

void Crab::andar() {
	delta = reloj.restart().asSeconds(), tiempo += delta;

	manejar_ola();

	if (aparecio) {
		if (!murio) {
			if (!eliminar) {
				if (time.asSeconds() > 0) {
					cords -= Vector2f(direccion.x * delta * velocidad, direccion.y * delta * velocidad);
					time -= seconds(delta);
					if (time.asSeconds() <= 0) { actualizar_direccion(); }
				}
				else {
					cords += Vector2f(direccion.x * delta * velocidad, direccion.y * delta * velocidad);
					float cosenoradio = cos(tiempo * frecuencia) * radio * delta * frecuencia;
					cords += Vector2f(direccionP.x * cosenoradio, direccionP.y * cosenoradio);
					if (tipo >= 2) {
						float sinenoradio = sin(tiempo * frecuencia) * radio * delta * frecuencia;
						if (tipo == 3) { cords += Vector2f(direccion.x * cosenoradio, direccion.y * cosenoradio); }
						else { cords += Vector2f(direccion.x * sinenoradio, direccion.y * sinenoradio); }
					}
				}
				sacar_angulo();
			}
		}
		else {
			muerte -= seconds(delta);
			if (muerte.asSeconds() > 0.4f) {
				float escala = 0.2f + (1 - muerte.asSeconds()) * 0.8f / 0.6f;
				sangre.setScale(escala, escala), sangre.setPosition(cords);;
			}
			else if (muerte.asSeconds() > 0) {
				Color color = sangre.getColor(); sangre.setPosition(cords);
				sangre.setColor(Color(color.r, color.g, color.b, muerte.asSeconds() * 255 / 0.4f));
				sprite.setColor(sangre.getColor());
			}
			else { eliminar = true; }
		}

	}
}

void Crab::mostrar(RenderWindow* Ventana) {
	if (murio) {
		Ventana->draw(sangre);
	}
	if (aparecio && !eliminar) {
		if ((int)(tiempo * 1000) % 200 > 100 && !murio) { sprite.setScale(escala, escala); }
		else { sprite.setScale(escala, -escala); }

		sprite.setOrigin(sprite.getLocalBounds().width / 2.0f, sprite.getLocalBounds().height / 2.0f);
		sprite.setPosition(cords);
		sprite.setRotation(angulo);

		Ventana->draw(sprite);
	}
	ola.setPosition(cords_ola);
	Ventana->draw(ola);
}

class Tacho {
	Sprite sprite;
	RectangleShape tacho;
public:
	Tacho(Texture*);

	void Mostrar(RenderWindow*);
	FloatRect coordenadas(void) { return tacho.getGlobalBounds(); }
};

Tacho::Tacho(Texture* textura) {
	sprite.setTexture(*textura), escalar(sprite, 150, 150), sprite.setPosition(1366 / 2, 150);
	sprite.setOrigin(sprite.getLocalBounds().width / 2.0f, sprite.getLocalBounds().height / 2.0f);
	tacho.setSize(Vector2f(100, 150)), tacho.setOrigin(tacho.getSize() / 2.0f);
	tacho.setPosition(1366 / 2, 150), tacho.setFillColor(Color::Green);
}

class Castillo {
	Sprite sprite;
	RectangleShape rectangulo;
	char vida_inicial;
	Color color;
public:
	char vida;
	Castillo(Texture*, char);
	void Mostrar(RenderWindow*);
	FloatRect coordenadas(void) { return sprite.getGlobalBounds(); }
};

Castillo::Castillo(Texture* textura, char _vida) {
	vida = vida_inicial = _vida, color = Color(0, 0, 255);
	sprite.setTexture(*textura), escalar(sprite, 250, 300), sprite.setPosition(1366 / 2, 768 / 2);
	sprite.setOrigin(sprite.getLocalBounds().width / 2.0f, sprite.getLocalBounds().height / 2.0f);
}

void Tacho::Mostrar(RenderWindow* Ventana) { Ventana->draw(sprite); }

void Castillo::Mostrar(RenderWindow* Ventana) {
	rectangulo.setSize(Vector2f(200, 20)), rectangulo.setOutlineThickness(3), rectangulo.setOutlineColor(Color::Black);
	rectangulo.setFillColor(Color::Transparent), rectangulo.setPosition(583, 10), Ventana->draw(rectangulo);

	rectangulo.setSize(Vector2f(vida * 200 / vida_inicial, 20)), rectangulo.setOutlineThickness(0);
	color.r = 255 - (vida * 255 / vida_inicial), color.b = vida * 255 / vida_inicial;
	rectangulo.setFillColor(color), Ventana->draw(rectangulo);

	Ventana->draw(sprite);
}

class Cangrejos {
public:
	Cangrejos(Castillo*, Tacho*, Event*);
	void NuevoCangrejo(Crab*);
	void ActualizarCangrejos(RenderWindow*, Vector2i*);
	void ReiniciarRelojes(void);
	void MostrarCangrejos(RenderWindow*);
	~Cangrejos(void);
private:
	float distancia;
	Event* evt;
	Castillo* castillo;
	Tacho* tacho;
	Crab* primerCangrejo;
	Crab* ultimoCangrejo;
};

Cangrejos::Cangrejos(Castillo* _castillo, Tacho* _tacho, Event* _evt) {
	primerCangrejo = ultimoCangrejo = NULL;
	castillo = _castillo, evt = _evt, tacho = _tacho;
}

void Cangrejos::NuevoCangrejo(Crab* _cangrejo) {
	if (primerCangrejo == NULL) {
		primerCangrejo = _cangrejo;
		ultimoCangrejo = primerCangrejo;
		ultimoCangrejo->next = NULL;
	}
	else {
		ultimoCangrejo->next = _cangrejo;
		ultimoCangrejo = ultimoCangrejo->next;
		ultimoCangrejo->next = NULL;
	}
}

void Cangrejos::ActualizarCangrejos(RenderWindow* _Ventana, Vector2i* mouse) {
	Crab* cangrejoActual = primerCangrejo;
	Crab* cangrejoAEliminar = NULL;
	Crab* nodoAnterior = NULL;

	while (cangrejoActual != NULL) {
		if (cangrejoActual->eliminar && cangrejoActual->desaparecio_ola) {
			cangrejoAEliminar = cangrejoActual;
			cangrejoActual = cangrejoActual->next;

			if (primerCangrejo == cangrejoAEliminar) {
				primerCangrejo = cangrejoActual;
				nodoAnterior = primerCangrejo;
			}
			else if (ultimoCangrejo == cangrejoAEliminar) {
				nodoAnterior->next = NULL;
				ultimoCangrejo = nodoAnterior;
			}
			else { nodoAnterior->next = cangrejoActual; }
			cangrejoAEliminar->next = NULL;
			delete cangrejoAEliminar;
		}
		else {
			distancia = powf(cangrejoActual->cords.x - mouse->x, 2) + powf(cangrejoActual->cords.y - mouse->y, 2);

			if (rightp && distancia < powf(60, 2) && !Mouse::isButtonPressed(Mouse::Button::Left) && !cangrejoActual->murio) {
				cangrejoActual->muerte = seconds(1), cangrejoActual->murio = true, sonido[1].play();
				if (niveles[nivel - 1].objetivo == 0 || niveles[nivel - 1].objetivo == 2) { cantidad_cangrejos1--; }
			}
			if (distancia < powf(60, 2) && agarrado == NULL) {
				multicolor = true;
				if (leftp) { agarrado = cangrejoActual; }
				if (cangrejoActual->can_back && !cangrejoActual->has_backed) {
					cangrejoActual->time = seconds(0.3f), cangrejoActual->has_backed = true;
				}

			}

			if (agarrado != cangrejoActual) { cangrejoActual->andar(); }
			else {
				cangrejoActual->delta = cangrejoActual->reloj.restart().asSeconds();
				cangrejoActual->manejar_ola();
				cangrejoActual->cords = Vector2f(*mouse);
				cangrejoActual->sacar_angulo();
			}

			if (leftr && agarrado == cangrejoActual) {
				if (adentro(tacho->coordenadas(), *mouse)) {
					cangrejoActual->eliminar = true, sonido[2].play();
					if (niveles[nivel - 1].objetivo == 1) {
						cantidad_cangrejos1--;
					}
					else if (niveles[nivel - 1].objetivo == 2) {
						cantidad_cangrejos2--;
					}
				}
				else { cangrejoActual->actualizar_direccion(); }
				agarrado = NULL;
			}

			cangrejoActual->mostrar(_Ventana);
			if (adentro(cangrejoActual->sprite.getGlobalBounds(), castillo->coordenadas()) && agarrado != cangrejoActual) {
				castillo->vida--, cangrejoActual->eliminar = true;
				if (castillo->vida <= 0) {
					nivel = -2, sonido[0].play();
				}
			}
			if (afuera_pantalla(cangrejoActual->sprite.getGlobalBounds())) { cangrejoActual->eliminar = true; }

			nodoAnterior = cangrejoActual;
			cangrejoActual = cangrejoActual->next;
		}
	}
}

void Cangrejos::ReiniciarRelojes(void) {
	Crab* cangrejoActual = primerCangrejo;
	while (cangrejoActual != NULL) {
		cangrejoActual->reloj.restart();
		cangrejoActual = cangrejoActual->next;
	}
}

void Cangrejos::MostrarCangrejos(RenderWindow* _Ventana) {
	Crab* cangrejoActual = primerCangrejo;
	while (cangrejoActual != NULL) {
		cangrejoActual->mostrar(_Ventana);
		cangrejoActual = cangrejoActual->next;
	}
}

Cangrejos::~Cangrejos(void) {
	Crab* cangrejoActual = primerCangrejo; int cantidad = 0;
	while (cangrejoActual != NULL) { cangrejoActual = cangrejoActual->next, cantidad++; }
	for (int i = 0; i < cantidad; i++) { cangrejoActual = primerCangrejo, primerCangrejo = primerCangrejo->next, delete cangrejoActual; }
}

class Juego {
	RenderWindow* Ventana;
	Event* evt;
	bool se_abrio;
	Castillo* castillo;
	Cangrejos* cangrejos;
	Tacho* tacho;
	Texture textura[20], * mobs[4];
	Vector2i* mouse;
	Sprite fondo, mano[4], boton[15], menu, gameover, creditos, instrucciones;
	Font font;
	Text texto;
	RectangleShape relojito;
	Music musica[2];
	SoundBuffer buffer[3];

	char nivel_respaldo, maus, EB[3], btn;
	Clock reloj, reloj_pausa;
	int tiempo, delta, seg_prox;

	void nuevo_nivel(void);
	bool nivel_finalizado(void);
	void Imprimir_instrucciones(float, float, bool, bool);
public:
	Juego(void);
	~Juego(void);
	bool correr_juego(void);
};

bool Juego::nivel_finalizado(void) {
	switch (niveles[nivel - 1].objetivo) {
	case 0: case 1: if (cantidad_cangrejos1 <= 0) { return true; } break;
	case 2: if (cantidad_cangrejos1 <= 0 && cantidad_cangrejos2 <= 0) { return true; } break;
	case 3: if (tiempo_partida.asSeconds() <= 0) { return true; } break;
	}
	return false;
}

void Juego::nuevo_nivel(void) {
	if (castillo != NULL) { delete castillo; }
	if (tacho != NULL) { delete tacho; }
	if (cangrejos != NULL) { delete cangrejos; }
	castillo = new Castillo(&textura[5], 20), tacho = new Tacho(&textura[9]);
	cangrejos = new Cangrejos(castillo, tacho, evt), seg_prox = rand() % 100 + 500;

	if (nivel <= 10) {
		switch (niveles[nivel - 1].objetivo) {
		case 2: cantidad_cangrejos2 = niveles[nivel - 1].probobj[1];
		case 0: case 1: cantidad_cangrejos1 = niveles[nivel - 1].probobj[0]; break;
		case 3: tiempo_partida = seconds(niveles[nivel - 1].probobj[0] * 10), tiempo_aux = niveles[nivel - 1].probobj[0]; break;
		}
	}
	else {
		objetivo_aux = rand() % 4;

		if (objetivo_aux == 0) { cantidad_cangrejos1 = rand() % 150 + 100; }
		else if (objetivo_aux == 1) { cantidad_cangrejos1 = rand() % 100 + 75; }
		else if (objetivo_aux == 2) { cantidad_cangrejos1 = rand() % 150 + 100, cantidad_cangrejos2 = rand() % 100 + 75; }
		else if (objetivo_aux == 3) { tiempo_aux = cantidad_cangrejos1 = rand() % 40 + 18; }
	}

}

Juego::Juego(void) {
	Ventana = new RenderWindow(VideoMode(1366, 768, 32), "Crab Attack!", Style::Fullscreen), evt = new Event();
	Ventana->setFramerateLimit(60), Ventana->setMouseCursorVisible(false), se_abrio = false;
	mouse = new Vector2i(), maus = 2, castillo = NULL, tacho = NULL, cangrejos = NULL;

	textura[0].loadFromFile("datos/gameover.png"), textura[1].loadFromFile("datos/ola01.png");
	textura[2].loadFromFile("datos/ola02.png"), textura[3].loadFromFile("datos/ola03.png");
	textura[4].loadFromFile("datos/arenita04.png"), textura[4].setRepeated(true), fondo.setTexture(textura[4]);
	fondo.setTextureRect(IntRect(0, 0, 1366, 768)), textura[5].loadFromFile("datos/castillo.png");
	textura[6].loadFromFile("datos/mano01.png"), textura[7].loadFromFile("datos/mano02.png");
	textura[8].loadFromFile("datos/mano03.png"), textura[9].loadFromFile("datos/palangana.png");
	mano[0].setTexture(textura[6]), mano[1].setTexture(textura[7]), mano[2].setTexture(textura[8]);
	escalar(mano[0], 50, 60), escalar(mano[1], 50, 60), escalar(mano[2], 50, 60);
	textura[10].loadFromFile("datos/Logo.png"), textura[11].loadFromFile("datos/botones.png"), menu.setTexture(textura[10]);
	textura[12].loadFromFile("datos/cangrejo1.png"), textura[13].loadFromFile("datos/cangrejo2.png");
	textura[14].loadFromFile("datos/manoapuntando.png"), mano[3].setTexture(textura[14]), escalar(mano[3], 50, 60);
	menu.setPosition(400, 50), gameover.setTexture(textura[0]), btn = 0, mano[3].setOrigin(89, 0);
	textura[15].loadFromFile("datos/credits.png"), creditos.setTexture(textura[15]), textura[16].loadFromFile("datos/sangre.png");
	textura[17].loadFromFile("datos/cangrejo3.png"), textura[18].loadFromFile("datos/cangrejo4.png");
	textura[19].loadFromFile("datos/instrucciones.png"), instrucciones.setTexture(textura[19]), instrucciones.setPosition(966, 0);
	for (int i = 0; i < 15; i++) {
		textura[i].setSmooth(true);
		boton[i].setTexture(textura[11]);
		if (i < 3) { mano[i].setOrigin(Vector2f(mano[i].getLocalBounds().width / 2.0f, mano[i].getLocalBounds().height / 2.0f)), EB[i] = 0; }
		if (i < 15) {
			boton[i].setTextureRect(IntRect(i / 3 * 151, i % 3 * 53, 151, 53));
			if (i < 9) { boton[i].setPosition(200, 300 + 100 * (i / 3)); }
			else { boton[i].setPosition(608, 600); }
		}

	}
	font.loadFromFile("datos/Random_font.ttf"), texto.setFont(font);
	texto.setFillColor(Color::Black);

	mobs[0] = &textura[12], mobs[1] = &textura[13], mobs[2] = &textura[17], mobs[3] = &textura[18];
	musica[0].openFromFile("datos/oleaje.ogg"), buffer[0].loadFromFile("datos/gameover.ogg"), buffer[1].loadFromFile("datos/pegar.ogg");
	sonido[0].setBuffer(buffer[0]), sonido[1].setBuffer(buffer[1]), musica[1].openFromFile("datos/Musica_de_ playa.ogg");
	buffer[2].loadFromFile("datos/tacho.ogg"), sonido[2].setBuffer(buffer[2]);

	niveles = new Nivel[cantidad_de_niveles]();
	ifstream partida("datos/niveles.bin", ios::in | ios::binary);
	for (int i = 0; i < cantidad_de_niveles; i++) {
		partida.read((char*)&niveles[i].cantidad, sizeof(niveles[i].cantidad));
		niveles[i].prob = new int[niveles[i].cantidad];
		for (int j = 0; j < niveles[i].cantidad; j++) { partida.read((char*)&niveles[i].prob[j], sizeof(niveles[i].prob[j])); }
		partida.read((char*)&niveles[i].objetivo, sizeof(niveles[i].objetivo));
		char repeticion = 0;
		if (niveles[i].objetivo == 2) { repeticion = 2; }
		else { repeticion = 1; }
		niveles[i].probobj = new int[repeticion];
		for (int j = 0; j < repeticion; j++) { partida.read((char*)&niveles[i].probobj[j], sizeof(niveles[i].probobj[j])); }
	}
	partida.close();

	for (int i = 0; i < 3; i++) {
		boton[12 + i].setPosition(boton[12 + i].getPosition().x, boton[12 + i].getPosition().y + 60);
	}
}

void Juego::Imprimir_instrucciones(float x, float y, bool mostrar_tiempo, bool centrar) {
	if (!centrar) { texto.setOrigin(0, 0); }
	char objetivo = objetivo_aux;
	if (nivel <= 10) { objetivo = niveles[nivel - 1].objetivo; }

	switch (objetivo) {
	case 0: texto.setString("CRABS TO KILL: " + int2string(cantidad_cangrejos1)); break;
	case 1: texto.setString("CRABS TO CATCH " + int2string(cantidad_cangrejos1)); break;
	case 2:
		texto.setString("CRABS TO CATCH " + int2string(cantidad_cangrejos2));
		if (centrar) { texto.setOrigin(texto.getGlobalBounds().width / 2.0f, texto.getGlobalBounds().height / 2.0f); }
		texto.setPosition(x, y + 40), Ventana->draw(texto);
		texto.setString("CRABS TO KILL " + int2string(cantidad_cangrejos1));
		break;
	case 3:
		texto.setString("SURVIVE FOR " + int2string((int)tiempo_partida.asSeconds()) + " SECONDS");
		if (mostrar_tiempo) {
			tiempo_partida -= milliseconds(delta);
			relojito.setSize(Vector2f(500, 20)), relojito.setOutlineThickness(3), relojito.setOutlineColor(Color::Black);
			relojito.setFillColor(Color::Transparent), relojito.setPosition(x, y + 50), Ventana->draw(relojito);
			relojito.setSize(Vector2f(tiempo_partida.asSeconds() * 500 / (tiempo_aux * 10), 20)), relojito.setOutlineThickness(0);
			relojito.setFillColor(Color(0, 150, 0)), Ventana->draw(relojito);
		}
	}
	if (centrar) { texto.setOrigin(texto.getGlobalBounds().width / 2.0f, texto.getGlobalBounds().height / 2.0f); }
	texto.setPosition(x, y + 0), Ventana->draw(texto);
}

bool Juego::correr_juego(void) {
	bool salir = false;

	while (!salir) {
		Ventana->clear(Color::Black);
		Ventana->draw(fondo);
		delta = reloj.restart().asMilliseconds();
		if (nivel != -3) { tiempo += delta; }
		*mouse = Mouse::getPosition(*Ventana);

		leftp = false, leftr = false, rightp = false;

		while (Ventana->pollEvent(*evt)) {
			switch (evt->type) {
			case Event::Closed: salir = true; break;
			case Event::KeyPressed:
				/*if (evt->key.code == Keyboard::Space) {
					nivel_terminado = true;
					nivel++, nuevo_nivel();
					for (int i = 0; i < 3; i++) { boton[9 + i].setPosition(1000, 380); }
				}*/
				if (evt->key.code == Keyboard::Escape && !nivel_terminado && (nivel > 0 || nivel == -3)) {
					if (nivel > 0) {
						nivel_respaldo = nivel, nivel = -3, reloj_pausa.restart();
						boton[9].setPosition(200, 400), boton[10].setPosition(200, 400), boton[11].setPosition(200, 400);
					}
					else {
						nivel = nivel_respaldo;
					}
				}
				break;
			case Event::MouseButtonPressed:
				if (evt->mouseButton.button == Mouse::Button::Left) { leftp = true; }
				else if (evt->mouseButton.button == Mouse::Button::Right) { rightp = true; }
				break;
			case Event::MouseButtonReleased:
				if (evt->mouseButton.button == Mouse::Button::Left) {
					leftr = true;
					if (nivel == 0) {
						for (int i = 0; i < 3; i++) {
							if (EB[i] == 2) {
								EB[i] = 0, btn = 0;
								switch (i) {
								case 0: nivel = 1, nuevo_nivel(), tiempo = 0; break;
								case 1:	nivel = -1;  break;
								case 2: salir = true; break;
								}
							}
						}
					}
					else if (nivel < 0 && nivel >= -2 && btn == 2) {
						nivel = 0, btn = 0;
					}
					else if (nivel == -3) {
						for (int i = 0; i < 3; i += 2) {
							if (EB[i] == 2) {
								EB[i] = 0, btn = 0;
								switch (i) {
								case 0: nivel = nivel_respaldo; break;
								case 2: nivel = 0; break;
								}
							}
						}
					}
				}
				break;
			}
		}


		if (musica[0].getStatus() == Sound::Status::Stopped) { musica[0].play(); }
		if (musica[1].getStatus() == Sound::Status::Stopped) { musica[1].play(); }

		if (!se_abrio) {
			Ventana->clear(Color::Black);
			se_abrio = true;
		}
		else {
			if (nivel == 0 || nivel == -3) {
				Ventana->draw(menu);
				Ventana->draw(instrucciones);
				if (nivel == 0) {
					for (int i = 0; i < 3; i++) {
						if (adentro(boton[i * 3].getGlobalBounds(), *mouse)) {
							if (Mouse::isButtonPressed(Mouse::Button::Left)) { EB[i] = 2; }
							else { EB[i] = 1; }
						}
						else { EB[i] = 0; }
						Ventana->draw(boton[i * 3 + EB[i]]);
					}
				}
				else {
					if (adentro(boton[6].getGlobalBounds(), *mouse)) {
						if (Mouse::isButtonPressed(Mouse::Button::Left)) { EB[2] = 2; }
						else { EB[2] = 1; }
					}
					else { EB[2] = 0; }
					Ventana->draw(boton[6 + EB[2]]);


					if (adentro(boton[9].getGlobalBounds(), *mouse)) {
						if (Mouse::isButtonPressed(Mouse::Button::Left)) { EB[0] = 2; }
						else { EB[0] = 1; }
					}
					else { EB[0] = 0; }
					Ventana->draw(boton[9 + EB[0]]);

					texto.setFillColor(Color::Black), texto.setCharacterSize(200), texto.setString("PAUSE");
					texto.setOrigin(texto.getGlobalBounds().width / 2.0f, texto.getGlobalBounds().height / 2.0f);
					texto.setPosition(270, 200);
					if (reloj_pausa.getElapsedTime().asMilliseconds() % 1000 < 500) { Ventana->draw(texto); }

					cangrejos->ReiniciarRelojes();
				}
			}
			else if (nivel > 0) {
				castillo->Mostrar(Ventana);
				tacho->Mostrar(Ventana);

				if (!nivel_terminado) {
					if (tiempo > seg_prox) {
						char ola = 0, aleatorio = rand() % 100, tipo = 0, nivel_calculo = nivel;
						if (nivel_calculo > cantidad_de_niveles) { nivel_calculo = cantidad_de_niveles; }
						int tiempo_suma = rand() % 100 + 1608.513587 * exp(-0.069845406 * nivel);
						if (niveles[nivel_calculo - 1].objetivo == 3) { tiempo_suma *= 0.8f; }
						seg_prox += tiempo_suma;

						Vector2f posicion = Vector2f(rand() % 200 + rand() % 2 * 1166, rand() % 668);


						for (int i = 0, porcentaje = niveles[nivel_calculo - 1].prob[i]; i < niveles[nivel_calculo - 1].cantidad; i++, porcentaje += niveles[nivel_calculo - 1].prob[i]) {

							if (aleatorio < porcentaje) { tipo = i; break; }
						}

						ola = (int)(posicion.y > 200) + (int)(posicion.y >= 468);
						cangrejos->NuevoCangrejo(new Crab(posicion, mobs[tipo], &textura[1 + ola], &textura[16], rand() % 100 + 50, rand() % 100 + 150, tipo, ola));//(float)(rand() % 200 + 100) / 2.0f

					}

					multicolor = false;
					cangrejos->ActualizarCangrejos(Ventana, mouse);

					texto.setCharacterSize(40), texto.setOrigin(0, 0);
					texto.setPosition(10, 0), texto.setString("Level: " + int2string((int)nivel));
					Ventana->draw(texto);

					Imprimir_instrucciones(800, 0, true, false);

					nivel_terminado = nivel_finalizado();
					if (nivel_terminado) {
						nivel++, nuevo_nivel();
						for (int i = 0; i < 3; i++) { boton[9 + i].setPosition(1000, 380); }
					}
				}
				else {
					texto.setCharacterSize(80), texto.setString("LEVEL COMPLETE!");
					texto.setOrigin(texto.getGlobalBounds().width / 2.0f, texto.getGlobalBounds().height / 2.0f);
					texto.setPosition(1366 / 2, 180), Ventana->draw(texto);

					texto.setCharacterSize(50), texto.setString("LEVEL " + int2string((int)nivel) + ":");
					texto.setOrigin(texto.getGlobalBounds().width / 2.0f, texto.getGlobalBounds().height / 2.0f);
					texto.setPosition(1366 / 2, 540), Ventana->draw(texto);

					Imprimir_instrucciones(1366 / 2, 580, false, true);


					if (adentro(boton[9 + btn].getGlobalBounds(), *mouse)) {
						btn = 1;
						if (Mouse::isButtonPressed(Mouse::Button::Left)) { btn = 2; }
					}
					else { btn = 0; }
					Ventana->draw(boton[9 + btn]);

					if (leftp && nivel_terminado && btn == 2) { btn = 0, nivel_terminado = false, tiempo = 0; }
					if (nivel > 10) { nivel = 0; }

					cangrejos->ReiniciarRelojes();
				}


			}
			else if (nivel < 0 && nivel >= -2) {
				if (nivel == -2) {
					Ventana->draw(gameover);
					texto.setFillColor(Color::White), texto.setCharacterSize(60), texto.setString("GAME OVER");
					texto.setOrigin(texto.getGlobalBounds().width / 2.0f, texto.getGlobalBounds().height / 2.0f);
					texto.setPosition(1366 / 2, 200), Ventana->draw(texto);
				}
				else { Ventana->draw(creditos); }

				for (int i = 0; i < 3; i++) {
					if (adentro(boton[12 + btn].getGlobalBounds(), *mouse)) {
						btn = 1;
						if (Mouse::isButtonPressed(Mouse::Button::Left)) { btn = 2; }
					}
					else { btn = 0; }
					Ventana->draw(boton[12 + btn]);

				}

			}


		}

		if (!Mouse::isButtonPressed(Mouse::Button::Left)) { agarrado = NULL; }

		if (nivel > 0 && !nivel_terminado) {
			if (agarrado == NULL) {
				if (Mouse::isButtonPressed(Mouse::Button::Right)) { maus = 0; }
				else { maus = 2; }
			}
			else { maus = 1; }
		}
		else { maus = 3; }



		if (maus == 2) {
			if (multicolor) {
				short random = rand() % 255;
				mano[maus].setColor(Color(random, random, random));
			}
			else { mano[maus].setColor(Color(255, 255, 255)); }
		}

		mano[maus].setPosition(mouse->x, mouse->y);
		Ventana->draw(mano[maus]);

		Ventana->display();

	}

	return salir;
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	ShowWindow(GetConsoleWindow(), 0); // Ocultar consola
	Juego* juego = new Juego();
	while (!juego->correr_juego());
	delete juego;
	return 0;
}

Juego::~Juego(void) {
	if (castillo != NULL) { delete castillo; }
	if (tacho != NULL) { delete tacho; }
	if (cangrejos != NULL) { delete cangrejos; }
	delete evt, delete mouse, delete Ventana;
	delete[]niveles;
}