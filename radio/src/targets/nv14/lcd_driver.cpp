/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "lcd_driver.h"
uint8_t LCD_FIRST_FRAME_BUFFER[DISPLAY_BUFFER_SIZE] __SDRAM;
uint8_t LCD_SECOND_FRAME_BUFFER[DISPLAY_BUFFER_SIZE] __SDRAM;
uint8_t LCD_BACKUP_FRAME_BUFFER[DISPLAY_BUFFER_SIZE] __SDRAM;

uint32_t CurrentLayer = LCD_FIRST_LAYER;

lcdMethod lcdOffFunction;
lcdMethod lcdOnFunction;

volatile U8 LCD_ReadBuffer[24] = {0, 0};

static void LCD_Delay(void)
{
  volatile unsigned int i;

  for (i = 0; i < 20; i++)
  {
    ;
  }
}

enum ENUM_IO_SPEED
{
  IO_SPEED_LOW,
  IO_SPEED_MID,
  IO_SPEED_QUICK,
  IO_SPEED_HIGH
};

enum ENUM_IO_MODE
{
  IO_MODE_INPUT,
  IO_MODE_OUTPUT,
  IO_MODE_ALTERNATE,
  IO_MODE_ANALOG
};

void GPIO_SetDirection(GPIO_TypeDef *GPIOx, unsigned char Pin, unsigned char IsInput)
{
  unsigned int Mask;
  unsigned int Position;
  unsigned int Register;

  Position = Pin << 1;
  Mask = ~(0x03UL << Position);

  //EnterCritical();
  Register = GPIOx->OSPEEDR & Mask;
  Register |= IO_SPEED_HIGH << Position;
  GPIOx->OSPEEDR = Register;
  //ExitCritical();

  //EnterCritical();
  Register = GPIOx->MODER & Mask;
  if (!IsInput)
  {
    Register |= IO_MODE_OUTPUT << Position;
  }

  GPIOx->MODER = Register;
  //ExitCritical();
}
static void LCD_AF_GPIOConfig(void)
{
  /*
   -----------------------------------------------------------------------------
   LCD_CLK <-> PG.07 | LCD_HSYNC <-> PI.12 | LCD_R3 <-> PJ.02 | LCD_G5 <-> PK.00
   | LCD VSYNC <-> PI.13 | LCD_R4 <-> PJ.03 | LCD_G6 <-> PK.01
   |                     | LCD_R5 <-> PJ.04 | LCD_G7 <-> PK.02
   |                     | LCD_R6 <-> PJ.05 | LCD_B4 <-> PK.03
   |                     | LCD_R7 <-> PJ.06 | LCD_B5 <-> PK.04
   |                     | LCD_G2 <-> PJ.09 | LCD_B6 <-> PK.05
   |                     | LCD_G3 <-> PJ.10 | LCD_B7 <-> PK.06
   |                     | LCD_G4 <-> PJ.11 | LCD_DE <-> PK.07
   |                     | LCD_B3 <-> PJ.15 |
   */

  // GPIOG configuration
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource7, GPIO_AF_LTDC);
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOG, &GPIO_InitStructure);

  // GPIOI configuration
  GPIO_PinAFConfig(GPIOI, GPIO_PinSource12, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOI, GPIO_PinSource13, GPIO_AF_LTDC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
  GPIO_Init(GPIOI, &GPIO_InitStructure);

  // GPIOJ configuration
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource2, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource3, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource4, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource5, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource6, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource9, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource10, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource11, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOJ, GPIO_PinSource15, GPIO_AF_LTDC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_15;
  GPIO_Init(GPIOJ, &GPIO_InitStructure);

  // GPIOK configuration
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource0, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource1, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource2, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource3, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource4, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource5, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource6, GPIO_AF_LTDC);
  GPIO_PinAFConfig(GPIOK, GPIO_PinSource7, GPIO_AF_LTDC);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOK, &GPIO_InitStructure);
}

