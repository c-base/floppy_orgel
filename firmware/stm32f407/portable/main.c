#include "stm32f4xx.h"
#include "delay.h"
#include "SSD1289.h"
#include "stm32_ub_rng.h"
#include "tm_stm32f4_fatfs.h"
#include "midifile.h"
#include "NesGamePad.h"


typedef struct {
  _MIDI_FILE* pMidiFile;
  MIDI_MSG msg[MAX_MIDI_TRACKS];
  int32_t startTime;
  int32_t currentTick;
  int32_t lastTick;
  int32_t deltaTick; // Must NEVER be negative!!!
  BOOL eventsNeedToBeFetched;
  BOOL trackIsFinished;
  BOOL allTracksAreFinished;
  float lastMsPerTick;
  float timeScaleFactor;
} MIDI_PLAYER;

static MIDI_PLAYER g_midiPlayer;

void enableMidiTimer() {
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

  TIM_TimeBaseInitTypeDef timerInitStructure;
  timerInitStructure.TIM_Prescaler = 84 - 1; // 1 Mhz
  timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  timerInitStructure.TIM_Period = -1; // no period, use maximum of uint32_t
  timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  timerInitStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM5, &timerInitStructure);
  TIM_Cmd(TIM5, ENABLE);
}

void enableDelayTimer() {
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseInitTypeDef timerInitStructure;
  timerInitStructure.TIM_Prescaler = 84 - 1; // 1 Mhz
  timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  timerInitStructure.TIM_Period = -1; // no period, use maximum of uint32_t
  timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  timerInitStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM2, &timerInitStructure);
  TIM_Cmd(TIM2, ENABLE);
}

// UART
void initializeDebugUart(uint32_t baudrate) {
	/* This is a concept that has to do with the libraries provided by ST
	 * to make development easier the have made up something similar to
	 * classes, called TypeDefs, which actually just define the common
	 * parameters that every peripheral needs to work correctly
	 *
	 * They make our life easier because we don't have to mess around with
	 * the low level stuff of setting bits in the correct registers
	 */
	GPIO_InitTypeDef GPIO_InitStruct; // this is for the GPIO pins used as TX and RX
	USART_InitTypeDef USART_InitStruct; // this is for the USART1 initilization

	/* enable APB2 peripheral clock for USART1
	 * note that only USART1 and USART6 are connected to APB2
	 * the other USARTs are connected to APB1
	 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* enable the peripheral clock for the pins used by
	 * USART1, PB6 for TX and PB7 for RX
	 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* This sequence sets up the TX and RX pins
	 * so they work correctly with the USART1 peripheral
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // Pins 6 (TX) and 7 (RX) are used
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF; 			// the pins are configured as alternate function so the USART peripheral has access to them
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;		// this defines the IO speed and has nothing to do with the baudrate!
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;			// this defines the output type as push pull mode (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			// this activates the pullup resistors on the IO pins
	GPIO_Init(GPIOB, &GPIO_InitStruct);					// now all the values are passed to the GPIO_Init() function which sets the GPIO registers

	/* The RX and TX pins are now connected to their AF
	 * so that the USART1 can take over control of the
	 * pins
	 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1); //
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

	/* Now the USART_InitStruct is used to define the
	 * properties of USART1
	 */
	USART_InitStruct.USART_BaudRate = baudrate;				// the baudrate is set to the value we passed into this init function
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	USART_InitStruct.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	USART_InitStruct.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART1, &USART_InitStruct);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting


	/* Here the USART1 receive interrupt is enabled
	 * and the interrupt controller is configured
	 * to jump to the USART1_IRQHandler() function
	 * if the USART1 receive interrupt occurs
	 */

	/*
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable the USART1 receive interrupt

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff
	*/

	// finally this enables the complete USART1 peripheral
	USART_Cmd(USART1, ENABLE);
}

