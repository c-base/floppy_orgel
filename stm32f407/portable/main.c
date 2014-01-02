#include "stm32f4xx.h"
#include "delay.h"
#include "SSD1289.h"
#include "touch_7846.h"
#include "stm32_ub_rng.h"


// Starfield
#define NUM_STARS 512
#define MAX_DEPTH 32
#define SCREEN_RES_X 240
#define SCREEN_RES_Y 320

struct{
	float x;
	float y;
	float z;
} stars[NUM_STARS];

int32_t randrange(int32_t min, int32_t max) {
	return UB_Rng_makeZufall(min, max);
}

void init_stars() {
	// create the starfield
	for(int i = 0; i < NUM_STARS; i++) {
		// A star is represented as a list with this format: [X,Y,Z]
		stars[i].x = randrange(-25, 25);
		stars[i].y = randrange(-25, 25);
		stars[i].z = randrange(1, MAX_DEPTH);
	}
}

void move_and_draw_stars() {
	// Move and draw the stars
	int origin_x = SCREEN_RES_X / 2;
	int origin_y = SCREEN_RES_Y / 2;

	for(int i = 0; i < NUM_STARS; i++) {
		// The Z component is decreased on each frame.
		stars[i].z -= 0.19f;

		// If the star has past the screen (I mean Z<=0) then we
		// reposition it far away from the screen (Z=max_depth)
		// with random X and Y coordinates.
		if(stars[i].z <= 0) {
			stars[i].x = randrange(-25, 25);
			stars[i].y = randrange(-25, 25);
			stars[i].z = MAX_DEPTH;
		}

		// Convert the 3D coordinates to 2D using perspective projection.
		float k = 128.0f / stars[i].z;
		int x = stars[i].x * k + origin_x;
		int y = stars[i].y * k + origin_y;

		// Draw the star (if it is visible in the screen).
		// We calculate the size such that distant stars are smaller than
		// closer stars. Similarly, we make sure that distant stars are
		// darker than closer stars. This is done using Linear Interpolation.

		// 0 <= x < SCREEN_RES_X && 0 <= y < SCREEN_RES_Y
		if( x >= 0 && x < SCREEN_RES_X && y >= 0 && y < SCREEN_RES_Y) {
			int size  = (1 - (float)stars[i].z / MAX_DEPTH) * 5;
			int shade = (1 - (float)stars[i].z / MAX_DEPTH) * 255;

			LCD_SetTextColor(ASSEMBLE_RGB(shade, shade, shade));
			LCD_DrawFullSquare(x, y, size);
		}
	}
}


int main(void)
{
  Delay(0x3FFFFF);
  LCD_Init();
  Delay(0x3FFFFF);
  touch_init();
  LCD_Clear(BLACK);
  Delay(0x0FFFFF);

  UB_Rng_Init(); // Init the random number generator
  init_stars();
  while(1) {
	  // Main Loop
	  LCD_Clear(BLACK);
	  move_and_draw_stars();
	  Delay(0x00FFFF);

	  // TODO: backbuffered screen update here, to prevent flickering!
  }
}