static void lcdSpiConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = LCD_SPI_SCK_GPIO_PIN | LCD_SPI_MOSI_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(LCD_SPI_GPIO, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = LCD_SPI_CS_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(LCD_SPI_GPIO, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = LCD_NRST_GPIO_PIN;
  GPIO_Init(LCD_NRST_GPIO, &GPIO_InitStructure);

  /* Set the chip select pin aways low */
  CLR_LCD_CS();
}

unsigned char LCD_ReadByteOnFallingEdge(void)
{
  unsigned int i;
  unsigned char ReceiveData = 0;

  SET_LCD_DATA();
  SET_LCD_DATA_INPUT();

  for (i = 0; i < 8; i++)
  {
    LCD_DELAY();
    SET_LCD_CLK();
    LCD_DELAY();
    LCD_DELAY();
    ReceiveData <<= 1;

    CLR_LCD_CLK();
    LCD_DELAY();
    LCD_DELAY();
    if (READ_LCD_DATA_PIN())
    {
      ReceiveData |= 0x01;
    }
  }

  SET_LCD_DATA_OUTPUT();

  return (ReceiveData);
}

static void LCD_WriteByte(uint8_t data_enable, uint8_t byte)
{
  LCD_SCK_LOW();
  LCD_DELAY();

  if (data_enable)
  {
    LCD_MOSI_HIGH();
  }
  else
  {
    LCD_MOSI_LOW();
  }

  LCD_SCK_HIGH();
  LCD_DELAY();

  for (int i = 0; i < 8; i++)
  {
    LCD_SCK_LOW();
    LCD_DELAY();

    if (byte & 0x80)
    {
      LCD_MOSI_HIGH();
    }
    else
    {
      LCD_MOSI_LOW();
    }

    LCD_SCK_HIGH();
    byte <<= 1;

    LCD_DELAY();
  }

  LCD_SCK_LOW();
}

unsigned char LCD_ReadByte(void)
{
  unsigned int i;
  unsigned char ReceiveData = 0;

  SET_LCD_DATA();
  SET_LCD_DATA_INPUT();
  for (i = 0; i < 8; i++)
  {
    CLR_LCD_CLK();
    LCD_DELAY();
    LCD_DELAY();
    ReceiveData <<= 1;
    SET_LCD_CLK();
    LCD_DELAY();
    LCD_DELAY();
    if (READ_LCD_DATA_PIN())
    {
      ReceiveData |= 0x01;
    }
  }
  CLR_LCD_CLK();
  SET_LCD_DATA_OUTPUT();
  return (ReceiveData);
}

unsigned char LCD_ReadRegister(unsigned char Register)
{
  unsigned char ReadData = 0;

  LCD_WriteByte(0, Register);
  LCD_DELAY();
  LCD_DELAY();
  ReadData = LCD_ReadByte();
  return (ReadData);
}

void LCD_WriteCommand(uint8_t command)
{
  LCD_WriteByte(0, command);
}

void LCD_WriteData(uint8_t data)
{
  LCD_WriteByte(1, data);
}

#define Command (1)
#define Parameter (0)
void Write(unsigned char isCommand, unsigned char data)
{
  if (isCommand)
    LCD_WriteCommand(data);
  else
    LCD_WriteData(data);
}

void LCD_ILI9481_Init(void)
{
  LCD_WriteCommand(0x11);
  delay_ms(120);

  LCD_WriteCommand(0xE4);
  LCD_WriteData(0x0A);

  LCD_WriteCommand(0xF0);
  LCD_WriteData(0x01);

  LCD_WriteCommand(0xF3);
  LCD_WriteData(0x02);
  LCD_WriteData(0x1A);

  LCD_WriteCommand(0xD0);
  LCD_WriteData(0x07);
  LCD_WriteData(0x42);
  LCD_WriteData(0x1B);

  LCD_WriteCommand(0xD1);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00); //04
  LCD_WriteData(0x1A);

  LCD_WriteCommand(0xD2);
  LCD_WriteData(0x01);
  LCD_WriteData(0x00); //11

  LCD_WriteCommand(0xC0);
  LCD_WriteData(0x10);
  LCD_WriteData(0x3B); //
  LCD_WriteData(0x00); //
  LCD_WriteData(0x02);
  LCD_WriteData(0x11);

  LCD_WriteCommand(0xC5);
  LCD_WriteData(0x03);

  LCD_WriteCommand(0xC8);
  LCD_WriteData(0x00);
  LCD_WriteData(0x01);
  LCD_WriteData(0x47);
  LCD_WriteData(0x60);
  LCD_WriteData(0x04);
  LCD_WriteData(0x16);
  LCD_WriteData(0x03);
  LCD_WriteData(0x67);
  LCD_WriteData(0x67);
  LCD_WriteData(0x06);
  LCD_WriteData(0x0F);
  LCD_WriteData(0x00);

  LCD_WriteCommand(0x36);
  LCD_WriteData(0x08);

  LCD_WriteCommand(0x3A);
  LCD_WriteData(0x66); //0x55=65k color, 0x66=262k color.

  LCD_WriteCommand(0x2A);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00);
  LCD_WriteData(0x01);
  LCD_WriteData(0x3F);

  LCD_WriteCommand(0x2B);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00);
  LCD_WriteData(0x01);
  LCD_WriteData(0xE0);

  LCD_WriteCommand(0xB4);
  LCD_WriteData(0x11);

  LCD_WriteCommand(0xc6);
  LCD_WriteData(0x82);

  delay_ms(120);

  LCD_WriteCommand(0x21);
  LCD_WriteCommand(0x29);
  LCD_WriteCommand(0x2C);
  LCD_WriteData(0x04);
  LCD_WriteData(0x16);
  LCD_WriteData(0x03);
  LCD_WriteData(0x67);
  LCD_WriteData(0x67);
  LCD_WriteData(0x06);
  LCD_WriteData(0x0F);
  LCD_WriteData(0x00);

  LCD_WriteCommand(0x36);
  LCD_WriteData(0x08);

  LCD_WriteCommand(0x3A);
  LCD_WriteData(0x66); //0x55=65k color, 0x66=262k color.

  LCD_WriteCommand(0x2A);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00);
  LCD_WriteData(0x01);
  LCD_WriteData(0x3F);

  LCD_WriteCommand(0x2B);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00);
  LCD_WriteData(0x01);
  LCD_WriteData(0xE0);

  LCD_WriteCommand(0xB4);
  LCD_WriteData(0x11);

  LCD_WriteCommand(0xc6);
  LCD_WriteData(0x82);

  delay_ms(120);

  LCD_WriteCommand(0x21);
  LCD_WriteCommand(0x29);
  LCD_WriteCommand(0x2C);
}

void LCD_ILI9481_On(void)
{
  LCD_WriteCommand(0x29);
}

void LCD_ILI9481_Off(void)
{
  LCD_WriteCommand(0x28);
}

unsigned int LCD_ILI9481_ReadID(void)
{
  int ID = 0;
  volatile int Data;

  CLR_LCD_CS();
  LCD_WriteByte(0, 0xBF);
  Data = LCD_ReadByteOnFallingEdge();
  Data = LCD_ReadByteOnFallingEdge();
  ID = LCD_ReadByteOnFallingEdge();
  ID <<= 8;
  ID |= LCD_ReadByteOnFallingEdge();
  Data = LCD_ReadByteOnFallingEdge();
  Data = LCD_ReadByteOnFallingEdge();
  SET_LCD_CS();
  LCD_DELAY();
  LCD_DELAY();
  LCD_DELAY();
  LCD_WriteCommand(0xC6);
  LCD_WriteData(0x82);
  //LCD_WriteData( 0x9b );
  return (ID);
}

