/*
 * TacheADC.c
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
#include <TacheADC/TacheADC.h>
#include <ti_drivers_config.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/ADC.h>
#include <TacheLCD/TacheLCD.h>

#define ADC_SAMPLE_COUNT (10)

#define TACHEADC_TASK_PRIORITY 1
#define TACHEADC_TASK_STACK_SIZE 1024

Task_Struct TacheADC;
uint8_t TacheADCStack[TACHEADC_TASK_STACK_SIZE];
Semaphore_Struct semTacheADCStruct;
Semaphore_Handle semTacheADCHandle;

uint16_t adcValue0;
uint32_t adcValue0MicroVolt;
uint16_t adcValue1[ADC_SAMPLE_COUNT];
uint32_t adcValue1MicroVolt[ADC_SAMPLE_COUNT];

static Clock_Struct myClock;

extern void TacheADC_init(void);
float Sampling(uint_least8_t Board_ADC_Number);

void myClockSwiFxn(uintptr_t arg0)
{
    Semaphore_post(semTacheADCHandle);
}

uint16_t i;
ADC_Handle adc;
ADC_Params params;
int_fast16_t res;

float gx, gy, gz;  // Variables pour stocker les moyennes des axes
float ADC_g[ADC_SAMPLE_COUNT];

void TacheADC_init(void){
GPIO_init();
ADC_init();
ADC_Params_init(&params);
Clock_Params clockParams;
Clock_Params_init(&clockParams);
clockParams.period = 100 * (1000/Clock_tickPeriod),//100ms
Clock_construct(&myClock, myClockSwiFxn,0, // Initial delay before first timeout
                &clockParams);
Clock_start(Clock_handle(&myClock));//Timer start
}

static void TacheADC_taskFxn(UArg a0, UArg a1)
{
    TacheADC_init();
    for(;;)
    {
        Semaphore_pend(semTacheADCHandle, BIOS_WAIT_FOREVER);
        // Récupérer les moyennes des axes
        gx = Sampling(CONFIG_ADC_0);  // Axe X
        gy = Sampling(CONFIG_ADC_1);  // Axe Y
        gz = Sampling(CONFIG_ADC_2);  // Axe Z

        afficherDonnees(gx, gy, gz);
        //AFFICHER
    }
}

void TacheADC_CreateTask(void){
    Semaphore_Params semParams;
    Task_Params taskParams;
    /* Configuration de la tache*/
    Task_Params_init(&taskParams);
    taskParams.stack = TacheADCStack;
    taskParams.stackSize = TACHEADC_TASK_STACK_SIZE;
    taskParams.priority = TACHEADC_TASK_PRIORITY;

    /* Creation de la tache*/
    Task_construct(&TacheADC, TacheADC_taskFxn,
                   &taskParams, NULL);

    /* Construire un objet semaphore
    pour etre utilise comme outil de
    verrouillage, comptage initial 0 */
    Semaphore_Params_init(&semParams);
    Semaphore_construct(&semTacheADCStruct,
    0, &semParams);

    /* Obtenir la gestion de l'instance */
    semTacheADCHandle = Semaphore_handle(&semTacheADCStruct);
}


float Sampling(uint_least8_t Board_ADC_Number){

    float sum = 0;  // Somme des échantillons
    float average = 0; // Moyenne des échantillons

    adc = ADC_open(Board_ADC_Number, &params);
    if (adc == NULL){
        while (1);
    }
    for (i = 0; i < ADC_SAMPLE_COUNT; i++){
        res = ADC_convert(adc, &adcValue1[i]);
        if (res == ADC_STATUS_SUCCESS){
            adcValue1MicroVolt[i] =
            ADC_convertRawToMicroVolts(adc,
                   adcValue1[i]);
            ADC_g[i] = ((float)adcValue1MicroVolt[i]-1650000)/660000;
            sum += ADC_g[i];  // Ajouter la valeur à la somme
        }
    }

    average = (float)sum / ADC_SAMPLE_COUNT;  // Calcul de la moyenne
    //average /= 1000000.0;  // Convertir en volts (de microvolts à volts)
    ADC_close(adc);
    return average;  // Retourner la moyenne
}











