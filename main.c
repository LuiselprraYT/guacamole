#include "raylib.h"

#define MAX_TOPOS 7

int main()
{
	bool exitWindow = 0;
	bool draw = 1;
	const int sHeight = 1080;
	const int sWidth = 607;
	Vector2 coords[MAX_TOPOS] = {
		{sWidth/3, sHeight/4},
		{2*sWidth/3, sHeight/4},
		{sWidth/4, 2*sHeight/4},
		{2*sWidth/4, 2*sHeight/4},
		{3*sWidth/4, 2*sHeight/4},
		{sWidth/3, 3*sHeight/4},
		{2*sWidth/3, 3*sHeight/4}
	};
	bool activeTopo[MAX_TOPOS] = {
		true,
		true,
		true,
		true,
		true,
		true,
		true
	};

	InitWindow(sWidth, sHeight, "Wakamole");

	Image topo = LoadImage("topo.png");
	Texture textura = LoadTextureFromImage(topo);
	textura.height = 100;
	textura.width = 100;
	UnloadImage(topo);

	SetTargetFPS(60);

	// Main loop
	while(!WindowShouldClose())
	{
		if(IsKeyPressed(KEY_ONE))
		{
			if(activeTopo[0]) activeTopo[0] = false;
			else activeTopo[0] = true;
		}
		if(IsKeyPressed(KEY_TWO))
		{
			if(activeTopo[1]) activeTopo[1] = false;
			else activeTopo[1] = true;
		}
		if(IsKeyPressed(KEY_THREE))
		{
			if(activeTopo[2]) activeTopo[2] = false;
			else activeTopo[2] = true;
		}
		if(IsKeyPressed(KEY_FOUR))
		{
			if(activeTopo[3]) activeTopo[3] = false;
			else activeTopo[3] = true;
		}
		if(IsKeyPressed(KEY_FIVE))
		{
			if(activeTopo[4]) activeTopo[4] = false;
			else activeTopo[4] = true;
		}
		if(IsKeyPressed(KEY_SIX))
		{
			if(activeTopo[5]) activeTopo[5] = false;
			else activeTopo[5] = true;
		}
		if(IsKeyPressed(KEY_SEVEN))
		{
			if(activeTopo[6]) activeTopo[6] = false;
			else activeTopo[6] = true;
		}

		// Drawing
		BeginDrawing();

		ClearBackground(BLACK);
		for(int i = 0; i < MAX_TOPOS; i++)
		{
			if(activeTopo[i])
			{
				DrawTexture(textura, coords[i].x - textura.width/2, coords[i].y - textura.height/2, WHITE);
			}
		}

		EndDrawing();
	}
	
	UnloadTexture(textura);
	CloseWindow();

	return 0;
}