void LCD_ILI9486_On(void)
{
  LCD_WriteCommand(0x29);
}
void LCD_ILI9486_Off(void)
{
  LCD_WriteCommand(0x28);
}

void LCD_ILI9486_Init(void)
{
  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x00);

  LCD_WriteCommand(0xf2);
  LCD_WriteData(0x18);
  LCD_WriteData(0xa3);
  LCD_WriteData(0x12);
  LCD_WriteData(0x02);
  LCD_WriteData(0xb2);
  LCD_WriteData(0x12);
  LCD_WriteData(0xff);
  LCD_WriteData(0x13);
  LCD_WriteData(0x00);
  LCD_WriteCommand(0xf1);
  LCD_WriteData(0x36);
  LCD_WriteData(0x04);
  LCD_WriteData(0x00);
  LCD_WriteData(0x3c);
  LCD_WriteData(0x0f);
  LCD_WriteData(0x8f);
  LCD_WriteCommand(0xf8);
  LCD_WriteData(0x21);
  LCD_WriteData(0x04);
  LCD_WriteCommand(0xf9);
  LCD_WriteData(0x00);
  LCD_WriteData(0x08);
  LCD_WriteCommand(0x36);
  //    if( IsHorizontal )
  //    {
  //        LCD_WriteData( 0x08 );
  //    }
  //    else
  //    {
  //        LCD_WriteData( 0x18 );
  //    }
  LCD_WriteData(0x08);

  LCD_WriteCommand(0x3a);
  LCD_WriteData(0x65);
  LCD_WriteCommand(0xc0);
  LCD_WriteData(0x0f);
  LCD_WriteData(0x0f);
  LCD_WriteCommand(0xc1);
  LCD_WriteData(0x41);

  LCD_WriteCommand(0xc5);
  LCD_WriteData(0x00);
  LCD_WriteData(0x27);
  LCD_WriteData(0x80);
  LCD_WriteCommand(0xb6);
#if 0 //解决很切屏 中间断层问题
    LCD_WriteData(0xb2);
    LCD_WriteData(0x42);
#else
  LCD_WriteData(0x22);
  LCD_WriteData(0x42); //0x02
#endif
  LCD_WriteData(0x3b);
  LCD_WriteCommand(0xb1);
  LCD_WriteData(0xb0);
  LCD_WriteData(0x11);
  LCD_WriteCommand(0xb4);
  LCD_WriteData(0x02);
  LCD_WriteCommand(0xb7);
  LCD_WriteData(0xC6);

  LCD_WriteCommand(0xe0);
  LCD_WriteData(0x0f);
  LCD_WriteData(0x1C);
  LCD_WriteData(0x18);
  LCD_WriteData(0x0B);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x06);
  LCD_WriteData(0x48);
  LCD_WriteData(0x87);
  LCD_WriteData(0x3A);
  LCD_WriteData(0x09);
  LCD_WriteData(0x15);
  LCD_WriteData(0x08);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x04);
  LCD_WriteData(0x00);

  LCD_WriteCommand(0xe1);
  LCD_WriteData(0x0f);
  LCD_WriteData(0x37);
  LCD_WriteData(0x34);
  LCD_WriteData(0x0A);
  LCD_WriteData(0x0B);
  LCD_WriteData(0x03);
  LCD_WriteData(0x4B);
  LCD_WriteData(0x31);
  LCD_WriteData(0x39);
  LCD_WriteData(0x03);
  LCD_WriteData(0x0F);
  LCD_WriteData(0x03);
  LCD_WriteData(0x22);
  LCD_WriteData(0x1D);
  LCD_WriteData(0x00);

  LCD_WriteCommand(0x21);
  LCD_WriteCommand(0x11);
  delay_ms(120);
  LCD_WriteCommand(0x28);
}

unsigned int LCD_ILI9486_ReadID(void)
{
  int ID = 0;

  LCD_WriteCommand(0XF7);
  LCD_WriteData(0xA9);
  LCD_WriteData(0x51);
  LCD_WriteData(0x2C);
  LCD_WriteData(0x82);
  LCD_WriteCommand(0XB0);
  LCD_WriteData(0X80);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x00);
  ID = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x01);
  ID = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x02);
  ID = LCD_ReadRegister(0xd3);
  ID <<= 8;
  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x03);
  ID |= LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x00);

  return (ID);
}

void LCD_ILI9488_On(void)
{
  LCD_WriteCommand(0x29);
}

void LCD_ILI9488_Init(void)
{
  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x00);

#if 0
    LCD_WriteCommand( 0XF7 );
    LCD_WriteData( 0xA9 );
    LCD_WriteData( 0x51 );
    LCD_WriteData( 0x2C );
    LCD_WriteData( 0x82 );
