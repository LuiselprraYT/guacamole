#include "raylib.h"
#include <math.h>
#include <stddef.h>

#define MAX_TOPOS 7
#define ANCHO_TOPO 160 // ancho al que se redimensionan los sprites

// ===================== AJUSTES DE VELOCIDAD =====================
#define TIEMPO_ENTRE_TOPOS 1.0f // segundos entre cada aparición (menor = salen más seguido)
#define DURACION_TOPO 2.0f		// segundos totales que un topo permanece en pantalla
#define TIEMPO_ANIMACION 0.3f	// segundos que tarda en subir y en bajar (menor = más rápido)
#define DURACION_PARTIDA 25.0f	// segundos que dura cada partida
// ================================================================

// ===================== AJUSTES DE TEXTOS FLOTANTES ==============
#define MAX_POPUPS 10
#define DURACION_POPUP 1.0f // segundos que se ve el puntaje sobre el topo
#define TAM_POPUP 48		// tamaño de letra del puntaje flotante
// ================================================================

// estados del juego
typedef enum
{
	ESTADO_INICIO,
	ESTADO_JUGANDO,
	ESTADO_FIN
} Estado;

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
	bool golpeado; // ya recibió un clic y se está escondiendo
} Topo;

// puntaje temporal que flota sobre la cabeza del topo
typedef struct
{
	Vector2 posicion;
	float tiempo; // segundos que le quedan
	int valor;	  // puntos ganados (+) o perdidos (-)
	bool activo;
} Popup;

// Carga una imagen, la redimensiona manteniendo proporción y la convierte en textura
static Texture CargarTopo(const char *archivo)
{
	Image img = LoadImage(archivo);
	if (img.data == NULL)
	{
		// imagen no encontrada: cuadro magenta para que el juego no se cierre
		img = GenImageColor(ANCHO_TOPO, ANCHO_TOPO / 2, MAGENTA);
	}
	int alto = img.height * ANCHO_TOPO / img.width;
	ImageResize(&img, ANCHO_TOPO, alto);
	Texture tex = LoadTextureFromImage(img);
	UnloadImage(img);
	return tex;
}

