#include <stdint.h>
#include "cmsis_gcc.h"
#include "virtual_timer.h"
#include "reactor.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#include "pmc.h"
#include "dac.h"

#define NUM_MAX_PICOS 50 //NUM_MAX_PICOS: número de picos que são lidos antes de o travamento ser realizado.}*)
#define NUM_MEDIDAS 40

unsigned int ler_medida[NUM_MEDIDAS], ponto_medida[NUM_MEDIDAS ];//lerMedida[40]: medidas lidas pelo pinMedicao. nMedida[40]: o ponto da rampa correspondente a cada medida tirada.
unsigned int ponto_pico[NUM_MAX_PICOS],num_de_dicos = 0;
unsigned int media = 0, media_translacao = 0, pico_atual = 0;

/*
 * Callback to blink the led to show the app is active
 */

const uint16_t amostras = 250;

unsigned int contador_medidas = 0; // Conta o numero de medidas realizadas.
uint16_t  rampa = 0;
uint16_t min_rampa = 2000 - (amostras/2), max_ramp = 2000 + (amostras/2), picos_iniciais = 0,media_picos;
uint16_t pulsoAtual = 0, pulsoAnterior = 0;
unsigned int pin_sinc = 4;//sincronização com o circuito de Higor

#define ADC_BUFFER_SIZE 1024
uint16_t adc_buffer[ADC_BUFFER_SIZE];

uint16_t ctr = 0;

uint16_t data;

uint16_t g_posicaoCursor = 2048;

#define POS 1
#define NEG -1
int LARG_CURSOR = 30;
void rampa_externa(hcos_word_t arg);
void rampa_interna(hcos_word_t arg);




uint16_t checa_pulso_descida(){
    static uint16_t pulso_anterior = 0;
    int k = 0;
    while(k < 3){
        k++;
    }
    uint16_t pulso_atual = gpio_read_pin(GPIOA, 29);
    if((pulso_anterior != pulso_atual) && pulso_atual == 0){
        pulso_anterior = pulso_atual;
        return 1;
    }
    pulso_anterior = pulso_atual;
    return 0;
}

void analisa_maximo(){//função para analisar o maximo valor do pico}*)
  unsigned int auxPico = 0, picoAtual = 0,ponto_pico[10], nMedida[0];
  
  for(int u=0;u<=39;u++){
    if(auxPico<ler_medida[u]){//pega o maior valor dentro destes dados
      auxPico=ler_medida[u];
      ponto_pico[picoAtual] = ponto_medida[u];
    }
  }
  
  for(int u=0; u<=39;u++)
        ler_medida[u]=0;
  
}

#define MAX_TICKS 0
#define MAX_TICKS_EXT 100


void rampa_interna(hcos_word_t arg){
    uint16_t i = min_rampa, retardador = 0;
    dac_set(DAC_CH0);
    do {
        
        if(checa_pulso_descida()){
            rampa = 1;
        }
        if(rampa == 1){
            do{
                if(retardador++ == 15){
                    do{
                    }while(!(DAC->ISR & 0x1));
                    DAC->CDR = i++;
                    retardador = 0;
                }

            }while(min_rampa <= i && i < max_ramp);
        }
        rampa = 0;
        i = min_rampa;
    } while (gpio_read_pin(GPIOC, 24));
}

void rampa_externa(hcos_word_t arg){
    uint16_t i = 0;
    uint16_t cursor_pos = 2048;
    int time = 0;
    uint16_t retardador = 0;
    do {
    dac_set(DAC_CH1);
    
    ADC->CHDR = 0xFFFF;    
    adc_sel_pin(A0);

    if(cursor_pos > (4095 - LARG_CURSOR))
        cursor_pos = 4095 - LARG_CURSOR;

    if(cursor_pos < LARG_CURSOR)
        cursor_pos = LARG_CURSOR;
    int slope = POS;
    
    do{
    if(retardador == MAX_TICKS_EXT){
            retardador = 0;
            if(DAC->ISR & 0x1){
                DAC->CDR = i;
                i = i+slope;
            }
            
            if(ADC->ISR & ADC_ISR_DRDY){
                cursor_pos = adc_read(ADC);
            }
            
            if( i >= cursor_pos - LARG_CURSOR && i <= cursor_pos + LARG_CURSOR && slope == POS)
                gpio_set_pin(GPIOC, 23);
            else
                gpio_clear_pin(GPIOC, 23);
            
            if(i == 4095)
                slope = NEG;
    }
    retardador++;
    }while(!(i <= 0 && slope == NEG));

    do{
        
    }while(!(DAC->ISR & 0x1));
    DAC->CDR = cursor_pos;
    if(gpio_read_pin(GPIOC, 24)){
        contador_medidas = 0; // Conta o numero de medidas realizadas.
        rampa = 0;
        min_rampa = 2000 - (amostras/2); max_ramp = 2000 + (amostras/2); picos_iniciais = 0; media_picos = min_rampa;
        pulsoAtual = 0; pulsoAnterior = 0;
        pin_sinc = 4;//sincronização com o circuito de Higor
        rampa_interna(0);
    }
    } while (1);
}




int main(void) {
    gpio_set_pin_mode(GPIOC, 24, GPIO_INPUT_MODE|GPIO_PULLUP);

    gpio_set_pin_mode(GPIOA, 29, GPIO_INPUT_MODE);
    gpio_set_pin_mode(GPIOC, 23, GPIO_OUTPUT_MODE);
    


    DAC->CR = DAC_CR_SWRST;
    


    ADC->CR = ADC_CR_SWRST;
    ADC->MR = ADC_MR_FREERUN | code_adc_mr_settling(0) |
        code_adc_mr_tracking(0) | code_adc_mr_transfer(0) |
        code_adc_mr_prescaler(200) | code_adc_mr_startup(1);
    adc_sel_pin(A8);
    /* ADC->IER = ADC_IER_DRDY; */
    
    PMC_DAC_CLK_ENABLE();
    PMC_ADC_CLK_ENABLE();

    ADC->CR = ADC_CR_START;



    rampa_externa(0);
    reactor_start();
    return 0;
}