#endif

  LCD_WriteCommand(0xC0);
  LCD_WriteData(0x11);
  LCD_WriteData(0x09);

  LCD_WriteCommand(0xC1);
  LCD_WriteData(0x41);

  LCD_WriteCommand(0XC5);
  LCD_WriteData(0x00);
  LCD_WriteData(0x0A);
  LCD_WriteData(0x80);

  LCD_WriteCommand(0xB1);
  LCD_WriteData(0xB0);
  LCD_WriteData(0x11);

  LCD_WriteCommand(0xB4);
  LCD_WriteData(0x02);

  LCD_WriteCommand(0xB6);
  LCD_WriteData(0x30);
  LCD_WriteData(0x02);

  LCD_WriteCommand(0xB7);
  LCD_WriteData(0xc6);

  LCD_WriteCommand(0xBE);
  LCD_WriteData(0x00);
  LCD_WriteData(0x04);

  LCD_WriteCommand(0xE9);
  LCD_WriteData(0x00);

  LCD_WriteCommand(0x36);
  LCD_WriteData(0x08);

  LCD_WriteCommand(0x3A);
  LCD_WriteData(0x65);

  LCD_WriteCommand(0xE0);
  LCD_WriteData(0x00);
  LCD_WriteData(0x07);
  LCD_WriteData(0x10);
  LCD_WriteData(0x09);
  LCD_WriteData(0x17);
  LCD_WriteData(0x0B);
  LCD_WriteData(0x41);
  LCD_WriteData(0x89);
  LCD_WriteData(0x4B);
  LCD_WriteData(0x0A);
  LCD_WriteData(0x0C);
  LCD_WriteData(0x0E);
  LCD_WriteData(0x18);
  LCD_WriteData(0x1B);
  LCD_WriteData(0x0F);

  LCD_WriteCommand(0XE1);
  LCD_WriteData(0x00);
  LCD_WriteData(0x17);
  LCD_WriteData(0x1A);
  LCD_WriteData(0x04);
  LCD_WriteData(0x0E);
  LCD_WriteData(0x06);
  LCD_WriteData(0x2F);
  LCD_WriteData(0x45);
  LCD_WriteData(0x43);
  LCD_WriteData(0x02);
  LCD_WriteData(0x0A);
  LCD_WriteData(0x09);
  LCD_WriteData(0x32);
  LCD_WriteData(0x36);
  LCD_WriteData(0x0F);

  LCD_WriteCommand(0x11);
  delay_ms(120);
  LCD_WriteCommand(0x28);

  LCD_ILI9488_On();
}

void LCD_ILI9488_Off(void)
{
  LCD_WriteCommand(0x28);
}

void LCD_ILI9488_ReadDevice(void)
{
  int Index = 0;
  int parameter = 0x80;

#if 1

#if 1
  LCD_WriteCommand(0XF7);
  LCD_WriteData(0xA9);
  LCD_WriteData(0x51);
  LCD_WriteData(0x2C);
  LCD_WriteData(0x82);

  LCD_WriteCommand(0XB0);
  LCD_WriteData(0X80);

#endif
  LCD_WriteCommand(0XFB);
  LCD_WriteData(parameter | 0x00);
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);

  //LCD_WriteCommand(0X2E);
  LCD_WriteCommand(0XFB);
  LCD_WriteData(parameter | 0x01); //Parameter2=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(parameter | 0x02); //Parameter2=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(parameter | 0x03); //Parameter2=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);
#endif

#if 0
  LCD_WriteCommand( 0XFB );
  LCD_WriteData( parameter|0x00 );        //Parameter3=0X94
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );
  LCD_WriteData( parameter|0x01 );//Parameter3=0X94
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );
  LCD_WriteCommand( 0XFB );
  LCD_WriteData( parameter|0x02 );//Parameter3=0X94
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );

  LCD_WriteCommand( 0XFB );
  LCD_WriteData( parameter|0x03 );//Parameter4=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );
#else
  //LCD_WriteCommand( 0xd0 );
  //LCD_WriteData( parameter|0x03 );        //Parameter4=0X88
  //LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd0 );
#endif
}

unsigned int LCD_ILI9488_ReadID(void)
{
  int ID = 0;

  LCD_WriteCommand(0XF7);
  LCD_WriteData(0xA9);
  LCD_WriteData(0x51);
  LCD_WriteData(0x2C);
  LCD_WriteData(0x82);
  LCD_WriteCommand(0XB0);
  LCD_WriteData(0X80);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x80 | 0x00);
  ID = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x80 | 0x01);
  ID = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x80 | 0x02);
  ID = LCD_ReadRegister(0xd3);
  ID <<= 8;

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x80 | 0x03);
  ID |= LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x00);
  return (ID);
}

void LCD_ST7796S_On(void)
{
  LCD_WriteCommand(0x29);
}

void LCD_ST7796S_Init(void)
{
  delay_ms(120);

  LCD_WriteCommand(0x11);
  delay_ms(120);
  LCD_WriteCommand(0xF0);
  LCD_WriteData(0xC3);

  LCD_WriteCommand(0xF0);
  LCD_WriteData(0x96);

  LCD_WriteCommand(0x36);
  LCD_WriteData(0x88);

  LCD_WriteCommand(0x3A);
  LCD_WriteData(0x66);

  //SET RGB STRAT
  LCD_WriteCommand(0xB0); //SET HS VS DE CLK 上升还是下降有效
  LCD_WriteData(0x80);

  LCD_WriteCommand(0xB4);
  LCD_WriteData(0x01);

  LCD_WriteCommand(0xB6);
  LCD_WriteData(0x20);
  LCD_WriteData(0x02);
  LCD_WriteData(0x3B);
  //SET RGB END

  LCD_WriteCommand(0xB7);
  LCD_WriteData(0xC6);

  LCD_WriteCommand(0xB9);
  LCD_WriteData(0x02);
  LCD_WriteData(0xE0);

  LCD_WriteCommand(0xC0);
  LCD_WriteData(0x80);
  LCD_WriteData(0x65);

  LCD_WriteCommand(0xC1);
  LCD_WriteData(0x0D);

  LCD_WriteCommand(0xC2);
  LCD_WriteData(0xA7);

  LCD_WriteCommand(0xC5);
  LCD_WriteData(0x14);

  LCD_WriteCommand(0xE8);
  LCD_WriteData(0x40);
  LCD_WriteData(0x8A);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00);
  LCD_WriteData(0x29);
  LCD_WriteData(0x19);
  LCD_WriteData(0xA5);
  LCD_WriteData(0x33);

  LCD_WriteCommand(0xE0);
  LCD_WriteData(0xD0);
  LCD_WriteData(0x00);
  LCD_WriteData(0x04);
  LCD_WriteData(0x05);
  LCD_WriteData(0x04);
  LCD_WriteData(0x21);
  LCD_WriteData(0x25);
  LCD_WriteData(0x43);
  LCD_WriteData(0x3F);
  LCD_WriteData(0x37);
  LCD_WriteData(0x13);
  LCD_WriteData(0x13);
  LCD_WriteData(0x29);
  LCD_WriteData(0x32);

  LCD_WriteCommand(0xE1);
  LCD_WriteData(0xD0);
  LCD_WriteData(0x04);
  LCD_WriteData(0x06);
  LCD_WriteData(0x09);
  LCD_WriteData(0x06);
  LCD_WriteData(0x03);
  LCD_WriteData(0x25);
  LCD_WriteData(0x32);
  LCD_WriteData(0x3E);
  LCD_WriteData(0x18);
  LCD_WriteData(0x15);
  LCD_WriteData(0x15);
  LCD_WriteData(0x2B);
  LCD_WriteData(0x30);

  LCD_WriteCommand(0xF0);
  LCD_WriteData(0x3C);

  LCD_WriteCommand(0xF0);
  LCD_WriteData(0x69);

  delay_ms(120);

  LCD_WriteCommand(0x21);

  LCD_WriteCommand(0x29);
}