void initializeBusUart(uint32_t baudrate) {
  GPIO_InitTypeDef GPIO_InitStruct; // this is for the GPIO pins used as TX and RX
  USART_InitTypeDef USART_InitStruct; // this is for the USART1 initilization

  /* enable APB2 peripheral clock for USART1
   * note that only USART1 and USART6 are connected to APB2
   * the other USARTs are connected to APB1
   */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

  /* enable the peripheral clock for the pins used by
   * USART6, PC6 for TX and PC7 for RX
   */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  /* This sequence sets up the TX and RX pins
   * so they work correctly with the USART1 peripheral
   */
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // Pins 6 (TX) and 7 (RX) are used
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;       // the pins are configured as alternate function so the USART peripheral has access to them
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;   // this defines the IO speed and has nothing to do with the baudrate!
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;     // this defines the output type as push pull mode (as opposed to open drain)
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;     // this activates the pullup resistors on the IO pins
  GPIO_Init(GPIOC, &GPIO_InitStruct);         // now all the values are passed to the GPIO_Init() function which sets the GPIO registers

  /* The RX and TX pins are now connected to their AF
   * so that the USART6 can take over control of the
   * pins
   */
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6); //
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

  /* Now the USAR6_InitStruct is used to define the
   * properties of USART1
   */
  USART_InitStruct.USART_BaudRate = baudrate;       // the baudrate is set to the value we passed into this init function
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
  USART_InitStruct.USART_StopBits = USART_StopBits_1;   // we want 1 stop bit (standard)
  USART_InitStruct.USART_Parity = USART_Parity_No;    // we don't want a parity bit (standard)
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
  USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
  USART_Init(USART6, &USART_InitStruct);          // again all the properties are passed to the USART_Init function which takes care of all the bit setting

  // finally this enables the complete USART6 peripheral
  USART_Cmd(USART6, ENABLE);
}

unsigned int _USART_getc(USART_TypeDef* USARTx, char* c) {
  if (USARTx->SR & USART_SR_RXNE) {
    // read RX data, combine with DR mask (we only accept a max of 9 Bits)
    *c = USARTx->DR & 0x1FF;
    return 1;
  }
  return 0;
}

// This will print on usart 1 (the original function has been commented out in printf.c !!!
int fputc(signed int c) {
  // Wait until transmit finishes
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

  // Transmit the character using USART1
  USART_SendData(USART1, (u8) c);
  return c;
}


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

/* TODO: port to new library version!
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
*/

BOOL midiPlayerOpenFile(MIDI_PLAYER* pMidiPlayer, const char* pFileName) {
  pMidiPlayer->pMidiFile = midiFileOpen(pFileName);
  if (!pMidiPlayer->pMidiFile)
    return FALSE;

  // Load initial midi events
  for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMidiPlayer->pMidiFile); iTrack++) {
    midiReadGetNextMessage(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]);
    pMidiPlayer->pMidiFile->Track[iTrack].deltaTime = pMidiPlayer->msg[iTrack].dt;
  }

  pMidiPlayer->startTime = hal_clock();
  pMidiPlayer->currentTick = 0;
  pMidiPlayer->lastTick = 0;
  pMidiPlayer->deltaTick; // Must NEVER be negative!!!
  pMidiPlayer->eventsNeedToBeFetched = FALSE;
  pMidiPlayer->trackIsFinished;
  pMidiPlayer->allTracksAreFinished = FALSE;
  pMidiPlayer->lastMsPerTick = pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->timeScaleFactor = 1.0f;

  return TRUE;
}

BOOL midiPlayerTick(MIDI_PLAYER* pMidiPlayer) {
  if(pMidiPlayer->pMidiFile == NULL)
    return FALSE;

  if (fabs(pMidiPlayer->lastMsPerTick - pMidiPlayer->pMidiFile->msPerTick) > 0.001f) { // TODO: avoid floating point operation here!
    // On a tempo change we need to transform the old absolute time scale to the new scale.
    pMidiPlayer->timeScaleFactor = pMidiPlayer->lastMsPerTick / pMidiPlayer->pMidiFile->msPerTick;
    pMidiPlayer->lastTick *= pMidiPlayer->timeScaleFactor;
  }

  pMidiPlayer->lastMsPerTick = pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->currentTick = (hal_clock() - pMidiPlayer->startTime) / pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->eventsNeedToBeFetched = TRUE;

  int32_t start = TIM5->CNT; // DEBUG
  while (pMidiPlayer->eventsNeedToBeFetched) { // This loop keeps all tracks synchronized in case of a lag
    pMidiPlayer->eventsNeedToBeFetched = FALSE;
    pMidiPlayer->allTracksAreFinished = TRUE;
    pMidiPlayer->deltaTick = pMidiPlayer->currentTick - pMidiPlayer->lastTick;
    if (pMidiPlayer->deltaTick < 0) {
      printf("Warning: deltaTick is negative! Fast forward? deltaTick=%d", pMidiPlayer->deltaTick);
      // TODO: correct time here!
      pMidiPlayer->deltaTick = 0;
    }

    for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMidiPlayer->pMidiFile); iTrack++) {
      pMidiPlayer->pMidiFile->Track[iTrack].deltaTime -= pMidiPlayer->deltaTick;
      pMidiPlayer->trackIsFinished = pMidiPlayer->pMidiFile->Track[iTrack].ptrNew == pMidiPlayer->pMidiFile->Track[iTrack].pEndNew;

      if (!pMidiPlayer->trackIsFinished) {
        if (pMidiPlayer->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMidiPlayer->trackIsFinished) { // Is it time to play this event?
          dispatchMidiMsg(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]); // shoot
          midiReadGetNextMessage(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]); // reload
          pMidiPlayer->pMidiFile->Track[iTrack].deltaTime += pMidiPlayer->msg[iTrack].dt;
        }

        if (pMidiPlayer->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMidiPlayer->trackIsFinished)
          pMidiPlayer->eventsNeedToBeFetched = TRUE;

        pMidiPlayer->allTracksAreFinished = FALSE;
      }
      pMidiPlayer->lastTick = pMidiPlayer->currentTick;
    }
  }

  uint32_t dispatchTime = (TIM5->CNT - start) / 1000;
  if(dispatchTime > 0)
    printf("Dispatch took: %d ms\n\r", dispatchTime);

  return !pMidiPlayer->allTracksAreFinished; // TODO: close file
}

