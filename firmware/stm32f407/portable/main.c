#include "stm32f4xx.h"
#include "delay.h"
#include "SSD1289.h"
#include "midifile.h"
#include "stm32_ub_rng.h"
#include "tm_stm32f4_fatfs.h"

signed int printf(const char *pFormat, ...);

void enableMidiTimer() {
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

// For later use...
void initSPI() {
  GPIO_InitTypeDef GPIO_InitStruct;
  SPI_InitTypeDef SPI_InitStruct;

  // enable clock for used IO pins
  //RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  /* configure pins used by SPI1
   * PB3 = SCK
   * PB4 = MISO
   * PB5 = MOSI
   */
  /*
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
  */

  // SCK
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStruct);

  // CS
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStruct);

  // MISO and MOSI
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStruct);


  // connect SPI1 pins to SPI alternate function
  /*
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);
  */
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource2,  GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource3,  GPIO_AF_SPI2);

  // enable peripheral clock
  //RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  /* configure SPI1 in Mode 0
   * CPOL = 0 --> clock is low when idle
   * CPHA = 0 --> data is sampled at the first edge
   */
  SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // set to full duplex mode, seperate MOSI and MISO lines
  SPI_InitStruct.SPI_Mode = SPI_Mode_Master;     // transmit in master mode, NSS pin has to be always high
  SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b; // one packet of data is 8 bits wide
  SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;        // clock is low when idle
  SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;      // data sampled at first edge
  SPI_InitStruct.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set; // set the NSS management to internal and pull internal NSS high
  SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // SPI frequency is APB2 frequency / 4
  SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;// data is transmitted MSB first
  //SPI_Init(SPI1, &SPI_InitStruct);
  SPI_Init(SPI2, &SPI_InitStruct);

  //SPI_Cmd(SPI1, ENABLE); // enable SPI1
  SPI_Cmd(SPI2, ENABLE); // enable SPI1

  uint8_t sendData = 0x09;

  for(int i = 0; i < 64; i++) {
      GPIOB->BSRRH |= GPIO_Pin_1; // CS low
      sendData = i;
      printf("SPI Send: %i\r\n", sendData);

      SPI2->DR = sendData; // write data to be transmitted to the SPI data register
      while( !(SPI2->SR & SPI_I2S_FLAG_TXE) ); // wait until transmit complete
      while( !(SPI2->SR & SPI_I2S_FLAG_RXNE) ); // wait until receive complete
      while( SPI2->SR & SPI_I2S_FLAG_BSY ); // wait until SPI is not busy anymore

      printf("SPI recv: 0x%X\r\n", SPI2->DR);
      GPIOB->BSRRL |= GPIO_Pin_1; // CS high
  }
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
int fputc(int ch) {
  // Wait until transmit finishes
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

  // Transmit the character using USART1
  USART_SendData(USART1, (u8) ch);
  return ch;
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


void drawFrame() {
  LCD_Clear(BLACK);
  move_and_draw_stars();
  //Delay(0x00FFFF);

  // TODO: backbuffered screen update here, to prevent flickering!
}

void debugPrintln(char* text) {
  printf("%s\r\n", text);
}



FRESULT scan_files (char* path)
{
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


void playMidiFile(const char *pFilename)
{
  _MIDI_FILE pMF;
  BOOL open_success;
  char str[128];
  int ev;

  midiFileOpen(&pMF, pFilename, &open_success);

  if (open_success)
  {
    MIDI_MSG msg;
    int i, iNum;
    unsigned int j;
    int any_track_had_data;
    uint32_t bpm = 120; // default value
    uint32_t current_midi_tick = 0;
    uint32_t ticks_to_wait = 0;
    uint32_t wait_until = 0;
    float us_per_tick = 60000000.0f / (bpm * pMF.Header.PPQN);

    midiReadInitMessage(&msg);
    iNum = midiReadGetNumTracks(&pMF);

    any_track_had_data = 1;
    while(any_track_had_data)
    {
      any_track_had_data = 0;
      ticks_to_wait = -1;

      for(i=0;i < iNum;i++)
      {
        if(midiReadGetNextMessage(&pMF, i, &msg)) // maybe this function will be the hardest part
        {
          any_track_had_data = 1;

          //printf("[Track: %d]", msg.iLastMsgChnl);

          if (msg.bImpliedMsg)
          { ev = msg.iImpliedMsg; }
          else
          { ev = msg.iType; }

          //printf(" %06d ", msg.dwAbsPos);


          //if (muGetMIDIMsgName(str, ev))
            //printf("%s  ", str);

          switch(ev)
          {
          case  msgNoteOff:
            muGetNameFromNote(str, msg.MsgData.NoteOff.iNote);
            playNote(msg.MsgData.NoteOn.iChannel, 0);
            //printf("(%d) %s\r\n", msg.MsgData.NoteOff.iChannel, str);
            break;
          case  msgNoteOn:
            muGetNameFromNote(str, msg.MsgData.NoteOn.iNote);
            playNote(msg.MsgData.NoteOn.iChannel, msg.MsgData.NoteOn.iNote);
            //printf("  (%d) %s %d\r\n", msg.MsgData.NoteOn.iChannel, str, msg.MsgData.NoteOn.iVolume);
            break;
          case  msgNoteKeyPressure:
            muGetNameFromNote(str, msg.MsgData.NoteKeyPressure.iNote);
            //printf("(%d) %s %d", msg.MsgData.NoteKeyPressure.iChannel, str, msg.MsgData.NoteKeyPressure.iPressure);
            break;
          case  msgSetParameter:
            muGetControlName(str, msg.MsgData.NoteParameter.iControl);
            //printf("(%d) %s -> %d", msg.MsgData.NoteParameter.iChannel, str, msg.MsgData.NoteParameter.iParam);
            break;
          case  msgSetProgram:
            //muGetInstrumentName(str, msg.MsgData.ChangeProgram.iProgram);
            //printf("(%d) %s", msg.MsgData.ChangeProgram.iChannel, str);
            break;
          case  msgChangePressure:
            muGetControlName(str, msg.MsgData.ChangePressure.iPressure);
            //printf("(%d) %s", msg.MsgData.ChangePressure.iChannel, str);
            break;
          case  msgSetPitchWheel:
            //printf("(%d) %d", msg.MsgData.PitchWheel.iChannel, msg.MsgData.PitchWheel.iPitch);
            break;

          case  msgMetaEvent:
            //printf("---- ");
            switch(msg.MsgData.MetaEvent.iType)
            {
            case  metaMIDIPort:
              //printf("MIDI Port = %d", msg.MsgData.MetaEvent.Data.iMIDIPort);
              break;

            case  metaSequenceNumber:
              //printf("Sequence Number = %d",msg.MsgData.MetaEvent.Data.iSequenceNumber);
              break;

            case  metaTextEvent:
              //printf("Text = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaCopyright:
              //printf("Copyright = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaTrackName:
              //printf("Track name = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaInstrument:
              //printf("Instrument = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaLyric:
              //printf("Lyric = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaMarker:
              //printf("Marker = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaCuePoint:
              //printf("Cue point = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaEndSequence:
              //printf("End Sequence");
              break;
            case  metaSetTempo:
              bpm = msg.MsgData.MetaEvent.Data.Tempo.iBPM;
              us_per_tick = 60000000.0f / (bpm * pMF.Header.PPQN);
              printf("Tempo = %d\r\n", msg.MsgData.MetaEvent.Data.Tempo.iBPM);
              break;
            case  metaSMPTEOffset:
              //printf("SMPTE offset = %d:%d:%d.%d %d",
              //  msg.MsgData.MetaEvent.Data.SMPTE.iHours,
              //  msg.MsgData.MetaEvent.Data.SMPTE.iMins,
              //  msg.MsgData.MetaEvent.Data.SMPTE.iSecs,
              //  msg.MsgData.MetaEvent.Data.SMPTE.iFrames,
              //  msg.MsgData.MetaEvent.Data.SMPTE.iFF
              //  );
              break;
            case  metaTimeSig:
              printf("Time sig = %d/%d\r\n",msg.MsgData.MetaEvent.Data.TimeSig.iNom,
              msg.MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
              break;
            case  metaKeySig:
              if (muGetKeySigName(str, msg.MsgData.MetaEvent.Data.KeySig.iKey))
                //printf("Key sig = %s", str);
              break;

            case  metaSequencerSpecific:
              //printf("Sequencer specific = ");
              //HexList(msg.MsgData.MetaEvent.Data.Sequencer.pData, msg.MsgData.MetaEvent.Data.Sequencer.iSize); // ok
              //printf("\r\n");
              break;
            }
            break;

          case  msgSysEx1:
          case  msgSysEx2:
            //printf("Sysex = ");
            HexList(msg.MsgData.SysEx.pData, msg.MsgData.SysEx.iSize); // ok
            break;
          }

          if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msg.MsgData.MetaEvent.iType==metaSequencerSpecific))
          {
            /* Already done a hex dump */
          }
          else
          {
            /*
            printf("  [");
            if (msg.bImpliedMsg) //printf("%X!", msg.iImpliedMsg);
            for(j=0;j<msg.iMsgSize;j++)
              printf("%X ", msg.data[j]);
            printf("]\r\n");
            */
          }
        }

        ticks_to_wait = (pMF.Track[i].pos - current_midi_tick > 0 && ticks_to_wait > pMF.Track[i].pos - current_midi_tick) ? pMF.Track[i].pos - current_midi_tick : ticks_to_wait;
      }

      if(ticks_to_wait == -1)
        ticks_to_wait = 0;

      // wait microseconds per tick here
      wait_until = ticks_to_wait * us_per_tick;
      //printf("TIM2->CNT: %d\r\n", TIM2->CNT);
      printf("wait: %d ms\r\n", wait_until / 1000);
      while(TIM2->CNT < wait_until) {
        wait_until = wait_until; // BP
      }
      TIM2->CNT = 0;

      current_midi_tick += ticks_to_wait;
    }

    midiReadFreeMessage(&msg);
  }
  else
  {
    printf("Open Failed!\nInvalid MIDI-File Header!\n");

  }
}

void playMidiFile2(const char *pFilename)
{
  _MIDI_FILE pMF;
  BOOL open_success;
  char str[128];
  int ev;
  int i = 0;
  int wait_until = 0;

  midiFileOpen(&pMF, pFilename, &open_success);
  if (open_success)
  {
    static MIDI_MSG msg[MAX_MIDI_TRACKS];
    int i, iNum;
    unsigned int j;
    int any_track_had_data = 1;
    DWORD current_midi_tick = 0;
    DWORD bpm = 120;
    float us_per_tick = 0;
    DWORD ticks_to_wait = 0;

    iNum = midiReadGetNumTracks(&pMF);

    for(i=0; i< iNum; i++)
    {
      midiReadInitMessage(&msg[i]);
      midiReadGetNextMessage(&pMF, i, &msg[i]);
    }

    printf("start playing...\r\n");
    TIM2->CNT = 0;

    while(any_track_had_data)
    {
      any_track_had_data = 1;
      ticks_to_wait = -1;

      for(i=0; i < iNum; i++)
      {
        //printf("while loop us: ");
        while(current_midi_tick == pMF.Track[i].pos && pMF.Track[i].ptr2 != pMF.Track[i].pEnd2)
        {
          int loopStart = TIM2->CNT; // time measurement
          //printf("[Track: %d]", i);

          if (msg[i].bImpliedMsg)
          { ev = msg[i].iImpliedMsg; }
          else
          { ev = msg[i].iType; }

          //printf(" %06d ", msg[i].dwAbsPos);


          if (muGetMIDIMsgName(str, ev))
            ;//printf("%s  ", str);

          switch(ev)
          {
          case  msgNoteOff:
            muGetNameFromNote(str, msg[i].MsgData.NoteOff.iNote);
            playNote(msg[i].MsgData.NoteOn.iChannel, 0);
        //    printf("(%d) %s", msg[i].MsgData.NoteOff.iChannel, str);
            //midiOutShortMsg(hMidiOut, (0 << 16) | (msg[i].MsgData.NoteOff.iNote << 8) | (0x80 + msg[i].MsgData.NoteOff.iChannel - 1) ); // note off
            break;
          case  msgNoteOn:
            muGetNameFromNote(str, msg[i].MsgData.NoteOn.iNote);
            if(msg[i].MsgData.NoteOn.iVolume > 0)
              playNote(msg[i].MsgData.NoteOn.iChannel, msg[i].MsgData.NoteOn.iNote);
            else
              playNote(msg[i].MsgData.NoteOn.iChannel, 0);

        //    printf("  (%d) %s %d", msg[i].MsgData.NoteOn.iChannel, str, msg[i].MsgData.NoteOn.iVolume);
            //midiOutShortMsg(hMidiOut, (msg[i].MsgData.NoteOn.iVolume << 16) | (msg[i].MsgData.NoteOn.iNote << 8) | (0x90 + msg[i].MsgData.NoteOn.iChannel - 1) ); // note on
            break;
          case  msgNoteKeyPressure:
            muGetNameFromNote(str, msg[i].MsgData.NoteKeyPressure.iNote);
            //printf("(%d) %s %d", msg[i].MsgData.NoteKeyPressure.iChannel,
            //  str,
            //  msg[i].MsgData.NoteKeyPressure.iPressure);
            break;
          case  msgSetParameter:
            muGetControlName(str, msg[i].MsgData.NoteParameter.iControl);
            //printf("(%d) %s -> %d", msg[i].MsgData.NoteParameter.iChannel,
            //  str, msg[i].MsgData.NoteParameter.iParam);
            break;
          case  msgSetProgram:
            //midiOutShortMsg(hMidiOut, (0 << 16) | (msg[i].MsgData.ChangeProgram.iProgram << 8) | 0xC0 + msg[i].MsgData.ChangeProgram.iChannel); // set program

            muGetInstrumentName(str, msg[i].MsgData.ChangeProgram.iProgram);
            printf("(%d) %s", msg[i].MsgData.ChangeProgram.iChannel, str);
            break;
          case  msgChangePressure:
            muGetControlName(str, msg[i].MsgData.ChangePressure.iPressure);
            //printf("(%d) %s", msg[i].MsgData.ChangePressure.iChannel, str);
            break;
          case  msgSetPitchWheel:
            //printf("(%d) %d", msg[i].MsgData.PitchWheel.iChannel,
              //msg[i].MsgData.PitchWheel.iPitch);
            break;

          case  msgMetaEvent:
            //printf("---- ");
            switch(msg[i].MsgData.MetaEvent.iType)
            {
            case  metaMIDIPort:
              //printf("MIDI Port = %d", msg[i].MsgData.MetaEvent.Data.iMIDIPort);
              break;

            case  metaSequenceNumber:
              //printf("Sequence Number = %d",msg[i].MsgData.MetaEvent.Data.iSequenceNumber);
              break;

            case  metaTextEvent:
              //printf("Text = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaCopyright:
              //printf("Copyright = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaTrackName:
              //printf("Track name = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaInstrument:
              //printf("Instrument = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaLyric:
              //printf("Lyric = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaMarker:
              //printf("Marker = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaCuePoint:
              //printf("Cue point = '%s'",msg[i].MsgData.MetaEvent.Data.Text.pData);
              break;
            case  metaEndSequence:
              //printf("End Sequence");
              break;
            case  metaSetTempo:
              bpm = msg[i].MsgData.MetaEvent.Data.Tempo.iBPM;
              us_per_tick = 60000000.0f / (bpm * pMF.Header.PPQN);
              printf("Tempo = %d", msg[i].MsgData.MetaEvent.Data.Tempo.iBPM);
              break;
            case  metaSMPTEOffset:
              //printf("SMPTE offset = %d:%d:%d.%d %d",
              //  msg[i].MsgData.MetaEvent.Data.SMPTE.iHours,
              //  msg[i].MsgData.MetaEvent.Data.SMPTE.iMins,
              //  msg[i].MsgData.MetaEvent.Data.SMPTE.iSecs,
              //  msg[i].MsgData.MetaEvent.Data.SMPTE.iFrames,
              //  msg[i].MsgData.MetaEvent.Data.SMPTE.iFF
              //  );
              break;
            case  metaTimeSig:
              printf("Time sig = %d/%d",msg[i].MsgData.MetaEvent.Data.TimeSig.iNom,
                msg[i].MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
              break;
            case  metaKeySig:
              if (muGetKeySigName(str, msg[i].MsgData.MetaEvent.Data.KeySig.iKey))
                printf("Key sig = %s", str);
              break;

            case  metaSequencerSpecific:
              //printf("Sequencer specific = ");
              //HexList(msg[i].MsgData.MetaEvent.Data.Sequencer.pData, msg[i].MsgData.MetaEvent.Data.Sequencer.iSize); // ok
              //printf("\r\n");
              break;
            }
            break;

          case  msgSysEx1:
          case  msgSysEx2:
            //printf("Sysex = ");
            //HexList(msg[i].MsgData.SysEx.pData, msg[i].MsgData.SysEx.iSize); // ok
            break;
          }

          if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msg[i].MsgData.MetaEvent.iType==metaSequencerSpecific))
          {
            // Already done a hex dump
          }
          else
          {

            //printf("  [");
            //if (msg[i].bImpliedMsg) printf("%X!", msg[i].iImpliedMsg);
            //for(j=0;j<msg[i].iMsgSize;j++)
            //  printf("%X ", msg[i].data[j]);
            //printf("]\r\n");
          }

          if(midiReadGetNextMessage(&pMF, i, &msg[i]))
          {
            any_track_had_data = 1; // 0 ???
          }

          //printf("%d\r\n", TIM2->CNT - loopStart);
        }
        ticks_to_wait = (pMF.Track[i].pos - current_midi_tick > 0 && ticks_to_wait > pMF.Track[i].pos - current_midi_tick) ? pMF.Track[i].pos - current_midi_tick : ticks_to_wait;
      }


      if(ticks_to_wait == -1)
        ticks_to_wait = 0;

      // wait microseconds per tick here
      wait_until = ticks_to_wait * us_per_tick;
      //if(TIM2->CNT > wait_until)
        //printf("warning: Did wait %d of %d!\r\n", TIM2->CNT, wait_until);

      while( TIM2->CNT < wait_until )
      {

        //printf("TIM2->CNT: %d us / %d us\r\n", TIM2->CNT, wait_until);
        // just wait here...
      }
      TIM2->CNT = 0;

      current_midi_tick += ticks_to_wait;
    }

    midiReadFreeMessage(&msg);
    //midiFileClose(&pMF);


    printf("done.\r\n");
  }
  else
  {
    printf("Open Failed!\nInvalid MIDI-File Header!\n");

  }
}


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