void LCD_ST7796S_Off(void)
{
  LCD_WriteCommand(0x28);
}

unsigned int LCD_ST7796S_ReadID(void)
{
  int ID = 0;

  LCD_WriteCommand(0XF7);
  LCD_WriteData(0xA9);
  LCD_WriteData(0x51);
  LCD_WriteData(0x2C);
  LCD_WriteData(0x82);
  LCD_WriteCommand(0XB0);
  LCD_WriteData(0X80);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x00);
  ID = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x01);
  ID = LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x02);
  ID = LCD_ReadRegister(0xd3);
  ID <<= 8;
  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x10 | 0x03);
  ID |= LCD_ReadRegister(0xd3);

  LCD_WriteCommand(0XFB);
  LCD_WriteData(0x00);

  return (ID);
}

static void lcdReset()
{
  LCD_NRST_HIGH();
  delay_ms(1);

  LCD_NRST_LOW(); // RESET();
  delay_ms(100);

  LCD_NRST_HIGH();
  delay_ms(100);
}

const STRUCT_LCD_DRIVER LCD_Devices[] = {
  {0x9486, 320, 480, 12000000, LCD_ILI9486_Init, LCD_ILI9486_On, LCD_ILI9486_Off, LCD_ILI9486_ReadID},
  {0x9481, 320, 480, 12000000, LCD_ILI9481_Init, LCD_ILI9481_On, LCD_ILI9481_Off, LCD_ILI9481_ReadID},
  {0x9488, 320, 480, 12000000, LCD_ILI9488_Init, LCD_ILI9488_On, LCD_ILI9488_Off, LCD_ILI9488_ReadID},
  {0x7796, 320, 480, 13500000, LCD_ST7796S_Init, LCD_ST7796S_On, LCD_ST7796S_Off, LCD_ST7796S_ReadID},
};

void LCD_Init_LTDC(unsigned int dotClock)
{
  LTDC_InitTypeDef LTDC_InitStruct;

  /* Configure PLLSAI prescalers for LCD */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAI_N = 192 Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLL_LTDC = 192/3 = 64 Mhz */
  /* LTDC clock frequency = PLLLCDCLK / RCC_PLLSAIDivR = 64/4 = 16 Mhz */

  dotClock <<= 2;
  dotClock /= 1000000;

  int Register = RCC->PLLSAICFGR;
  Register &= ~(0x07UL << 28);
  Register |= (0x02UL << 28);
  Register &= 0xffff803f;
  Register |= (dotClock << 6);
  RCC->PLLSAICFGR = Register;

  Register = RCC->DCKCFGR;
  Register &= ~(0x03UL << 16);
  Register |= 0x01UL << 16;
  RCC->DCKCFGR = Register;

  //RCC_PLLSAIConfig(192 * 2 / 3, 6, 3);
  //RCC_LTDCCLKDivConfig (RCC_PLLSAIDivR_Div4);

  /* Enable PLLSAI Clock */
  RCC_PLLSAICmd(ENABLE);

  /* Wait for PLLSAI activation */
  while (RCC_GetFlagStatus(RCC_FLAG_PLLSAIRDY) == RESET)
    ;

  /* LTDC Configuration *********************************************************/
  /* Polarity configuration */
  /* Initialize the horizontal synchronization polarity as active low */
  LTDC_InitStruct.LTDC_HSPolarity = LTDC_HSPolarity_AL;
  /* Initialize the vertical synchronization polarity as active low */
  LTDC_InitStruct.LTDC_VSPolarity = LTDC_VSPolarity_AL;
  /* Initialize the data enable polarity as active low */
  LTDC_InitStruct.LTDC_DEPolarity = LTDC_DEPolarity_AL;
  /* Initialize the pixel clock polarity as input pixel clock */
  LTDC_InitStruct.LTDC_PCPolarity = LTDC_PCPolarity_IPC;

  /* Configure R,G,B component values for LCD background color */
  LTDC_InitStruct.LTDC_BackgroundRedValue = 0;
  LTDC_InitStruct.LTDC_BackgroundGreenValue = 0;
  LTDC_InitStruct.LTDC_BackgroundBlueValue = 0;

  /* Configure horizontal synchronization width */
  LTDC_InitStruct.LTDC_HorizontalSync = HSW;
  /* Configure vertical synchronization height */
  LTDC_InitStruct.LTDC_VerticalSync = VSH;
  /* Configure accumulated horizontal back porch */
  LTDC_InitStruct.LTDC_AccumulatedHBP = HBP;
  /* Configure accumulated vertical back porch */
  LTDC_InitStruct.LTDC_AccumulatedVBP = VBP;
  /* Configure accumulated active width */
  LTDC_InitStruct.LTDC_AccumulatedActiveW = LCD_W + HBP;
  /* Configure accumulated active height */
  LTDC_InitStruct.LTDC_AccumulatedActiveH = LCD_H + VBP;
  /* Configure total width */
  LTDC_InitStruct.LTDC_TotalWidth = LCD_W + HBP + HFP;
  /* Configure total height */
  LTDC_InitStruct.LTDC_TotalHeigh = LCD_H + VBP + VFP;

  LTDC_Init(&LTDC_InitStruct);

#if 0
  LTDC_ITConfig(LTDC_IER_LIE, ENABLE);
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = LTDC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = LTDC_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; /* Not used as 4 bits are used for the pr     e-emption priority. */;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( &NVIC_InitStructure );

  DMA2D_ITConfig(DMA2D_CR_TCIE, ENABLE);
  NVIC_InitStructure.NVIC_IRQChannel = DMA2D_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = DMA_SCREEN_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; /* Not used as 4 bits are used for the pr     e-emption priority. */;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( &NVIC_InitStructure );

  DMA2D->IFCR = (unsigned long)DMA2D_IFSR_CTCIF;
#endif
}