void drawCursor(uint32_t cursorPos) {
  const uint32_t X_OFFSET = 25;
  const uint32_t Y_OFFSET = 45;

  SSD1289_DrawRect(X_OFFSET, Y_OFFSET + 18 * cursorPos, 5, 5, White);
  const uint32_t LINE_OFFSET = 18;

  uint32_t x[3] = { X_OFFSET,      // links oben
                    X_OFFSET + 50, // rechts mitte
                    X_OFFSET};     // links unten

  uint32_t y[3] = { Y_OFFSET + 18 * cursorPos,       // links oben
                    Y_OFFSET + 18 * cursorPos + 25,  // rechts mitte
                    Y_OFFSET + 18 * cursorPos + 50}; // links unten
}

FRESULT drawTracks(char* path) {
  DIR dirs;
  FRESULT res;
  BYTE i;
  char *fn;
  DWORD acc_size;       /* Work register for fs command */
  WORD acc_files, acc_dirs;
  FILINFO finfo;

  static TCHAR lfname[_MAX_LFN];
  finfo.lfname = lfname;
  finfo.lfsize = 250;

  static const uint32_t X_OFFSET = 35;
  static const uint32_t Y_OFFSET = 40;

  if (res = f_opendir(&dirs, path) == FR_OK) {
    i = strlen(path);

    int32_t itemCount = 0;
    while (((res = f_readdir(&dirs, &finfo)) == FR_OK) && finfo.fname[0]) {
      #if _USE_LFN
        fn = *finfo.fname ? *finfo.lfname != 0 ? finfo.lfname : finfo.fname : finfo.fname;
      #else
        fn = finfo.fname;
      #endif
      if (finfo.fattrib & AM_DIR) {
        acc_dirs++;
        *(path + i) = '/'; strcpy(path + i + 1, fn);
        SSD1289_Text(X_OFFSET, Y_OFFSET + 18 * itemCount++, path, White, Black);

        res = drawTracks(path);
        *(path+i) = '\0';

        if (res != FR_OK) break;
      } else {
        SSD1289_Text(X_OFFSET, Y_OFFSET + 18 * itemCount++, fn, White, Black);

        acc_files++;
        acc_size += finfo.fsize;
      }
    }
  }
  else
    printf("Failed opening directory: '%s'.\r\n", path);

  for(int i = strlen(path); i > 0; i--) {
    if(path[i] == '/')
      path[i + 1] = '\0';
  }

  return res;
}