// Suaviza el movimiento (empieza y termina lento)
static float Suavizar(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

// Devuelve qué tan afuera del hoyo está el topo: 0 = escondido, 1 = totalmente arriba
static float ProgresoTopo(const Topo *h)
{
	float subida = (DURACION_TOPO - h->tiempoRestante) / TIEMPO_ANIMACION;
	float bajada = h->tiempoRestante / TIEMPO_ANIMACION;
	float p = fminf(fminf(subida, bajada), 1.0f);
	if (p < 0.0f)
		p = 0.0f;
	return Suavizar(p);
}

// Crea un puntaje flotante en el primer espacio libre
static void CrearPopup(Popup popups[], Vector2 posicion, int valor)
{
	for (int i = 0; i < MAX_POPUPS; i++)
	{
		if (!popups[i].activo)
		{
			popups[i].posicion = posicion;
			popups[i].tiempo = DURACION_POPUP;
			popups[i].valor = valor;
			popups[i].activo = true;
			return;
		}
	}
}

// Texto con contorno para que se lea sobre cualquier fondo
static void DrawTextoContorno(const char *texto, int x, int y, int tam, Color color, Color contorno, int grosor)
{
	DrawText(texto, x - grosor, y, tam, contorno);
	DrawText(texto, x + grosor, y, tam, contorno);
	DrawText(texto, x, y - grosor, tam, contorno);
	DrawText(texto, x, y + grosor, tam, contorno);
	DrawText(texto, x, y, tam, color);
}

// Dibuja un topo mostrando solo la parte que ya salió del hoyo
static void DibujarTopo(Texture t, Vector2 centro, float progreso)
{
	int x = (int)(centro.x - t.width / 2);
	int yArriba = (int)(centro.y - t.height / 2);
	int desplazamiento = (int)((1.0f - progreso) * t.height);
	int altoVisible = t.height - desplazamiento;

	if (altoVisible <= 0)
		return;

	// Se recorta la parte de abajo del sprite: así parece que sube (o se hunde) en el hoyo
	Rectangle origen = {0, 0, (float)t.width, (float)altoVisible};
	Vector2 destino = {(float)x, (float)(yArriba + desplazamiento)};
	DrawTextureRec(t, origen, destino, WHITE);
}

int main()
{
	// Tamaño "lógico" del juego: todo se dibuja a 1280x720 y luego se escala a la ventana
	const int sHeight = 720;
	const int sWidth = 1280;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(sWidth, sHeight, "Guacamole");
	SetWindowMinSize(640, 360);
	MaximizeWindow(); // arranca ocupando toda la pantalla (con barra de título)
	SetTargetFPS(60);

	// Lienzo donde se dibuja el juego; después se estira a la ventana
	RenderTexture2D lienzo = LoadRenderTexture(sWidth, sHeight);
	SetTextureFilter(lienzo.texture, TEXTURE_FILTER_BILINEAR);

	int puntajeActual = 0;
	int mejorPuntaje = 0;

	// Todos los hoyos empiezan vacíos
	Topo hoyos[MAX_TOPOS] = {
		{{sWidth / 3, sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{2 * sWidth / 3, sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{sWidth / 4, 2 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{2 * sWidth / 4, 2 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{3 * sWidth / 4, 2 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{sWidth / 3, 3 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{2 * sWidth / 3, 3 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false}};

	// Puntajes flotantes (todos empiezan inactivos)
	Popup popups[MAX_POPUPS] = {0};

	// Texturas
	Texture fondo = LoadTexture("fondo.png");
	Texture texNormal = CargarTopo("topo.png");
	Texture texDorado = CargarTopo("topo_dorado.png");
	Texture texBomba = CargarTopo("topo_bomba.png");

	// Variables de control del tiempo
	float tiempoJuego = DURACION_PARTIDA;
	float tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;
	Estado estado = ESTADO_INICIO;

	// bucle principal del juego
	while (!WindowShouldClose())
	{
		float dt = GetFrameTime();

		// F11 alterna pantalla completa
		if (IsKeyPressed(KEY_F1))
			ToggleBorderlessWindowed();

		// Cálculo de cómo se acomoda el lienzo en la ventana (conserva proporción)
		float escala = fminf((float)GetScreenWidth() / sWidth, (float)GetScreenHeight() / sHeight);
		float anchoDestino = sWidth * escala;
		float altoDestino = sHeight * escala;
		Rectangle destino = {
			(GetScreenWidth() - anchoDestino) / 2.0f,
			(GetScreenHeight() - altoDestino) / 2.0f,
			anchoDestino,
			altoDestino};

		// Posición del mouse convertida a las coordenadas del juego (1280x720)
		Vector2 mousePos = GetMousePosition();
		mousePos.x = (mousePos.x - destino.x) / escala;
		mousePos.y = (mousePos.y - destino.y) / escala;

		// los puntajes flotantes siempre avanzan, incluso al terminar la partida
		for (int i = 0; i < MAX_POPUPS; i++)
		{
			if (popups[i].activo)
			{
				popups[i].tiempo -= dt;
				if (popups[i].tiempo <= 0.0f)
					popups[i].activo = false;
			}
		}

		if (estado == ESTADO_JUGANDO)
		{
			// cronómetro del juego
			tiempoJuego -= dt;
			if (tiempoJuego <= 0.0f)
			{
				tiempoJuego = 0.0f;
				estado = ESTADO_FIN;
				if (puntajeActual > mejorPuntaje)
					mejorPuntaje = puntajeActual;
			}

			// los topos se esconden solos cuando se les acaba el tiempo
			for (int i = 0; i < MAX_TOPOS; i++)
			{
				if (hoyos[i].activo)
				{
					hoyos[i].tiempoRestante -= dt;
					if (hoyos[i].tiempoRestante <= 0.0f)
						hoyos[i].activo = false;
				}
			}

			// aparición de un topo nuevo
			tiempoDeAparicion -= dt;
			if (tiempoDeAparicion <= 0.0f)
			{
				tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;
				int indice = GetRandomValue(0, MAX_TOPOS - 1);
				if (!hoyos[indice].activo)
				{
					hoyos[indice].activo = true;
					hoyos[indice].golpeado = false;
					hoyos[indice].tiempoRestante = DURACION_TOPO;

					// probabilidades: 60% normal, 25% dorado, 15% bomba
					int probabilidad = GetRandomValue(1, 100);
					if (probabilidad <= 60)
						hoyos[indice].tipo = TOPO_NORMAL;
					else if (probabilidad <= 85)
						hoyos[indice].tipo = TOPO_DORADO;
					else
						hoyos[indice].tipo = TOPO_BOMBA;
				}
			}

			// detección de clics
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				for (int i = 0; i < MAX_TOPOS; i++)
				{
					float p = ProgresoTopo(&hoyos[i]);

					// solo se puede golpear si está activo, sin golpear y ya salió lo suficiente
					if (!hoyos[i].activo || hoyos[i].golpeado || p < 0.5f)
						continue;

					Texture t = (hoyos[i].tipo == TOPO_DORADO)	? texDorado
								: (hoyos[i].tipo == TOPO_BOMBA) ? texBomba
																: texNormal;

					Rectangle areaTopo = {
						hoyos[i].posicion.x - t.width / 2.0f,
						hoyos[i].posicion.y - t.height / 2.0f,
						(float)t.width,
						(float)t.height};

					if (CheckCollisionPointRec(mousePos, areaTopo))
					{
						// el topo empieza a esconderse de inmediato
						hoyos[i].golpeado = true;
						if (hoyos[i].tiempoRestante > TIEMPO_ANIMACION)
							hoyos[i].tiempoRestante = TIEMPO_ANIMACION;

						// puntaje según el tipo de topo
						int antes = puntajeActual;
						switch (hoyos[i].tipo)
						{
						case TOPO_NORMAL:
							puntajeActual += 100;
							break;
						case TOPO_DORADO:
							puntajeActual += 300;
							break;
						case TOPO_BOMBA:
							puntajeActual -= 200;
							if (puntajeActual < 0)
								puntajeActual = 0;
							break;
						}

						// puntaje flotante sobre la cabeza del topo (muestra lo que realmente cambió)
						Vector2 cabeza = {
							hoyos[i].posicion.x,
							hoyos[i].posicion.y - t.height / 2.0f + (1.0f - p) * t.height - 10.0f};
						CrearPopup(popups, cabeza, puntajeActual - antes);
						break; // un clic solo golpea un topo
					}
				}
			}
		}
		else if (IsKeyPressed(KEY_SPACE))
		{
			// ESPACIO inicia el juego (desde la pantalla de inicio) o lo reinicia (desde el fin)
			estado = ESTADO_JUGANDO;
			puntajeActual = 0;
			tiempoJuego = DURACION_PARTIDA;
			tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;
			for (int i = 0; i < MAX_TOPOS; i++)
			{
				hoyos[i].activo = false;
				hoyos[i].golpeado = false;
			}
			for (int i = 0; i < MAX_POPUPS; i++)
				popups[i].activo = false;
		}

		// ---------- Dibujado dentro del lienzo de 1280x720 ----------
		BeginTextureMode(lienzo);
		ClearBackground(DARKGREEN);
		if (fondo.id != 0)
			DrawTexture(fondo, 0, 0, WHITE);

		// Barra superior (la franja oscura viene en fondo.png, 68 px de alto)
		if (estado != ESTADO_INICIO)
		{
			const int tamHUD = 34;
			const int yHUD = (68 - tamHUD) / 2;

			const char *textoPuntaje = TextFormat("Puntaje: %d", puntajeActual);
			DrawText(textoPuntaje, (sWidth - MeasureText(textoPuntaje, tamHUD)) / 2, yHUD, tamHUD, MAGENTA);

			// el tiempo se pone rojo en los últimos 10 segundos
			const char *textoTiempo = TextFormat("Tiempo: %d", (int)ceilf(tiempoJuego));
			DrawText(textoTiempo, 24, yHUD, tamHUD, (tiempoJuego <= 10.0f) ? RED : WHITE);

			const char *textoMejor = TextFormat("Mejor: %d", mejorPuntaje);
			DrawText(textoMejor, sWidth - MeasureText(textoMejor, tamHUD) - 24, yHUD, tamHUD, YELLOW);
		}

		// Topos
		for (int i = 0; i < MAX_TOPOS; i++)
		{
			if (!hoyos[i].activo)
				continue;

			Texture t = (hoyos[i].tipo == TOPO_DORADO)	? texDorado
						: (hoyos[i].tipo == TOPO_BOMBA) ? texBomba
														: texNormal;
			DibujarTopo(t, hoyos[i].posicion, ProgresoTopo(&hoyos[i]));
		}

		// Puntajes flotantes: suben y se desvanecen
		for (int i = 0; i < MAX_POPUPS; i++)
		{
			if (!popups[i].activo)
				continue;

			float avance = 1.0f - popups[i].tiempo / DURACION_POPUP; // 0 → 1
			float alfa = fminf(1.0f, popups[i].tiempo / (DURACION_POPUP * 0.4f));
			int y = (int)(popups[i].posicion.y - avance * 70.0f);

			const char *texto = (popups[i].valor > 0)	? TextFormat("+%d", popups[i].valor)
								: (popups[i].valor < 0) ? TextFormat("%d", popups[i].valor)
														: "0";
			Color color = (popups[i].valor >= 300) ? GOLD
						  : (popups[i].valor > 0)  ? LIME
												   : RED;
			int x = (int)popups[i].posicion.x - MeasureText(texto, TAM_POPUP) / 2;

			DrawTextoContorno(texto, x, y, TAM_POPUP, Fade(color, alfa), Fade(BLACK, alfa), 3);
		}

		// Pantalla de inicio
		if (estado == ESTADO_INICIO)
		{
			DrawRectangle(0, 0, sWidth, sHeight, Fade(BLACK, 0.55f));

			const char *titulo = "GUACAMOLE";
			DrawTextoContorno(titulo, (sWidth - MeasureText(titulo, 96)) / 2, 70, 96, GOLD, BLACK, 4);

			// botón START
			Rectangle boton = {sWidth / 2.0f - 190, 210, 380, 120};
			DrawRectangleRounded((Rectangle){boton.x - 6, boton.y - 6, boton.width + 12, boton.height + 12}, 0.35f, 10, WHITE);
			DrawRectangleRounded(boton, 0.35f, 10, (Color){30, 140, 50, 255});
			const char *start = "START";
			DrawText(start, (sWidth - MeasureText(start, 72)) / 2, (int)boton.y + 24, 72, WHITE);

			// mensaje parpadeante
			float pulso = 0.55f + 0.45f * sinf((float)GetTime() * 4.0f);
			const char *aviso = "Presiona ESPACIO para comenzar";
			DrawText(aviso, (sWidth - MeasureText(aviso, 36)) / 2, 360, 36, Fade(WHITE, pulso));

			// leyenda: cuánto vale cada topo
			Texture ley[3] = {texNormal, texDorado, texBomba};
			const char *etiquetas[3] = {"+100", "+300", "-200"};
			Color colores[3] = {LIME, GOLD, RED};
			for (int i = 0; i < 3; i++)
			{
				int cx = sWidth / 2 + (i - 1) * 300;
				DrawTexture(ley[i], cx - ley[i].width / 2, 430, WHITE);
				DrawTextoContorno(etiquetas[i], cx - MeasureText(etiquetas[i], 44) / 2, 540, 44, colores[i], BLACK, 3);
			}

			const char *ayuda = "Dale clic a los topos con el mouse. ¡Cuidado con la bomba!";
			DrawText(ayuda, (sWidth - MeasureText(ayuda, 28)) / 2, 625, 28, WHITE);
			const char *pantalla = "F11: pantalla completa";
			DrawText(pantalla, (sWidth - MeasureText(pantalla, 20)) / 2, 675, 20, LIGHTGRAY);
		}

		// Pantalla de fin de juego
		if (estado == ESTADO_FIN)
		{
			DrawRectangle(0, 0, sWidth, sHeight, Fade(BLACK, 0.75f));

			const char *fin = "FIN DEL JUEGO";
			const char *final = TextFormat("Puntaje final: %d", puntajeActual);
			const char *reiniciar = "Presiona ESPACIO para jugar de nuevo";

			DrawText(fin, (sWidth - MeasureText(fin, 60)) / 2, sHeight / 2 - 100, 60, RED);
			DrawText(final, (sWidth - MeasureText(final, 40)) / 2, sHeight / 2, 40, WHITE);
			DrawText(reiniciar, (sWidth - MeasureText(reiniciar, 28)) / 2, sHeight / 2 + 70, 28, LIGHTGRAY);
		}
		EndTextureMode();

		// ---------- Se estira el lienzo a la ventana ----------
		BeginDrawing();
		ClearBackground(BLACK);
		// alto negativo porque las render textures vienen volteadas verticalmente
		Rectangle origen = {0, 0, (float)lienzo.texture.width, -(float)lienzo.texture.height};
		DrawTexturePro(lienzo.texture, origen, destino, (Vector2){0, 0}, 0.0f, WHITE);
		EndDrawing();
	}

	// Desinicialización
	UnloadRenderTexture(lienzo);
	UnloadTexture(fondo);
	UnloadTexture(texNormal);
	UnloadTexture(texDorado);
	UnloadTexture(texBomba);
	CloseWindow();

	return 0;
}