void LCD_LayerInit()
{
  LTDC_Layer_InitTypeDef LTDC_Layer_InitStruct;

  /* Windowing configuration */
  /* In this case all the active display area is used to display a picture then :
   Horizontal start = horizontal synchronization + Horizontal back porch = 30
   Horizontal stop = Horizontal start + window width -1 = 30 + 240 -1
   Vertical start   = vertical synchronization + vertical back porch     = 4
   Vertical stop   = Vertical start + window height -1  = 4 + 320 -1      */
  LTDC_Layer_InitStruct.LTDC_HorizontalStart = HBP + 1;
  LTDC_Layer_InitStruct.LTDC_HorizontalStop = (LCD_W + HBP);
  LTDC_Layer_InitStruct.LTDC_VerticalStart = VBP + 1;
  LTDC_Layer_InitStruct.LTDC_VerticalStop = (LCD_H + VBP);

  /* Pixel Format configuration*/
  LTDC_Layer_InitStruct.LTDC_PixelFormat = LTDC_Pixelformat_RGB565;
  /* Alpha constant (255 totally opaque) */
  LTDC_Layer_InitStruct.LTDC_ConstantAlpha = 255;
  /* Default Color configuration (configure A,R,G,B component values) */
  LTDC_Layer_InitStruct.LTDC_DefaultColorBlue = 0;
  LTDC_Layer_InitStruct.LTDC_DefaultColorGreen = 0;
  LTDC_Layer_InitStruct.LTDC_DefaultColorRed = 0;
  LTDC_Layer_InitStruct.LTDC_DefaultColorAlpha = 0;

  /* Configure blending factors */
  LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_CA;
  LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_CA;

  /* the length of one line of pixels in bytes + 3 then :
   Line Lenth = Active high width x number of bytes per pixel + 3
   Active high width         = LCD_W
   number of bytes per pixel = 2    (pixel_format : RGB565)
   */
  LTDC_Layer_InitStruct.LTDC_CFBLineLength = ((LCD_W * 2) + 3);
  /* the pitch is the increment from the start of one line of pixels to the
   start of the next line in bytes, then :
   Pitch = Active high width x number of bytes per pixel */
  LTDC_Layer_InitStruct.LTDC_CFBPitch = (LCD_W * 2);

  /* Configure the number of lines */
  LTDC_Layer_InitStruct.LTDC_CFBLineNumber = LCD_H;

  /* Start Address configuration : the LCD Frame buffer is defined on SDRAM w/ Offset */
  LTDC_Layer_InitStruct.LTDC_CFBStartAdress = (uint32_t)LCD_FIRST_FRAME_BUFFER;

  /* Initialize LTDC layer 1 */
  LTDC_LayerInit(LTDC_Layer1, &LTDC_Layer_InitStruct);

  /* Configure Layer 2 */
  LTDC_Layer_InitStruct.LTDC_BlendingFactor_1 = LTDC_BlendingFactor1_PAxCA;
  LTDC_Layer_InitStruct.LTDC_BlendingFactor_2 = LTDC_BlendingFactor2_PAxCA;

  /* Start Address configuration : the LCD Frame buffer is defined on SDRAM w/ Offset */
  LTDC_Layer_InitStruct.LTDC_CFBStartAdress = (uint32_t)LCD_SECOND_FRAME_BUFFER;

  /* Initialize LTDC layer 2 */
  LTDC_LayerInit(LTDC_Layer2, &LTDC_Layer_InitStruct);

  /* LTDC configuration reload */
  LTDC_ReloadConfig(LTDC_IMReload);

  LTDC_LayerCmd(LTDC_Layer1, ENABLE);
  LTDC_LayerCmd(LTDC_Layer2, ENABLE);

  LTDC_ReloadConfig(LTDC_IMReload);

  /* dithering activation */
  LTDC_DitherCmd(ENABLE);
}

BitmapBuffer lcdBuffer1(BMP_RGB565, LCD_W, LCD_H, (uint16_t *)LCD_FIRST_FRAME_BUFFER);
BitmapBuffer lcdBuffer2(BMP_RGB565, LCD_W, LCD_H, (uint16_t *)LCD_SECOND_FRAME_BUFFER);
BitmapBuffer *lcd = &lcdBuffer1;

void LCD_SetLayer(uint32_t layer)
{
  if (layer == LCD_FIRST_LAYER)
  {
    lcd = &lcdBuffer1;
  }
  else
  {
    lcd = &lcdBuffer2;
  }
  CurrentLayer = layer;
}

void LCD_SetTransparency(uint8_t transparency)
{
  if (CurrentLayer == LCD_FIRST_LAYER)
  {
    LTDC_LayerAlpha(LTDC_Layer1, transparency);
  }
  else
  {
    LTDC_LayerAlpha(LTDC_Layer2, transparency);
  }
  LTDC_ReloadConfig(LTDC_IMReload);
}