void drawMenu() {
  static uint8_t lastStateCode = 0;
  static union NesGamePadStates_t gamePad;
  static BOOL firstStart = TRUE;

  static const uint32_t X_OFFSET = 65;
  static const uint32_t Y_OFFSET = 240 - 18;
  static uint32_t cursorPos = 0;
  static uint32_t maxCursorPos = 0;
  static BOOL isPlaying = FALSE;

  if(isPlaying)
    if(!midiPlayerTick(&g_midiPlayer)) {
      isPlaying = FALSE;
      firstStart = TRUE;
    }


  isPlaying = midiPlayerTick(&g_midiPlayer);

  gamePad = getNesGamepadState();

  if((lastStateCode != gamePad.code && gamePad.code != 0x00) || firstStart) {
    SSD1289_Clear(Black);
    SSD1289_Text(X_OFFSET - 30, 0, "Use the game pad to select a song", White, Black);
    SSD1289_Text(X_OFFSET + 10, 18, "Press A button to start", White, Black);

    if(gamePad.code == 0xFF) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "Please plug in game pad", White, Black);
    }
    else if(gamePad.code == 0x00) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "No button is pressed", White, Black);
    }
    else if(gamePad.states.A) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing A", White, Black);
      onMenuAction(cursorPos);
    }
    else if(gamePad.states.B) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing B", White, Black);
      onMenuBack();
    }
    else if(gamePad.states.Start) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing Start", White, Black);
      onMenuAction(cursorPos);
    }
    else if(gamePad.states.Select) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing Select", White, Black);
      onMenuBack();
    }
    else if(gamePad.states.North) {
      cursorPos--;
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing North (UP)", White, Black);
    }
    else if(gamePad.states.South) {
      cursorPos++;
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing South (DOWN)", White, Black);
    }
    else if(gamePad.states.West) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing West (LEFT)", White, Black);
    }
    else if(gamePad.states.East) {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing West (RIGHT)", White, Black);
    }
    else {
      //SSD1289_Text(X_OFFSET, Y_OFFSET, "You are pressing something else...", White, Black);
    }

    // Draw midi files here
    static char trackName[256] = "";
    drawTracks(trackName);
    drawCursor(cursorPos);
  }



  lastStateCode = gamePad.code;
  firstStart = FALSE;
  //move_and_draw_stars();
}

void debugPrintln(char* text) {
  printf("%s\r\n", text);
}


// Menu
// TODO: make much better! Prevent flickering etc!!!
void onMenuBack() {
  SSD1289_Clear(Red);
}

void onMenuAction(uint32_t menuItem) {
  DIR dirs;
  FRESULT res;
  FILINFO finfo;
  static TCHAR lfname[_MAX_LFN];
  finfo.lfname = lfname;
  finfo.lfsize = 250;
  static const uint32_t X_OFFSET = 65;

  res = f_opendir(&dirs, "/");
  if (res == FR_OK) {
    int32_t currentItem = 0;
    char* fn;

    while (((res = f_readdir(&dirs, &finfo)) == FR_OK) && finfo.fname[0]) {
      #if _USE_LFN
        fn = *finfo.fname ? *finfo.lfname != 0 ? finfo.lfname : finfo.fname : finfo.fname;
      #else
        fn = finfo.fname;
      #endif
      if (finfo.fattrib & AM_DIR)
        continue;
      else {
        if(currentItem++ == menuItem) {
          midiPlayerOpenFile(&g_midiPlayer, fn);
          SSD1289_Clear(Black);
          static char infoText[256];
          sprintf(infoText, "Now playing: %s", fn);
          SSD1289_Text(X_OFFSET - 30, 0, infoText, White, Black);
          SSD1289_Text(X_OFFSET + 10, 18, "Press B or Select button to stop", White, Black);
          return;
        }
      }
    }
  }
}

FRESULT scan_files(char* path) {
  DIR dirs;
  FRESULT res;
  BYTE i;
  char *fn;
  DWORD acc_size;       /* Work register for fs command */
  WORD acc_files, acc_dirs;
  FILINFO finfo;

  static TCHAR lfname[_MAX_LFN];
  finfo.lfname = lfname;
  finfo.lfsize = 250;

  printf("Opening directory: '%s'\r\n", path);
  if (res = f_opendir(&dirs, path) == FR_OK) {
    i = strlen(path);
    while (((res = f_readdir(&dirs, &finfo)) == FR_OK) && finfo.fname[0]) {
      #if _USE_LFN
        fn = *finfo.fname ? *finfo.lfname != 0 ? finfo.lfname : finfo.fname : finfo.fname;
      #else
        fn = finfo.fname;
      #endif
      if (finfo.fattrib & AM_DIR) {
        acc_dirs++;
        *(path+i) = '/'; strcpy(path+i+1, fn);
        printf("%s\r\n", path);

        res = scan_files(path);
        *(path+i) = '\0';

        if (res != FR_OK) break;
      } else {

        printf("%s/%s - %d KB\r\n", path, fn, (10 * finfo.fsize / 1024) / 10);

        acc_files++;
        acc_size += finfo.fsize;
      }
    }
  }
  else
    printf("Failed opening directory: '%s'.\r\n", path);

  printf("- End of Directory.\r\n");
  for(int i = strlen(path); i > 0; i--) {
    if(path[i] == '/')
      path[i + 1] = '\0';
  }

  return res;
}


