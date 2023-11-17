/*
 * LCD_GFX.c
 *
 * Created: 9/20/2021 6:54:25 PM
 *  Author: You
 */ 

#include "LCD_GFX.h"
#include "ST7735.h"

/******************************************************************************
* Local Functions
******************************************************************************/



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddr(x,y,x,y);
	SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note			The point is the top left corner of the character (lowest x & y value it occupies).
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor){
	uint16_t row = character - 0x20;		//Determine row of ASCII table starting at space
	int i, j;
	if ((LCD_WIDTH-x>7)&&(LCD_HEIGHT-y>7)){
		for(i=0;i<5;i++){
			uint8_t pixels = ASCII[row][i]; //Go through the list of pixels
			for(j=0;j<8;j++){
				if ((pixels>>j)&1==1){
					LCD_drawPixel(x+i,y+j,fColor);
				}
				else {
					LCD_drawPixel(x+i,y+j,bColor);
				}
			}
		}
	}
}


/******************************************************************************
* LAB 4 TO DO. COMPLETE THE FUNCTIONS BELOW.
* You are free to create and add any additional files, libraries, and/or
*  helper function. All code must be authentically yours.
******************************************************************************/
uint8_t LCD_StayInX(short x){// function to keep x inside boundaries of LCD
	if (x > LCD_WIDTH){
		return LCD_WIDTH;
	}
	if (x < 0){
		return 0;
	}
	return x;
}

uint8_t LCD_StayInY(short y){// function to keep y inside boundaries of LCD
	if (y > LCD_HEIGHT){
		return LCD_HEIGHT;
	}
	if (y < 0){
		return 0;
	}
	return y;
}

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color)
{
	// Goes from the min to the max x value inside the circle and finds all the y-values 
	// That are in the circle using r^2 > x^2 + y^2 for points inside a circle.
	// Then transmits one column of y at a time for be written onto LED for faster transmission.
	uint16_t dx2; uint16_t dy2; uint8_t dy; // dy is y-y0 and dy2 is (y-y0)^2. 
	uint16_t r2 = radius * radius; // radius squared
	uint8_t smoother = (radius > 4) ? 0:1; // constant added to radius squared in comparison to smooth circle
	short xmin = x0-radius; short xmax = x0+radius; // opposite ends of the circle.
	for (short x = LCD_StayInX(xmin); x < LCD_StayInX(xmax)+1; x++){
		dx2 = (x-x0)*(x-x0);
		dy = 0; dy2 = 0; // Starting at y = y0, so dy=0. 
		while (smoother + r2 > dx2 + dy2){// Check if point (x, y0+dy) is inside the circle. 
			dy++; // could also do dy--, but dy2 will be the same. 
			dy2 = (dy)*(dy); 
		}
		// transmitting one column of the circle at a time instead of each pixel.
		LCD_setAddr(x,LCD_StayInY(y0-dy),x,LCD_StayInY(y0+dy)); 
		clear(LCD_PORT, LCD_TFT_CS);	//CS pulled low to start communication
		for (uint8_t i = 0; i < 2*dy; i++) {
			SPI_ControllerTx_16bit_stream(color); // Write color to current pixel and increment pointer
		}
		set(LCD_PORT, LCD_TFT_CS);	//set CS to high to end this command stream.
	}
}

void LCD_drawCircleFast(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color,uint16_t bg_color){
	// Goes from the min to the max x value inside the circle and finds all the y-values
	// That are in the circle using r^2 > x^2 + y^2 for points inside a circle.
	// Then transmits entire circle as a block. 
	uint16_t dx2; uint16_t dy2; uint8_t dy; // dy is y-y0 and dy2 is (y-y0)^2.
	uint16_t r2 = radius * radius; // radius squared
	uint8_t smoother = (radius > 4) ? 0:1; // constant added to radius squared in comparison to smooth circle
	short xmin = x0-radius-1; short xmax = x0+radius+1; // opposite ends of the circle.
	short ymin = y0-radius-1; short ymax = y0+radius+1; // opposite ends of the circle.
	// Adding 1 to the min and maxes so that there will be a bg_color outline around circle, cleaning up as it moves
	LCD_setAddr(LCD_StayInX(xmin),LCD_StayInY(ymin),LCD_StayInX(xmax),LCD_StayInY(ymax));
	clear(LCD_PORT, LCD_TFT_CS);	//CS pulled low to start communication
	for (short x = LCD_StayInX(xmin); x < LCD_StayInX(xmax)+1; x++){
		dx2 = (x-x0)*(x-x0);
		for (short y = LCD_StayInY(ymin); y < LCD_StayInY(ymax)+1; y++){
			dy2 = (y-y0)*(y-y0); // Starting at y = y0, so dy=0.
			if (smoother + r2 > dx2 + dy2){// Check if point is inside the circle.
				SPI_ControllerTx_16bit_stream(color);
			}
			else {
				SPI_ControllerTx_16bit_stream(bg_color); // Write color to current pixel and increment pointer
			}
		}
	}
	set(LCD_PORT, LCD_TFT_CS);	//set CS to high to end this command stream.
}


