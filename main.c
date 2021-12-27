/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

#define and &&
#define or ||

#define variable Variables_1.Variable_1

/***** Matriz de Calibracion *****/
                                        //30    //45
uint16 ADC_1_Eje_X[91] = {1490,	1501,	1514,	1523,	1535,	1548,	1560,	1569,	1579,	1591,	1608,	// 10
1619,	1634,	1640,	1648,	1662,	1670,	1679,	1692,	1705,	1711,	1725,	1736,	1751,	1760,   // 24	
1771,	1778,	1788,	1798,	1806,	1820,	1832,	1838,	1850,	1859,	1867,	1879,	1888,	1897,	// 38
1904,	1915,	1921,	1937,	1942,	1950,	1958,	1976,	1980,	1985,	1991,	2000,	2006,	2017,	// 52
2020,	2022,	2030,	2043,	2047,	2053,	2061,	2067,	2070,	2074,	2077,	2085,	2090,	2098,   	
2103,	2105,	2107,	2109,	2120,	2116,	2131,	2128,	2122,	2127,	2134,	2135,	2131,	2139, 
2136,	2138,	2138,	2140,	2138,	2144,	2148,	2143,	2149,	2148};
                                                        //46
uint16 ADC_2_Eje_Z[91] = {1886,	1892,	1885,	1887,	1889,	1891,	1882,	1886,	1885,	1885,	1884,   // 10	
1875,	1874,	1873,	1873,	1870,	1869,	1867,	1865,	1856,	1851,	1848,	1843,	1832,	1838,   // 24	
1833,	1831,	1829,	1818,	1816,	1793,	1790,	1787,	1784,	1781,	1775,	1771,	1761,	1760,   // 38	
1751,	1739,	1731,	1722,	1715,	1713,	1708,	1689,	1675,	1668,	1659,	1651,	1645,	1633,	// 52
1623,	1616,	1605,	1595,	1581,	1572,	1563,	1557,	1542,	1530,	1519,	1508,	1493,	1484,	// 66
1473,	1460,	1448,	1441,	1431,	1420,	1408,	1396,	1386,	1371,	1360,	1346,	1336,	1325,	// 80
1311,	1298,	1290,	1281,	1264,	1256,	1241,	1223,	1215,	1100};          //78
                        //70                            //60

/**** Variables y banceras para el control del Sistema ****/
typedef struct Tiempo{                                                          // Estructura con las variables de tiempo
    uint16 ms:10;                                                               // que permiten controlar freuencia de muestreo
    uint16 seg:3;
}Tiempo;
Tiempo tiempo;

typedef struct Medidas{        
    uint32 acumulado_ADC_1:20;
    uint32 acumulado_ADC_2:20;   
    
    uint32 ADC_1:16;
    uint32 ADC_2:16;  
    
    uint32 angulo:7;
}medidas;
medidas medida;

typedef union Banderas_1{                                                      
    struct Variables1{                  
        uint16 contador:7;
    }Variable_1;
}banderas_1;
banderas_1 Variables_1;

/**** Rutinas de funcionamiento ****/
void sensar(void);
void calcular_angulo(unsigned char bandera_angulo);
void imprimir(void);

CY_ISR_PROTO(cronometro);

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    isr_counter_StartEx(cronometro);
    
    LCD_Start();
    ADC_Start();
    Counter_Start();
    AMux_Start();

    for(;;)
    {
        
        if (tiempo.ms%25 == 0) sensar();                                        // Sensar 40 veces por segundo
        if (tiempo.ms%500 == 0) imprimir();                                     // Imprime 2 veces por segundo
    }
}

void imprimir(void){                                                            // Interfaz en el LCD
    LCD_ClearDisplay();
    
    LCD_Position(0,0);                                                          // Medida del Eje X
    LCD_PrintString("ADC 1-X: ");
    LCD_PrintNumber(medida.ADC_1);
    
    LCD_Position(1,0);                                                          // Medida del Eje Z
    LCD_PrintString("ADC 2-Z: ");
    LCD_PrintNumber(medida.ADC_2);
    
    LCD_Position(2,0);                                                          // Angulo calculado
    LCD_PrintString("Angulo: ");
    LCD_PrintNumber(medida.angulo);
}

void sensar(void){
    variable.contador++;                                                        // Contador hasta 20
    
    AMux_Select(0);                                                             // Sensado del Eje X
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    medida.acumulado_ADC_1 += ADC_GetResult16();
    
    AMux_Select(1);                                                             // Sensado del Eje Z
    ADC_StartConvert();
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    medida.acumulado_ADC_2 += ADC_GetResult16();
    
    
    if (variable.contador == 20){                                               
        variable.contador = 0;
        
        medida.ADC_1 = medida.acumulado_ADC_1 /= 20;                            // Promedio de las lecturas de cada eje
        medida.ADC_2 = medida.acumulado_ADC_2 /= 20;
        
        medida.acumulado_ADC_1 = 0;                                             // Reset de las variables acumuladas
        medida.acumulado_ADC_2 = 0;
        
        if (medida.ADC_1 <= 1958) calcular_angulo('x');                         // Con el eje X se calcula entre 0° y 45°
        else calcular_angulo('z');                                              // Con el eje Z se calcula entre 45° y 90°
        
    }
}

void calcular_angulo(unsigned char bandera_angulo){                             // Calculo de angulo mediante las lecturas del ADC de cada eje
    if (bandera_angulo == 'x'){
        for (variable.contador = 0; variable.contador < 92; variable.contador++){
            if (ADC_1_Eje_X[variable.contador] >= medida.ADC_1){                // Sí el valor del vector de calibración en la posicion 
                medida.angulo = variable.contador;                              // del contador es mayor a la medida del ADC_1
                variable.contador = 0;                                          // se asigna el valor de la posicion al angulo
                return;                                                         // y se termina el ciclo "for"
            }
        }
    }
    else if (bandera_angulo == 'z'){                                            // Sí el valor del vector de calibración en la posicion
        for (variable.contador = 0; variable.contador < 92; variable.contador++){// del contador es menor a la medida del ADC_2
            if (ADC_2_Eje_Z[variable.contador] <= medida.ADC_2){                // se asigna el valor de la posicion al angulo
                medida.angulo = variable.contador;                              // y se termina el ciclo "for"
                variable.contador = 0;
                return;
            }
        }
    }
}

CY_ISR(cronometro){                                                             // Interrupcion del Cronometro
    tiempo.ms++;      
    if (tiempo.ms == 1000) {
        tiempo.ms = 0;
        tiempo.seg++;
    }
}

/* [] END OF FILE */