uint16_t midiNote2Ticks[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    64282,
    60675,
    57269,
    54055,
    51021,
    48157,
    45455,
    42903,
    40495,
    38223,
    36077,
    34052,
    32141,
    30337,
    28635,
    27027,
    25511,
    24079,
    22727,
    21452,
    20248,
    19111,
    18039,
    17026,
    16071,
    15169,
    14317,
    13514,
    12755,
    12039,
    11364,
    10726,
    10124,
    9556,
    9019,
    8513,
    8035,
    7584,
    7159,
    6757,
    6378,
    6020,
    5682,
    5363,
    5062,
    4778,
    4510,
    4257,
    4018,
    3792,
    3579,
    3378,
    3189,
    3010,
    2841,
    2681,
    2531,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

void playNote(int midi_channel, int midi_note) {
  uint16_t ticks = midiNote2Ticks[midi_note];
  unsigned char data[5] = {0x55, 0xAA, midi_channel, ((char*)&ticks)[1], ((char*)&ticks)[0]};

  for(int i = 0; i < sizeof(data); i++) {
    while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET); // Wait until transmit finishes
      USART_SendData(USART6, data[i]);
  }
}

/*
def play_note(self, midi_channel, midi_note):
        if midi_channel < 1 or midi_channel > self.MAX_CHANNELS:
            raise Exception("channel '%d' out of range. it has to be between 1 - %d" % (midi_channel, self.MAX_CHANNELS) )

        half_period = self.midiHalfperiods[midi_note]
        midi_channel = (self.midi_channels[midi_channel - 1].floppy_channel - 1)

        prescaler = 8
        crystal_clock = 20000000 # Mhz
        frequency = (2**(1.0/12))**(midi_note - 69) * 440

        if(frequency > 440):
          return

        ticks = round(crystal_clock / (2.0 * prescaler * frequency))

        try:
            data = struct.pack('B', 0x55) + struct.pack('B', 0xAA) + struct.pack('B', midi_channel) + struct.pack('>H', int(ticks))
            self._used_serial_ports[self.midi_channels[midi_channel - 1].serial_port].write(data)
            #print "Playing note: %d on channel: %d" % (frequency, midi_channel)
        except:
            pass #print "serial port error"
*/





// TODO: layout to misc library
typedef struct {
  uint32_t
  TIM2EN   : 1,
  TIM3EN   : 1,
  TIM4EN   : 1,
  TIM5EN   : 1,
  TIM6EN   : 1,
  TIM7EN   : 1,
  TIM12EN  : 1,
  TIM13EN  : 1,
  TIM14EN  : 1,
           : 2,
  WWDGEN   : 1,
           : 2,
  SPI2EN   : 1,
  SPI3EN   : 1,
           : 1,
  USART2EN : 1,
  USART3EN : 1,
  UART4EN  : 1,
  UART5EN  : 1,
  I2C1EN   : 1,
  I2C2EN   : 1,
  I2C3EN   : 1,
           : 1,
  CAN1EN   : 1,
  CAN2EN   : 1,
  PWREN    : 1,
  DACEN    : 1,
           : 2;
} RegSTM32F4_RCC_APB1ENR_t;

void getFileNameFromUart(char* fileName) {
  int fileNameComplete = 0;
  int i = 0;
  int c = '\0';

  while(!fileNameComplete) {
    if(_USART_getc(USART1, &c)) {
      while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // Wait until transmit finishes
        USART_SendData(USART1, c);

      // check for backspace
      if(c == 0x08)
        i = i > 0 ? i - 1: i;

      // check for end of line
      if(c == '\r' || c == '\n') {
        fileName[i] = '\0'; // nulltermination
        fileNameComplete++;
      }
      else {
        fileName[i] = c;
        i++;
      }
    }
  }
}


///////////

void HexList(uint8_t *pData, int32_t iNumBytes) {
  for (int32_t i = 0; i < iNumBytes; i++)
    printf("%.2x ", pData[i]);
}