/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/
void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t color) // shorts are signed 16 bit integers.
{
	// TODO: Test it out
	short D; short x; short y;
	short xs = (x1 > x0) ?  1 : -1; // x sign.
	short dx = (x1 - x0)*xs; // absolute value of change in x. 
	short ys = (y1 > y0) ?  1 : -1; // y sign
	short dy = (y1 - y0)*ys; // absolute value of change in y.
	if (dx > dy){// y as function of x
		y = y0;
		D = 2*dy - dx; // difference between F(x,y) from F(x+1,y+0.5)
		for (x = x0; x != LCD_StayInX(x1) + xs; x+=xs){
			LCD_drawPixel(x,y,color);
			if (D >= 0){
				y += ys; // Y value changes
				D += -2*dx;
			}
			D += 2*dy;
		}
	}
	else { // x as function of y.
		x = x0;
		D = 2*dx - dy; // difference between F(x,y) from F(x+0.5,y+1)
		for (y = y0; y != LCD_StayInY(y1) + ys; y+=ys){
			LCD_drawPixel(x,y,color);
			if (D >= 0){
				x += xs; // X value changes
				D += -2*dy;
			}
			D += 2*dx;
		}
	}
}



/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
{
	uint16_t i; uint16_t dx; uint16_t dy;
	dx = x1 - x0; dy = y1 - y0; 
	LCD_setAddr(x0, y0, x1, y1); // set the range of pixel addresses to write commands onto
	clear(LCD_PORT, LCD_TFT_CS);	//CS pulled low to start communication
	for (i = 0; i < (dx+1)*(dy+1); i++) { // Add 1 since for loops are non-inclusive of endpoints.
		SPI_ControllerTx_16bit_stream(color); // Write color to current pixel and increment pointer
	}
	set(LCD_PORT, LCD_TFT_CS);	//set CS to high to end this command stream. 
}

/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color) 
{
	LCD_setAddr(0, 0, LCD_WIDTH, LCD_HEIGHT); // set the range of pixel addresses to write commands onto
	clear(LCD_PORT, LCD_TFT_CS);	//CS pulled low to start communication
	for (uint16_t i = 0; i < LCD_SIZE; i++) {
		SPI_ControllerTx_16bit_stream(color); // Write color to current pixel and increment pointer
	}
	set(LCD_PORT, LCD_TFT_CS);	//set CS to high to end this command stream. 
}

/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note			Assumes each ASCII character is 5 pixels in width.
*****************************************************************************/
void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
{
	uint8_t xSpacing = 1; // number of empty pixels between chars
	uint8_t ySpacing = 1; // number of empty pixels between lines
	uint8_t x0 = x; // keep track of original x
	LCD_drawBlock(x-xSpacing, y,x-1,y+ASCII_HEIGHT-1,bg);
	while(*str) {//iterate through the pointer from its start until its null character
		if (*str == '\n'){ // make new line.
			LCD_drawBlock(x0-1,y + ASCII_HEIGHT,x-1,y+ASCII_HEIGHT+ySpacing-1,bg);
			y += ASCII_HEIGHT + ySpacing; 
			x=x0; *str++;
			LCD_drawBlock(x-1, y,x+xSpacing-1,y+ASCII_HEIGHT-1,bg);
			}
		LCD_drawChar(x, y,*str++,fg,bg);
		LCD_drawBlock(x+ASCII_WIDTH, y,x+ASCII_WIDTH+xSpacing-1,y+ASCII_HEIGHT-1,bg);
		x += ASCII_WIDTH + xSpacing;
	}
}