void lcdInit(void)
{
  /* Configure the LCD SPI+RESET pins */
  lcdSpiConfig();
  /* Configure the LCD Control pins */
  LCD_AF_GPIOConfig();

  unsigned index = 0;
  unsigned size = sizeof(LCD_Devices) / sizeof(LCD_Devices[0]);

  const STRUCT_LCD_DRIVER *LCD_Device = nullptr;
  for (index = 0; index < size; index++)
  {
    lcdReset();
    if (LCD_Devices[index].LCD_ReadID)
    {
      if (LCD_Devices[index].LCD_ReadID() == LCD_Devices[index].ID)
      {
        LCD_Device = &LCD_Devices[index];
        TRACE("LCD ID %X", LCD_Devices[index].ID);
        break;
      }
    }
  }
  if (!LCD_Device)
    LCD_Device = &LCD_Devices[size - 1];

  lcdOffFunction = LCD_Device->LCD_Off;
  lcdOnFunction = LCD_Device->LCD_On;

  lcdReset();
  if (LCD_Device->LCD_Init)
    LCD_Device->LCD_Init();

  LCD_Init_LTDC(LCD_Device->DotClock);

  LCD_LayerInit();

  /* Enable LCD display */
  LTDC_Cmd(ENABLE);

  /* Set Background layer */
  LCD_SetLayer(LCD_FIRST_LAYER);
  // lcdClear();
  LCD_SetTransparency(0);

  /* Set Foreground layer */
  LCD_SetLayer(LCD_SECOND_LAYER);
  lcd->clear();
  LCD_SetTransparency(255);

  LCD_Device->LCD_On();
}

void DMAFillRect(uint16_t *dest, uint16_t destw, uint16_t desth, uint16_t x,
                 uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  DMA2D_DeInit();

  DMA2D_InitTypeDef DMA2D_InitStruct;
  DMA2D_InitStruct.DMA2D_Mode = DMA2D_R2M;
  DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
  DMA2D_InitStruct.DMA2D_OutputGreen = (0x07E0 & color) >> 5;
  DMA2D_InitStruct.DMA2D_OutputBlue = 0x001F & color;
  DMA2D_InitStruct.DMA2D_OutputRed = (0xF800 & color) >> 11;
  DMA2D_InitStruct.DMA2D_OutputAlpha = 0x0F;
  DMA2D_InitStruct.DMA2D_OutputMemoryAdd = CONVERT_PTR_UINT(dest) + 2 * (destw * y + x);
  DMA2D_InitStruct.DMA2D_OutputOffset = (destw - w);
  DMA2D_InitStruct.DMA2D_NumberOfLine = h;
  DMA2D_InitStruct.DMA2D_PixelPerLine = w;
  DMA2D_Init(&DMA2D_InitStruct);

  /* Start Transfer */
  DMA2D_StartTransfer();

  /* Wait for CTC Flag activation */
  while (DMA2D_GetFlagStatus(DMA2D_FLAG_TC) == RESET)
    ;
}

void DMACopyBitmap(uint16_t *dest, uint16_t destw, uint16_t desth, uint16_t x,
                   uint16_t y, const uint16_t *src, uint16_t srcw, uint16_t srch,
                   uint16_t srcx, uint16_t srcy, uint16_t w, uint16_t h)
{
  DMA2D_DeInit();

  DMA2D_InitTypeDef DMA2D_InitStruct;
  DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M;
  DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
  DMA2D_InitStruct.DMA2D_OutputMemoryAdd = CONVERT_PTR_UINT(
      dest + y * destw + x);
  DMA2D_InitStruct.DMA2D_OutputGreen = 0;
  DMA2D_InitStruct.DMA2D_OutputBlue = 0;
  DMA2D_InitStruct.DMA2D_OutputRed = 0;
  DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
  DMA2D_InitStruct.DMA2D_OutputOffset = destw - w;
  DMA2D_InitStruct.DMA2D_NumberOfLine = h;
  DMA2D_InitStruct.DMA2D_PixelPerLine = w;
  DMA2D_Init(&DMA2D_InitStruct);

  DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;
  DMA2D_FG_StructInit(&DMA2D_FG_InitStruct);
  DMA2D_FG_InitStruct.DMA2D_FGMA = CONVERT_PTR_UINT(src + srcy * srcw + srcx);
  DMA2D_FG_InitStruct.DMA2D_FGO = srcw - w;
  DMA2D_FG_InitStruct.DMA2D_FGCM = CM_RGB565;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0;
  DMA2D_FGConfig(&DMA2D_FG_InitStruct);

  /* Start Transfer */
  DMA2D_StartTransfer();

  /* Wait for CTC Flag activation */
  while (DMA2D_GetFlagStatus(DMA2D_FLAG_TC) == RESET)
    ;
}

void DMACopyAlphaBitmap(uint16_t *dest, uint16_t destw, uint16_t desth,
                        uint16_t x, uint16_t y, const uint16_t *src, uint16_t srcw, uint16_t srch,
                        uint16_t srcx, uint16_t srcy, uint16_t w, uint16_t h)
{
  DMA2D_DeInit();

  DMA2D_InitTypeDef DMA2D_InitStruct;
  DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M_BLEND;
  DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
  DMA2D_InitStruct.DMA2D_OutputMemoryAdd = CONVERT_PTR_UINT(
      dest + y * destw + x);
  DMA2D_InitStruct.DMA2D_OutputGreen = 0;
  DMA2D_InitStruct.DMA2D_OutputBlue = 0;
  DMA2D_InitStruct.DMA2D_OutputRed = 0;
  DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
  DMA2D_InitStruct.DMA2D_OutputOffset = destw - w;
  DMA2D_InitStruct.DMA2D_NumberOfLine = h;
  DMA2D_InitStruct.DMA2D_PixelPerLine = w;
  DMA2D_Init(&DMA2D_InitStruct);

  DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;
  DMA2D_FG_StructInit(&DMA2D_FG_InitStruct);
  DMA2D_FG_InitStruct.DMA2D_FGMA = CONVERT_PTR_UINT(src + srcy * srcw + srcx);
  DMA2D_FG_InitStruct.DMA2D_FGO = srcw - w;
  DMA2D_FG_InitStruct.DMA2D_FGCM = CM_ARGB4444;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0;
  DMA2D_FGConfig(&DMA2D_FG_InitStruct);

  DMA2D_BG_InitTypeDef DMA2D_BG_InitStruct;
  DMA2D_BG_StructInit(&DMA2D_BG_InitStruct);
  DMA2D_BG_InitStruct.DMA2D_BGMA = CONVERT_PTR_UINT(dest + y * destw + x);
  DMA2D_BG_InitStruct.DMA2D_BGO = destw - w;
  DMA2D_BG_InitStruct.DMA2D_BGCM = CM_RGB565;
  DMA2D_BG_InitStruct.DMA2D_BGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
  DMA2D_BG_InitStruct.DMA2D_BGPFC_ALPHA_VALUE = 0;
  DMA2D_BGConfig(&DMA2D_BG_InitStruct);

  /* Start Transfer */
  DMA2D_StartTransfer();

  /* Wait for CTC Flag activation */
  while (DMA2D_GetFlagStatus(DMA2D_FLAG_TC) == RESET)
    ;
}