void printTrackPrefix(uint32_t track, uint32_t tick, char* pEventName)  {
  printf("[Track: %d] %06d %s ", track, tick, pEventName);
}

// Midi Event handlers
char noteName[64]; // TOOD: refactor to const string array

void onNoteOff(int32_t track, int32_t tick, int32_t channel, int32_t note) {
  return;

  muGetNameFromNote(noteName, note);
  playNote(channel, 0);

  printTrackPrefix(track, tick, "Note Off");
  printf("(%d) %s", channel, noteName);
  printf("\n\r");
}

void onNoteOn(int32_t track, int32_t tick, int32_t channel, int32_t note, int32_t velocity) {
  return;

  muGetNameFromNote(noteName, note);
  if(velocity > 0)
    playNote(channel, note);
  else
    playNote(channel, 0);

  printTrackPrefix(track, tick, "Note On");
  printf("(%d) %s [%d] %d", channel, noteName, note, velocity);
  printf("\n\r");
}

void onNoteKeyPressure(int32_t track, int32_t tick, int32_t channel, int32_t note, int32_t pressure) {
  return;

  muGetNameFromNote(noteName, note);
  printTrackPrefix(track, tick, "Note Key Pressure");
  printf("(%d) %s %d", channel, noteName, pressure);
  printf("\n\r");
}

void onSetParameter(int32_t track, int32_t tick, int32_t channel, int32_t control, int32_t parameter) {
  return;

  muGetControlName(noteName, control);
  printTrackPrefix(track, tick, "Set Parameter");
  printf("(%d) %s -> %d", channel, noteName, parameter);
  printf("\n\r");
}

void onSetProgram(int32_t track, int32_t tick, int32_t channel, int32_t program) {
  return;

  muGetInstrumentName(noteName, program);
  printTrackPrefix(track, tick, "Set Program");
  printf("(%d) %s", channel, noteName);
  printf("\n\r");
}

void onChangePressure(int32_t track, int32_t tick, int32_t channel, int32_t pressure) {
  return;

  muGetControlName(noteName, pressure);
  printTrackPrefix(track, tick, "Change Pressure");
  printf("(%d) %s", channel, noteName);
  printf("\n\r");
}

void onSetPitchWheel(int32_t track, int32_t tick, int32_t channel, int16_t pitch) {
  return;

  printTrackPrefix(track, tick, "Set Pitch Wheel");
  printf("(%d) %d", channel, pitch);
  printf("\n\r");
}

void onMetaMIDIPort(int32_t track, int32_t tick, int32_t midiPort) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("MIDI Port = %d", midiPort);
  printf("\n\r");
}

void onMetaSequenceNumber(int32_t track, int32_t tick, int32_t sequenceNumber) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Sequence Number = %d", sequenceNumber);
  printf("\n\r");
}

void onMetaTextEvent(int32_t track, int32_t tick, char* pText) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Text = '%s'", pText);
  printf("\n\r");
}

void onMetaCopyright(int32_t track, int32_t tick, char* pText) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Copyright = '%s'", pText);
  printf("\n\r");
}

void onMetaTrackName(int32_t track, int32_t tick, char *pText) {
  printTrackPrefix(track, tick, "Meta event ----");
  printf("Track name = '%s'", pText);
  printf("\n\r");
}

void onMetaInstrument(int32_t track, int32_t tick, char *pText) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Instrument = '%s'", pText);
  printf("\n\r");
}

void onMetaLyric(int32_t track, int32_t tick, char *pText) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Lyric = '%s'", pText);
  printf("\n\r");
}

void onMetaMarker(int32_t track, int32_t tick, char *pText) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Marker = '%s'", pText);
  printf("\n\r");
}

void onMetaCuePoint(int32_t track, int32_t tick, char *pText) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Cue point = '%s'", pText);
  printf("\n\r");
}

void onMetaEndSequence(int32_t track, int32_t tick) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("End Sequence");
  printf("\n\r");
}

void onMetaSetTempo(int32_t track, int32_t tick, int32_t bpm) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Tempo = %d", bpm);
  printf("\n\r");
}

void onMetaSMPTEOffset(int32_t track, int32_t tick, uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t frames, uint32_t subframes) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("SMPTE offset = %d:%d:%d.%d %d", hours, minutes, seconds, frames, subframes);
  printf("\n\r");
}

void onMetaTimeSig(int32_t track, int32_t tick, int32_t nom, int32_t denom, int32_t metronome, int32_t thirtyseconds) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Time sig = %d/%d", nom, denom);
  printf("\n\r");
}

