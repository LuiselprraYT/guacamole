// ============================================================================
// 1. LIBRERÍAS Y CONSTANTES DEL JUEGO
// ============================================================================
#include "raylib.h"
#include <math.h>
#include <stddef.h>

#define MAX_TOPOS 7
#define ANCHO_TOPO 160 // Ancho al que se redimensionan los sprites

// --- Tiempos y Velocidades ---
#define TIEMPO_ENTRE_TOPOS 0.9 // Segundos entre cada aparición
#define DURACION_TOPO 2.0f	   // Segundos totales que un topo permanece visible
#define TIEMPO_ANIMACION 0.3f  // Segundos que tarda en subir y bajar
#define DURACION_PARTIDA 25.0f // Duración total de la partida

// --- Ajustes de Texto Flotante ---
#define MAX_POPUPS 10
#define DURACION_POPUP 1.0f // Segundos de visibilidad del puntaje flotante
#define TAM_POPUP 48		// Tamaño de letra del puntaje flotante

// ============================================================================
// 2. ESTRUCTURAS Y ENUMERACIONES (DATOS)
// ============================================================================
typedef enum
{
	ESTADO_INICIO,
	ESTADO_JUGANDO,
	ESTADO_FIN
} Estado;

typedef enum
{
	TOPO_NORMAL,
	TOPO_DORADO,
	TOPO_BOMBA
} TipoTopo;

typedef struct
{
	Vector2 posicion;
	bool activo;
	float tiempoRestante;
	TipoTopo tipo;
	bool golpeado;
} Topo;

typedef struct
{
	Vector2 posicion;
	float tiempo;
	int valor;
	bool activo;
} Popup;

// ============================================================================
// 3. FUNCIONES AUXILIARES Y DE RENDERIZADO
// ============================================================================

// Carga una imagen, la redimensiona manteniendo proporción y la convierte en textura
static Texture CargarTopo(const char *archivo)
{
	Image img = LoadImage(archivo);
	if (img.data == NULL)
	{
		// Si no encuentra la imagen, genera un cuadro magenta de seguridad
		img = GenImageColor(ANCHO_TOPO, ANCHO_TOPO / 2, MAGENTA);
	}
	int alto = img.height * ANCHO_TOPO / img.width;
	ImageResize(&img, ANCHO_TOPO, alto);
	Texture tex = LoadTextureFromImage(img);
	UnloadImage(img);
	return tex;
}