int main(void) {
  initializeDebugUart(115200);
  initializeBusUart(9600);
  debugPrintln("Floppy Orgel v3.0 initialisiert.");

  FATFS SD_Fs;
  uint32_t free, total;

  /* Try to mount SD card */
  /* SD card is at 0: */
  printf("Mounting SD card... ");
  if (f_mount(&SD_Fs, "0:", 1) == FR_OK) {
    printf("success!\r\n"); /* Mounted ok */

    /* Get total and free space on SD card */
    TM_FATFS_DriveSize(&total, &free);

    /* Total space */
    printf("Total: %8u kB; %5u MB; %2u GB\r\n", total, total / 1024, total / 1048576);
    printf("Free:  %8u kB; %5u MB; %2u GB\r\n", free, free / 1024, free / 1048576);
  }
  else
    printf("Failed! No SD-Card?\r\n");


  char dir[256] = { 0 };
  //dir[0] = '/';
  scan_files(dir);

  // MIDI
  //char* pMidiFile = "zelda0.mid";
  char pMidiFile[256];
  printf("Enter MIDI File: ");
  getFileNameFromUart(pMidiFile);

  //debug
  enableMidiTimer();
  RCC_TypeDef* rcc = RCC;
  RegSTM32F4_RCC_APB1ENR_t* apb1enr = &RCC->APB1ENR;
  //

  printf("Now playing midi file: %s\r\n", pMidiFile);
  _MIDI_FILE pMF;
  BOOL open_success = 0;
  playMidiFile2(pMidiFile);

  Delay(0x3FFFFF);
  LCD_Init();
  Delay(0x3FFFFF);
  //touch_init();
  LCD_Clear(BLACK);
  Delay(0x3FFFFF);

  UB_Rng_Init(); // Init the random number generator
  init_stars();
  LCD_Clear(GREEN);

  char c = 0;

  printf("Unmounting SD card...\n");
  f_mount(0, "0:", 1); /* Unmount SD card */
  printf("Done.\r\n");

  // Main Loop
  while(1) {
	  drawFrame();
	  if(_USART_getc(USART1, &c)) {
	    //while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET); // Wait until transmit finishes
	    USART_SendData(USART6, c);
	  }
  }
}

