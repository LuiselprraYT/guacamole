#include "raylib.h"
#define MAX_TOPOS 7

// tipos de topitos
typedef enum
{
	TOPO_NORMAL,
	TOPO_DORADO,
	TOPO_BOMBA
} TipoTopo;

// struct de cada hoyo y topo
typedef struct
{
	Vector2 posicion;
	bool activo;
	float tiempoRestante;
	TipoTopo tipo;
} Topo;

void mostrarClasificacion(int c[], int index, int w, int h);

int main()
{
	const int sHeight = 720;
	const int sWidth = 1280;

	int puntajes[10];
	int puntajeActual = 0;
	char tPuntaje[50];
	int tAncho;

	bool clasi = false;
	
	InitWindow(sWidth, sHeight, "Guacamole");

	// inicialización del arreglo con nueva estructura
	Topo hoyos[MAX_TOPOS] = {
		{{sWidth / 3, sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{2 * sWidth / 3, sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{sWidth / 4, 2 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{2 * sWidth / 4, 2 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{3 * sWidth / 4, 2 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{sWidth / 3, 3 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{2 * sWidth / 3, 3 * sHeight / 4}, true, 0.0f, TOPO_NORMAL}};
	// topo normal
	Image topo = LoadImage("topo.png");
	Texture textura = LoadTextureFromImage(topo);
	textura.height = 100;
	textura.width = 100;
	UnloadImage(topo);
	// topo dorado
	Image topo_dorado = LoadImage("topo_dorado.png");
	Texture textura_dorado = LoadTextureFromImage(topo_dorado);
	textura.height = 100;
	textura.width = 100;
	// topo bomba
	Image topo_bomba = LoadImage("topo_bomba.png");
	Texture textura_bomba = LoadTextureFromImage(topo_bomba);
	textura.height = 100;
	textura.width = 100;
	SetTargetFPS(60);

	// Variables para todo el tema del control del teimpo
	float tiempoJuego = 45.0f, tiempoDeAparicion = 0.9f;
	bool gameOver = false;

	// bucle principal del juego
	while (!WindowShouldClose())
	{
		float dt = GetFrameTime(); // tiempo trancurrido entre frames
		// cronometro del juego
		if (!gameOver)
		{
			tiempoJuego -= dt;
			gameOver = true;
		}
		// aparicion del topo
		tiempoDeAparicion -= dt;
		if (tiempoDeAparicion <= 0.0f)
		{
			tiempoDeAparicion = 0.9f; // reiniciar tiempo en cada topo
			int indice = GetRandomValue(0, MAX_TOPOS - 1);
			if (!hoyos[indice].activo)
			{
				hoyos[indice].activo = true;
				hoyos[indice].tiempoRestante = 1.2f; // topo arriba

				// probabilidaddes de cada too arriba
				int probabilidad = GetRandomValue(1, 100);
				if (probabilidad < 60)
					hoyos[indice].tipo = TOPO_NORMAL;
				else if (probabilidad <= 85)
				{
					hoyos[indice].tipo = TOPO_DORADO;
				}
				else
				{
					hoyos[indice].tipo = TOPO_BOMBA;
				}
			}
		}
		// detección de clics con el mouse
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			Vector2 mousePos = GetMousePosition();

			for (int i = 0; i < MAX_TOPOS; i++)
			{
				if (hoyos[i].activo)
				{
					// rectangulo para poder dar clic al topo
					Rectangle areaTopo = {
						hoyos[i].posicion.x - textura.width / 2.0f,
						hoyos[i].posicion.y - textura.height / 2.0f,
						(float)textura.width,
						(float)textura.height};
					// se verifica si el clic cae dentro del recatngulo del topito y si si se oculta
					if (CheckCollisionPointRec(mousePos, areaTopo))
					{
						hoyos[i].activo = false;
						switch(hoyos[i].tipo)
						{
							case TOPO_NORMAL:
								puntajeActual += 100;
								break;
							case TOPO_DORADO:
								puntajeActual += 500;
								break;
							case TOPO_BOMBA:
								puntajeActual -= 200;
								break;
						}
					}
				}
			}
		}

		// manejo de teclas
		if(IsKeyPressed(KEY_TAB))
		{
			clasi = (clasi) ? false : true;
		}
		
		// Maneja el texto de puntaje
		TextCopy(tPuntaje, TextFormat("Puntaje: %d", puntajeActual));
		tAncho = MeasureText(tPuntaje, 20);

		// Dibujado
		BeginDrawing();
		ClearBackground(BLACK);

		DrawText(tPuntaje, (sWidth - tAncho) / 2, sHeight / 100, 20, MAGENTA);

		for (int i = 0; i < MAX_TOPOS; i++)
		{
			if (hoyos[i].activo)
			{
				DrawTexture(textura, hoyos[i].posicion.x - textura.width / 2, hoyos[i].posicion.y - textura.height / 2, WHITE);
			}
		}

		if (clasi)
		{
			mostrarClasificacion(puntajes, 1, sWidth, sHeight);
		}
		EndDrawing();
	}

	// Desinicialización
	UnloadTexture(textura);
	CloseWindow();

	return 0;
}

void mostrarClasificacion(int c[], int index, int w, int h)
{
	float size = 0.5;
	int width = w * size;
	int height = h * size; 
	float off = 0.25;

	DrawRectangle(w * off, h * off, width, height, DARKGRAY);
	DrawText("Tabla de clasificacion", w * off + 5, h * off + 10, 20, MAGENTA);

	for(int i = 0; i < 10; i++)
	{
		DrawText(TextFormat("%d- %d", i + 1, c[i]), w * off + 20, h * off + 50 + (25 * i), 20, MAGENTA);
	}
}
