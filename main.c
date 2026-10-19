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

int main()
{
	const int sHeight = 720;
	const int sWidth = 1280;

	int puntajes[10];
	int puntajeActual = 0;
	char tPuntaje[50];
	int tAncho;

	// inicialización del arreglo con nueva estructura
	Topo hoyos[MAX_TOPOS] = {
		{{sWidth / 3, sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{2 * sWidth / 3, sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{sWidth / 4, 2 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{2 * sWidth / 4, 2 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{3 * sWidth / 4, 2 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{sWidth / 3, 3 * sHeight / 4}, true, 0.0f, TOPO_NORMAL},
		{{2 * sWidth / 3, 3 * sHeight / 4}, true, 0.0f, TOPO_NORMAL}};

	InitWindow(sWidth, sHeight, "guacamole");

	Image topo = LoadImage("topo.png");
	Texture textura = LoadTextureFromImage(topo);
	textura.height = 100;
	textura.width = 100;
	UnloadImage(topo);

	SetTargetFPS(60);

	// bucle principal del juego
	while (!WindowShouldClose())
	{
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
						puntajeActual += 100;
					}
				}
			}
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

		EndDrawing();
	}

	// Desinicialización
	UnloadTexture(textura);
	CloseWindow();

	return 0;
}