void DMABitmapConvert(uint16_t *dest, const uint8_t *src, uint16_t w,
                      uint16_t h, uint32_t format)
{
  DMA2D_DeInit();

  DMA2D_InitTypeDef DMA2D_InitStruct;
  DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M_PFC;
  DMA2D_InitStruct.DMA2D_CMode = format;
  DMA2D_InitStruct.DMA2D_OutputMemoryAdd = CONVERT_PTR_UINT(dest);
  DMA2D_InitStruct.DMA2D_OutputGreen = 0;
  DMA2D_InitStruct.DMA2D_OutputBlue = 0;
  DMA2D_InitStruct.DMA2D_OutputRed = 0;
  DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
  DMA2D_InitStruct.DMA2D_OutputOffset = 0;
  DMA2D_InitStruct.DMA2D_NumberOfLine = h;
  DMA2D_InitStruct.DMA2D_PixelPerLine = w;
  DMA2D_Init(&DMA2D_InitStruct);

  DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;
  DMA2D_FG_StructInit(&DMA2D_FG_InitStruct);
  DMA2D_FG_InitStruct.DMA2D_FGMA = CONVERT_PTR_UINT(src);
  DMA2D_FG_InitStruct.DMA2D_FGO = 0;
  DMA2D_FG_InitStruct.DMA2D_FGCM = CM_ARGB8888;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = REPLACE_ALPHA_VALUE;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0;
  DMA2D_FGConfig(&DMA2D_FG_InitStruct);

  /* Start Transfer */
  DMA2D_StartTransfer();

  /* Wait for CTC Flag activation */
  while (DMA2D_GetFlagStatus(DMA2D_FLAG_TC) == RESET)
    ;
}

void DMACopy(void *src, void *dest, unsigned len)
{
  DMA2D_DeInit();

  DMA2D_InitTypeDef DMA2D_InitStruct;
  DMA2D_InitStruct.DMA2D_Mode = DMA2D_M2M;
  DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;
  DMA2D_InitStruct.DMA2D_OutputMemoryAdd = CONVERT_PTR_UINT(dest);
  DMA2D_InitStruct.DMA2D_OutputGreen = 0;
  DMA2D_InitStruct.DMA2D_OutputBlue = 0;
  DMA2D_InitStruct.DMA2D_OutputRed = 0;
  DMA2D_InitStruct.DMA2D_OutputAlpha = 0;
  DMA2D_InitStruct.DMA2D_OutputOffset = 0;
  DMA2D_InitStruct.DMA2D_NumberOfLine = LCD_H;
  DMA2D_InitStruct.DMA2D_PixelPerLine = LCD_W;
  DMA2D_Init(&DMA2D_InitStruct);

  DMA2D_FG_InitTypeDef DMA2D_FG_InitStruct;
  DMA2D_FG_StructInit(&DMA2D_FG_InitStruct);
  DMA2D_FG_InitStruct.DMA2D_FGMA = CONVERT_PTR_UINT(src);
  DMA2D_FG_InitStruct.DMA2D_FGO = 0;
  DMA2D_FG_InitStruct.DMA2D_FGCM = CM_RGB565;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_MODE = NO_MODIF_ALPHA_VALUE;
  DMA2D_FG_InitStruct.DMA2D_FGPFC_ALPHA_VALUE = 0;
  DMA2D_FGConfig(&DMA2D_FG_InitStruct);

  /* Start Transfer */
  DMA2D_StartTransfer();

  /* Wait for CTC Flag activation */
  while (DMA2D_GetFlagStatus(DMA2D_FLAG_TC) == RESET)
    ;
}

void lcdStoreBackupBuffer()
{
  DMACopy(lcd->getData(), LCD_BACKUP_FRAME_BUFFER, DISPLAY_BUFFER_SIZE);
}

int lcdRestoreBackupBuffer()
{
  DMACopy(LCD_BACKUP_FRAME_BUFFER, lcd->getData(), DISPLAY_BUFFER_SIZE);
  return 1;
}

void lcdRefresh()
{
  if (CurrentLayer == LCD_FIRST_LAYER)
  {
    LTDC_LayerAlpha(LTDC_Layer1, 255);
    LTDC_LayerAlpha(LTDC_Layer2, 0);
  }
  else
  {
    LTDC_LayerAlpha(LTDC_Layer1, 0);
    LTDC_LayerAlpha(LTDC_Layer2, 255);
  }
  LTDC_ReloadConfig(LTDC_IMReload);
}

void lcdNextLayer()
{
  if (CurrentLayer == LCD_FIRST_LAYER)
  {
    LCD_SetLayer(LCD_SECOND_LAYER);
  }
  else
  {
    LCD_SetLayer(LCD_FIRST_LAYER);
  }
}
