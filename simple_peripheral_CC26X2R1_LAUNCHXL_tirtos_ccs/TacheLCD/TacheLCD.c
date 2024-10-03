/*
 * SPI.c
 *
 *  Created on: 3 oct. 2024
 *      Author: GEII Robot
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <TacheLCD/TacheLCD.h>

#define TACHELCD_TASK_PRIORITY 1
#define TACHELCD_TASK_STACK_SIZE 1024

Task_Struct TacheLCD;
uint8_t TacheLCDStack[TACHELCD_TASK_STACK_SIZE];
Semaphore_Struct semTacheLCDStruct;
Semaphore_Handle semTacheLCDHandle;

#include "TacheLCD/LCD_LAUNCHPAD/OLED_Display.h"
#include <TacheLCD/SPI_UTLN/SPIUTLN.h>
#include <TacheLCD/LCD_LAUNCHPAD/LCD_LAUNCHPAD.h>

float Ax,Ay,Az;

void LCD_Init(void)
{
//Initialize the LCD controller
    Initialize_LCD();
//Fill display with a given RGB value
    Fill_LCD(0xFF, 0x00, 0x00); //RGB
    Task_sleep(500000 / Clock_tickPeriod); // Delay 100ms
    char DataLCD[] = "UTLN";
    OLEDText22(40, 17, DataLCD, SIZE_TWO, 0x00, 0xFF, 0xFF);
    Task_sleep(1000000 / Clock_tickPeriod); // Delay 1s
}


void TacheLCD_init(void)
{
    SPI_UTLN_Init();
    LCD_Init();
}

void floatToString1d(char *ax, float AX)
{
    char convert[6];
//char* ax = " ";
    if (AX < 0)
    {
        char moins[6] = "-";
        strcat(ax, moins);
        AX = AX * -1.0f;
    }
    char point[6] = ".";
    long Axlong = (long) AX;
    long Axdeclong = (long) ((AX - (float) Axlong) * 10);
    ltoa(Axlong, convert, 10);
    strcat(ax, convert);
    strcat(ax, point);
    ltoa(Axdeclong, convert, 10);
    strcat(ax, convert);
}


void afficherDonnees(float accx, float accy, float accz){
    Ax = accx;
    Ay = accy;
    Az = accz;
    Semaphore_post(semTacheLCDHandle);

}


void intToString(char *ax, float AX)
{
    char convert[6];
//char* ax = " ";
    if (AX < 0)
    {
        char moins[6] = "-";
        strcat(ax, moins);
        AX = AX * -1.0f;
    }
    long Axlong = (long) AX;
    ltoa(Axlong, convert, 10);
    strcat(ax, convert);
}



extern void TacheLCD_init(void);

static void TacheLCD_taskFxn(UArg a0, UArg a1)
{

    TacheLCD_init();

    char DataLCD[] = "AX:";
    char DataLCD2[] = "AY:";
    char DataLCD3[] = "AZ:";

    //Fill display with a given RGB value

    Fill_LCD(0xFF, 0x00, 0x00); //RGB
    OLEDText22(8, 8, DataLCD, SIZE_TWO, 0xFF, 0xFF, 0x00);
    OLEDText22(8, 33, DataLCD2, SIZE_TWO, 0xFF, 0xFF, 0x00);
    OLEDText22(8, 58, DataLCD3, SIZE_TWO, 0xFF, 0xFF, 0x60);



    for (;;)
    {
        // Attendre que le sémaphore soit posté par la tâche ADC
        Semaphore_pend(semTacheLCDHandle, BIOS_WAIT_FOREVER);
        char axStr[10]=" ";
        char ayStr[10]=" ";
        char azStr[10]=" ";
        // Convertir les valeurs flottantes en chaînes de caractères
        floatToString1d(axStr, Ax);
        floatToString1d(ayStr, Ay);
        floatToString1d(azStr, Az);


        // Fill_Rectangle(x_position, y_position, width, height, background_color);

        // Afficher les nouvelles valeurs sur l'écran LCD
        OLEDText22(50, 8, axStr, SIZE_TWO, 0xFF, 0xFF, 0x00);
        OLEDText22(50, 33, ayStr, SIZE_TWO, 0xFF, 0xFF, 0x00);
        OLEDText22(50, 58, azStr, SIZE_TWO, 0xFF, 0xFF, 0x00);
    }
}

void TacheLCD_CreateTask(void)
{
    Semaphore_Params semParams;
    Task_Params taskParams;
    /* Configuration de la tache*/
    Task_Params_init(&taskParams);
    taskParams.stack = TacheLCDStack;
    taskParams.stackSize = TACHELCD_TASK_STACK_SIZE;
    taskParams.priority = TACHELCD_TASK_PRIORITY;
    /* Creation de la tache*/
    Task_construct(&TacheLCD, TacheLCD_taskFxn, &taskParams, NULL);
    /* Construire un objet semaphore
     pour etre utilise comme outil de
     verrouillage, comptage initial 0 */
    Semaphore_Params_init(&semParams);
    Semaphore_construct(&semTacheLCDStruct, 0, &semParams);
    /* Obtenir la gestion de l'instance */
    semTacheLCDHandle = Semaphore_handle(&semTacheLCDStruct);
}