void onMetaKeySig(int32_t track, int32_t tick, uint32_t key, uint32_t scale) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  if (muGetKeySigName(noteName, key)) {
    printf("Key sig = %s", noteName);
    printf("\n\r");
  }
}

void onMetaSequencerSpecific(int32_t track, int32_t tick, void* pData, uint32_t size) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("Sequencer specific = ");
  HexList(pData, size);
  printf("\n\r");
}

void onMetaSysEx(int32_t track, int32_t tick, void* pData, uint32_t size) {
  return;

  printTrackPrefix(track, tick, "Meta event ----");
  printf("SysEx = ");
  HexList(pData, size);
  printf("\n\r");
}

// TODO: Hide the following functions from user
void dispatchMidiMsg(_MIDI_FILE* midiFile, int32_t track, MIDI_MSG* msg) {
  int32_t eventType = msg->bImpliedMsg ? msg->iImpliedMsg : msg->iType;
  switch (eventType) {
    case  msgNoteOff:
      onNoteOff(track, msg->dwAbsPos, msg->MsgData.NoteOff.iChannel, msg->MsgData.NoteOff.iNote);
      break;
    case  msgNoteOn:
      onNoteOn(track, msg->dwAbsPos, msg->MsgData.NoteOn.iChannel, msg->MsgData.NoteOn.iNote, msg->MsgData.NoteOn.iVolume);
      break;
    case  msgNoteKeyPressure:
      onNoteKeyPressure(track, msg->dwAbsPos, msg->MsgData.NoteKeyPressure.iChannel, msg->MsgData.NoteKeyPressure.iNote, msg->MsgData.NoteKeyPressure.iPressure);
      break;
    case  msgSetParameter:
      onSetParameter(track, msg->dwAbsPos, msg->MsgData.NoteParameter.iChannel, msg->MsgData.NoteParameter.iControl, msg->MsgData.NoteParameter.iParam);
      break;
    case  msgSetProgram:
      onSetProgram(track, msg->dwAbsPos, msg->MsgData.ChangeProgram.iChannel, msg->MsgData.ChangeProgram.iProgram);
      break;
    case  msgChangePressure:
      onChangePressure(track, msg->dwAbsPos, msg->MsgData.ChangePressure.iChannel, msg->MsgData.ChangePressure.iPressure);
      break;
    case  msgSetPitchWheel:
      onSetPitchWheel(track, msg->dwAbsPos, msg->MsgData.PitchWheel.iChannel, msg->MsgData.PitchWheel.iPitch + 8192);
      break;
    case  msgMetaEvent:
      switch (msg->MsgData.MetaEvent.iType) {
      case  metaMIDIPort:
        onMetaMIDIPort(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iMIDIPort);
        break;
      case  metaSequenceNumber:
        onMetaSequenceNumber(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iSequenceNumber);
        break;
      case  metaTextEvent:
        onMetaTextEvent(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaCopyright:
        onMetaCopyright(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaTrackName:
        onMetaTrackName(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaInstrument:
        onMetaInstrument(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaLyric:
        onMetaLyric(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaMarker:
        onMetaMarker(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaCuePoint:
        onMetaCuePoint(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case  metaEndSequence:
        onMetaEndSequence(track, msg->dwAbsPos);
        break;
      case  metaSetTempo:
        setPlaybackTempo(midiFile, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        onMetaSetTempo(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        break;
      case  metaSMPTEOffset:
        onMetaSMPTEOffset(track, msg->dwAbsPos,
          msg->MsgData.MetaEvent.Data.SMPTE.iHours,
          msg->MsgData.MetaEvent.Data.SMPTE.iMins,
          msg->MsgData.MetaEvent.Data.SMPTE.iSecs,
          msg->MsgData.MetaEvent.Data.SMPTE.iFrames,
          msg->MsgData.MetaEvent.Data.SMPTE.iFF
          );
        break;
      case  metaTimeSig:
        // TODO: Metronome and thirtyseconds are missing!!!
        onMetaTimeSig(track,
          msg->dwAbsPos,
          msg->MsgData.MetaEvent.Data.TimeSig.iNom,
          msg->MsgData.MetaEvent.Data.TimeSig.iDenom / MIDI_NOTE_CROCHET,
          0, 0
          );
        break;
      case  metaKeySig: // TODO: scale is missing!!!
        onMetaKeySig(track, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.KeySig.iKey, 0);
        break;
      case  metaSequencerSpecific:
        onMetaSequencerSpecific(track, msg->dwAbsPos,
        msg->MsgData.MetaEvent.Data.Sequencer.pData, msg->MsgData.MetaEvent.Data.Sequencer.iSize);
        break;
      }
      break;

    case  msgSysEx1:
    case  msgSysEx2:
      onMetaSysEx(track, msg->dwAbsPos, msg->MsgData.SysEx.pData, msg->MsgData.SysEx.iSize);
      break;
    }
}

void debugMidiPlayer() {
  static char pMidiFile[256];
  static char dir[256] = { 0 };
  scan_files(dir);

  while(TRUE) {
    printf("Enter MIDI File: ");
    getFileNameFromUart(pMidiFile);

    //debug
    enableMidiTimer();
    RCC_TypeDef* rcc = RCC;
    RegSTM32F4_RCC_APB1ENR_t* apb1enr = &RCC->APB1ENR;
    //

    printf("Now playing midi file: %s\n\r", pMidiFile);
    _MIDI_FILE pMF;
    BOOL open_success = 0;
    if(!playMidiFile2(pMidiFile))
      printf("Opening failed!\n\r");
    else {
      printf("Done.\n\r");
    }
  }

  printf("Unmounting SD card...\n\r");
  f_mount(0, "0:", 1); /* Unmount SD card */

}

///////////

void debugPrintNesGamePadState() {
  union NesGamePadStates_t state = getNesGamepadState();
  if(state.code == 0xFF)
    printf("Game pad is not plugged in\n\r");
  else {
    printf("A: ");      if(state.states.A)      printf(" ON"); else printf("OFF"); printf(" | ");
    printf("B: ");      if(state.states.B)      printf(" ON"); else printf("OFF"); printf(" | ");
    printf("UP: ");     if(state.states.North)  printf(" ON"); else printf("OFF"); printf(" | ");
    printf("DOWN: ");   if(state.states.South)  printf(" ON"); else printf("OFF"); printf(" | ");
    printf("LEFT: ");   if(state.states.West)   printf(" ON"); else printf("OFF"); printf(" | ");
    printf("RIGHT: ");  if(state.states.East)   printf(" ON"); else printf("OFF"); printf(" | ");
    printf("START: ");  if(state.states.Start)  printf(" ON"); else printf("OFF"); printf(" | ");
    printf("SELECT: "); if(state.states.Select) printf(" ON"); else printf("OFF"); printf(" | ");
    printf("SPI recv: 0x%X\r\n", SPI2->DR);
  }
  uint32_t lastTime = TIM5->CNT;
}

int main(void) {
  enableDelayTimer();
  enableMidiTimer();
  initializeDebugUart(115200);
  initializeBusUart(9600);
  setupNesGamePad();
  SSD1289_Init();
  SSD1289_Clear(Black);
  delayUs(100);

  // stop all drives
  for(int i = 0; i < 16; i++) {
    playNote(i, 0);
  }

  debugPrintln("\n\r");
  debugPrintln("################################");
  debugPrintln("Floppy Orgel v3.0 initialisiert.");
  debugPrintln("################################");

  FATFS SD_Fs;
  uint32_t free, total;

  /* Try to mount SD card */
  /* SD card is at 0: */
  printf("Mounting SD card... ");
  if (f_mount(&SD_Fs, "0:", 1) == FR_OK) {
    printf("success!\n\r"); /* Mounted ok */

    /* Get total and free space on SD card */
    TM_FATFS_DriveSize(&total, &free);

    /* Total space */
    printf("Total: %8u kB; %5u MB; %2u GB\r\n", total, total / 1024, total / 1048576);
    printf("Free:  %8u kB; %5u MB; %2u GB\r\n", free, free / 1024, free / 1048576);
  }
  else
    printf("Failed! No SD-Card?\r\n");

//  debugMidiPlayer();

  UB_Rng_Init(); // Init the random number generator
  init_stars();
  SSD1289_Clear(Green);

  char c = 0;
  printf("now listening on serial port...\n\r");

  // Main Loop
  g_midiPlayer.pMidiFile = NULL;


  while(1) {
	  drawMenu();

	  /*
	  if(_USART_getc(USART1, &c)) {
	    while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET); // Wait until transmit finishes
	      USART_SendData(USART6, c);
	  }
	  */
  }
}

