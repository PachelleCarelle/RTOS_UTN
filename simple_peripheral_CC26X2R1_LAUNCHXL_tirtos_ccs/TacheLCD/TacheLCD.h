/*
 * SPI.h
 *
 *  Created on: 3 oct. 2024
 *      Author: GEII Robot
 */




#ifndef TACHELCD_TACHELCD_H_
#define TACHELCD_TACHELCD_H_


void LCD_Init(void);
void floatToString(char* ax, float AX);
void floatToString1d(char* ax, float AX);
void intToString(char* ax, float AX);
void TacheLCD_CreateTask(void);
void afficherDonnees(float accx, float accy, float accz);


#endif /* TACHELCD_TACHELCD_H_ */