// Suaviza el movimiento de subida y bajada del topo
static float Suavizar(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

// Devuelve la altura relativa del topo (0 = escondido, 1 = afuera)
static float ProgresoTopo(const Topo *h)
{
	float subida = (DURACION_TOPO - h->tiempoRestante) / TIEMPO_ANIMACION;
	float bajada = h->tiempoRestante / TIEMPO_ANIMACION;
	float p = fminf(fminf(subida, bajada), 1.0f);
	if (p < 0.0f)
		p = 0.0f;
	return Suavizar(p);
}

// Genera un texto flotante de puntos en la posición del topo
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

// Dibuja texto con contorno para mejorar la legibilidad sobre el fondo
static void DrawTextoContorno(const char *texto, int x, int y, int tam, Color color, Color contorno, int grosor)
{
	DrawText(texto, x - grosor, y, tam, contorno);
	DrawText(texto, x + grosor, y, tam, contorno);
	DrawText(texto, x, y - grosor, tam, contorno);
	DrawText(texto, x, y + grosor, tam, contorno);
	DrawText(texto, x, y, tam, color);
}

// Dibuja el topo recortando la parte inferior para simular que sale del hoyo
static void DibujarTopo(Texture t, Vector2 centro, float progreso)
{
	int x = (int)(centro.x - t.width / 2);
	int yArriba = (int)(centro.y - t.height / 2);
	int desplazamiento = (int)((1.0f - progreso) * t.height);
	int altoVisible = t.height - desplazamiento;

	if (altoVisible <= 0)
		return;

	Rectangle origen = {0, 0, (float)t.width, (float)altoVisible};
	Vector2 destino = {(float)x, (float)(yArriba + desplazamiento)};
	DrawTextureRec(t, origen, destino, WHITE);
}

// ============================================================================
// 4. FUNCIÓN PRINCIPAL (MAIN)
// ============================================================================
int main()
{
	// ------------------------------------------------------------------------
	// 4.1 INICIALIZACIÓN DE VENTANA Y RENDER TEXTURE
	// ------------------------------------------------------------------------
	const int sWidth = 1280;
	const int sHeight = 720;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(sWidth, sHeight, "Guacamole");
	SetWindowMinSize(640, 360);
	MaximizeWindow();
	SetTargetFPS(60);

	// Ocultar cursor nativo
	HideCursor();

	// Lienzo donde se renderiza a resolución fija de 1280x720
	RenderTexture2D lienzo = LoadRenderTexture(sWidth, sHeight);
	SetTextureFilter(lienzo.texture, TEXTURE_FILTER_BILINEAR);

	// ------------------------------------------------------------------------
	// 4.2 CARGA DE TEXTURAS Y RECURSOS
	// ------------------------------------------------------------------------
	Texture fondo = LoadTexture("fondo.png");
	Texture texNormal = CargarTopo("topo.png");
	Texture texDorado = CargarTopo("topo_dorado.png");
	Texture texBomba = CargarTopo("topo_bomba.png");
	Texture2D texMartillo = LoadTexture("martillo.png");

	// ------------------------------------------------------------------------
	// 4.3 INICIALIZACIÓN DE VARIABLES DEL JUEGO
	// ------------------------------------------------------------------------
	int puntajeActual = 0;
	int mejorPuntaje = 0;
	float tiempoJuego = DURACION_PARTIDA;
	float tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;
	Estado estado = ESTADO_INICIO;
	bool pausado = false;

	Topo hoyos[MAX_TOPOS] = {
		{{sWidth / 3, sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{2 * sWidth / 3, sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{sWidth / 4, 2 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{2 * sWidth / 4, 2 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{3 * sWidth / 4, 2 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{sWidth / 3, 3 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false},
		{{2 * sWidth / 3, 3 * sHeight / 4}, false, 0.0f, TOPO_NORMAL, false}};

	Popup popups[MAX_POPUPS] = {0};

	// ------------------------------------------------------------------------
	// 4.4 BUCLE PRINCIPAL DEL JUEGO
	// ------------------------------------------------------------------------
	while (!WindowShouldClose())
	{
		HideCursor();
		float dt = GetFrameTime();

		// Control de Pantalla Completa
		if (IsKeyPressed(KEY_F11))
			ToggleBorderlessWindowed();

		// Alternar pausa con la tecla P solo durante el juego
		if (IsKeyPressed(KEY_P) && estado == ESTADO_JUGANDO)
			pausado = !pausado;

		// Cálculo de escala para adaptar el lienzo a la ventana
		float escala = fminf((float)GetScreenWidth() / sWidth, (float)GetScreenHeight() / sHeight);
		float anchoDestino = sWidth * escala;
		float altoDestino = sHeight * escala;
		Rectangle destino = {
			(GetScreenWidth() - anchoDestino) / 2.0f,
			(GetScreenHeight() - altoDestino) / 2.0f,
			anchoDestino,
			altoDestino};

		// Posición del mouse en coordenadas lógicas (1280x720)
		Vector2 mousePos = GetMousePosition();
		mousePos.x = (mousePos.x - destino.x) / escala;
		mousePos.y = (mousePos.y - destino.y) / escala;

		// Si el juego está en pausa, no actualizamos la lógica del juego
		if (!pausado)
		{
			// --- ACTUALIZACIÓN DE POPUPS ---
			for (int i = 0; i < MAX_POPUPS; i++)
			{
				if (popups[i].activo)
				{
					popups[i].tiempo -= dt;
					if (popups[i].tiempo <= 0.0f)
						popups[i].activo = false;
				}
			}

			// --- LÓGICA SEGÚN EL ESTADO DEL JUEGO ---
			if (estado == ESTADO_INICIO)
			{
				Rectangle botonStart = {sWidth / 2.0f - 190, 210, 380, 120};
				bool clicEnBoton = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, botonStart);

				if (IsKeyPressed(KEY_SPACE) || clicEnBoton)
				{
					estado = ESTADO_JUGANDO;
					puntajeActual = 0;
					tiempoJuego = DURACION_PARTIDA;
					tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;
					pausado = false;
					for (int i = 0; i < MAX_TOPOS; i++)
					{
						hoyos[i].activo = false;
						hoyos[i].golpeado = false;
					}
					for (int i = 0; i < MAX_POPUPS; i++)
						popups[i].activo = false;
				}
			}
			else if (estado == ESTADO_JUGANDO)
			{
				// Cronómetro general
				tiempoJuego -= dt;
				if (tiempoJuego <= 0.0f)
				{
					tiempoJuego = 0.0f;
					estado = ESTADO_FIN;
					if (puntajeActual > mejorPuntaje)
						mejorPuntaje = puntajeActual;
				}

				// Ocultar topos cuando expire su tiempo
				for (int i = 0; i < MAX_TOPOS; i++)
				{
					if (hoyos[i].activo)
					{
						hoyos[i].tiempoRestante -= dt;
						if (hoyos[i].tiempoRestante <= 0.0f)
							hoyos[i].activo = false;
					}
				}

				// Generar nuevo topo
				tiempoDeAparicion -= dt;
				if (tiempoDeAparicion <= 0.0f)
				{
					tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;

					// Elige cuántos topos va a intentar activar en este ciclo (entre 1 y 3)
					int cantidadASacar = GetRandomValue(1, 3);

					for (int k = 0; k < cantidadASacar; k++)
					{
						int indice = GetRandomValue(0, MAX_TOPOS - 1);
						if (!hoyos[indice].activo)
						{
							hoyos[indice].activo = true;
							hoyos[indice].golpeado = false;
							hoyos[indice].tiempoRestante = DURACION_TOPO;

							int probabilidad = GetRandomValue(1, 100);
							if (probabilidad <= 60)
								hoyos[indice].tipo = TOPO_NORMAL;
							else if (probabilidad <= 85)
								hoyos[indice].tipo = TOPO_DORADO;
							else
								hoyos[indice].tipo = TOPO_BOMBA;
						}
					}
				}

				// Detección de clics en topos
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					for (int i = 0; i < MAX_TOPOS; i++)
					{
						float p = ProgresoTopo(&hoyos[i]);
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
							hoyos[i].golpeado = true;
							if (hoyos[i].tiempoRestante > TIEMPO_ANIMACION)
								hoyos[i].tiempoRestante = TIEMPO_ANIMACION;

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

							Vector2 cabeza = {
								hoyos[i].posicion.x,
								hoyos[i].posicion.y - t.height / 2.0f + (1.0f - p) * t.height - 10.0f};
							CrearPopup(popups, cabeza, puntajeActual - antes);
							break;
						}
					}
				}
			}
			else if (estado == ESTADO_FIN)
			{
				if (IsKeyPressed(KEY_SPACE))
				{
					estado = ESTADO_JUGANDO;
					puntajeActual = 0;
					tiempoJuego = DURACION_PARTIDA;
					tiempoDeAparicion = TIEMPO_ENTRE_TOPOS;
					pausado = false;
					for (int i = 0; i < MAX_TOPOS; i++)
					{
						hoyos[i].activo = false;
						hoyos[i].golpeado = false;
					}
					for (int i = 0; i < MAX_POPUPS; i++)
						popups[i].activo = false;
				}
			}
		}

		// ------------------------------------------------------------------------
		// 4.5 DIBUJADO EN EL LIENZO (1280x720)
		// ------------------------------------------------------------------------
		BeginTextureMode(lienzo);
		ClearBackground(DARKGREEN);
		if (fondo.id != 0)
			DrawTexture(fondo, 0, 0, WHITE);

		// --- DIBUJAR HUD ---
		if (estado != ESTADO_INICIO)
		{
			const int tamHUD = 34;
			const int yHUD = (68 - tamHUD) / 2;

			const char *textoPuntaje = TextFormat("Puntaje: %d", puntajeActual);
			DrawText(textoPuntaje, (sWidth - MeasureText(textoPuntaje, tamHUD)) / 2, yHUD, tamHUD, MAGENTA);

			const char *textoTiempo = TextFormat("Tiempo: %d", (int)ceilf(tiempoJuego));
			DrawText(textoTiempo, 24, yHUD, tamHUD, (tiempoJuego <= 10.0f) ? RED : WHITE);

			const char *textoMejor = TextFormat("Mejor: %d", mejorPuntaje);
			DrawText(textoMejor, sWidth - MeasureText(textoMejor, tamHUD) - 24, yHUD, tamHUD, YELLOW);
		}

		// --- DIBUJAR TOPOS ---
		for (int i = 0; i < MAX_TOPOS; i++)
		{
			if (!hoyos[i].activo)
				continue;

			Texture t = (hoyos[i].tipo == TOPO_DORADO)	? texDorado
						: (hoyos[i].tipo == TOPO_BOMBA) ? texBomba
														: texNormal;
			DibujarTopo(t, hoyos[i].posicion, ProgresoTopo(&hoyos[i]));
		}

		// --- DIBUJAR POPUPS DE PUNTAJE ---
		for (int i = 0; i < MAX_POPUPS; i++)
		{
			if (!popups[i].activo)
				continue;

			float avance = 1.0f - popups[i].tiempo / DURACION_POPUP;
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

		// --- DIBUJAR PANTALLA DE INICIO ---
		if (estado == ESTADO_INICIO)
		{
			DrawRectangle(0, 0, sWidth, sHeight, Fade(BLACK, 0.55f));

			const char *titulo = "GUACAMOLE";
			DrawTextoContorno(titulo, (sWidth - MeasureText(titulo, 96)) / 2, 70, 96, GOLD, BLACK, 4);

			Rectangle boton = {sWidth / 2.0f - 190, 210, 380, 120};
			DrawRectangleRounded((Rectangle){boton.x - 6, boton.y - 6, boton.width + 12, boton.height + 12}, 0.35f, 10, WHITE);
			DrawRectangleRounded(boton, 0.35f, 10, (Color){30, 140, 50, 255});
			const char *start = "START";
			DrawText(start, (sWidth - MeasureText(start, 72)) / 2, (int)boton.y + 24, 72, WHITE);

			float pulso = 0.55f + 0.45f * sinf((float)GetTime() * 4.0f);
			const char *aviso = "Presiona ESPACIO o START para comenzar";
			DrawText(aviso, (sWidth - MeasureText(aviso, 32)) / 2, 360, 32, Fade(WHITE, pulso));

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

		// --- DIBUJAR PANTALLA DE FIN ---
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

		// --- DIBUJAR OVERLAY DE PAUSA EN EL LIENZO ---
		if (pausado)
		{
			DrawRectangle(0, 0, sWidth, sHeight, Fade(BLACK, 0.6f));
			const char *txtPausa = "JUEGO EN PAUSA (Presiona P)";
			DrawTextoContorno(txtPausa, (sWidth - MeasureText(txtPausa, 40)) / 2, sHeight / 2 - 20, 40, WHITE, BLACK, 3);
		}

		EndTextureMode(); // Fin del dibujo en el lienzo

		// ------------------------------------------------------------------------
		// 4.6 DIBUJADO FINAL EN PANTALLA
		// ------------------------------------------------------------------------
		BeginDrawing();
		ClearBackground(BLACK);

		// Dibuja el lienzo escalado
		Rectangle origenLienzo = {0, 0, (float)lienzo.texture.width, -(float)lienzo.texture.height};
		DrawTexturePro(lienzo.texture, origenLienzo, destino, (Vector2){0, 0}, 0.0f, WHITE);

		// Cursor: Martillo
		Vector2 realMouse = GetMousePosition();
		if (texMartillo.id != 0 && texMartillo.width > 0)
		{
			float anchoMartillo = 90.0f;
			float altoMartillo = anchoMartillo * ((float)texMartillo.height / texMartillo.width);
			float angulo = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? -35.0f : 0.0f;

			Rectangle origM = {0, 0, (float)texMartillo.width, (float)texMartillo.height};
			Rectangle destM = {realMouse.x, realMouse.y, anchoMartillo, altoMartillo};
			Vector2 pivote = {anchoMartillo * 0.5f, altoMartillo * 0.5f};

			DrawTexturePro(texMartillo, origM, destM, pivote, angulo, WHITE);
		}

		EndDrawing();
	}

	// ------------------------------------------------------------------------
	// 4.7 LIBERACIÓN DE MEMORIA Y SALIDA
	// ------------------------------------------------------------------------
	UnloadTexture(texMartillo);
	UnloadRenderTexture(lienzo);
	UnloadTexture(fondo);
	UnloadTexture(texNormal);
	UnloadTexture(texDorado);
	UnloadTexture(texBomba);
	CloseWindow();

	return 0;
}