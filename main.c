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

unsigned int lerMedida[NUM_MEDIDAS], nMedida[NUM_MEDIDAS ];//lerMedida[40]: medidas lidas pelo pinMedicao. nMedida[40]: o ponto da rampa correspondente a cada medida tirada.
unsigned int pontoPico[NUM_MAX_PICOS],numDePicos = 0;
unsigned int media = 0, mediaTrans = 0, picoAtual = 0;

/*
 * Callback to blink the led to show the app is active
 */
void blink_cb(hcos_word_t arg) {
    gpio_toggle_pin(GPIOB, 27);
}

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


void analisa_maximo(){//função para analisar o maximo valor do pico}*)
  unsigned int auxPico = 0, picoAtual = 0,ponto_pico[10], nMedida[0];
  
  for(int u=0;u<=39;u++){
    if(auxPico<lerMedida[u]){//pega o maior valor dentro destes dados
      auxPico=lerMedida[u];
      ponto_pico[picoAtual] = nMedida[u];
    }
  }
  
  for(int u=0; u<=39;u++)
        lerMedida[u]=0;
  
}

void rampa_interna(hcos_word_t arg){
    unsigned int contadorMedidas = 0; // Conta o numero de medidas realizadas.
    unsigned int amostras = 250, rampa = 0;
    uint16_t min_rampa = 2000 - (amostras/2), max_ramp = 2000 + (amostras/2), pontosIniciais = 0,mediaAux = min_rampa;
    uint16_t pulsoAtual = 0, pulsoAnterior = 0;
    unsigned int pinSinc = 4;//sincronização com o circuito de Higor


    
    uint16_t i = min_rampa;
    dac_set(DAC_CH1);
    gpio_toggle_pin(GPIOC, 25);

    do{            
        if(DAC->ISR & 0x1){
            DAC->CDR = i;            
        }
#if 0
        if(i == max_ramp){
            analisa_maximo();//define o maximo a ser mandado
            pontosIniciais++;
            if(pontosIniciais < NUM_MAX_PICOS){//picos a serem lidos para se ter uma media antes de se realizar o travamento
                do{
                  DAC->CDR = pontoPico[picoAtual];
                }while(!(DAC->ISR & 0x1));
                picoAtual++;
            }else{//quando terminar de ler os picos iniciais...
                picoAtual = NUM_MAX_PICOS - 1;
                pontosIniciais--;
                for(int k=0;k <= NUM_MAX_PICOS-1;k++)
                media = pontoPico[k] + media;
                media = media/NUM_MAX_PICOS;
                media = media;
                for(int k=0; k < 5; k++)
                    mediaTrans = mediaTrans + pontoPico[picoAtual - k];
                mediaTrans = mediaTrans / 5;//mediaTrans: média de translação
                for(int k=1;k <= NUM_MAX_PICOS - 1;k++)
                pontoPico[k-1] = pontoPico[k];
                if(mediaTrans > 4000-(amostras/2))//definindo um valor maximo que mediaTrans pode ser
                    mediaTrans = 4000-(amostras/2);
                if(mediaTrans < (amostras/2))//definindo um valor minimo que mediaTrans pode ser
                    mediaTrans = amostras/2;        
                if(mediaTrans != (max_ramp + min_rampa) / 2){//este if tem como finalidade acompanhar o pico em que se pretende travar, mudando o max e min da rampa gerada
                    max_ramp = mediaTrans + amostras/2;
                    min_rampa = mediaTrans - amostras/2;
                }
                if(DAC->ISR & 0x1 && rampa == 0){
                DAC->CDR = mediaAux;
                }
            }
        }
#endif
        i++;
            
    }while(min_rampa <= i < max_ramp);
    while(1){}
    ctr = 0;
    
    if(gpio_read_pin(GPIOC, 24))
        reactor_add_handler(rampa_interna, 0);
    else
        reactor_add_handler(rampa_externa, 0);
}

void rampa_externa(hcos_word_t arg){
    uint16_t i = 0;
    uint16_t cursor_pos = 2048;
    int time = 0;
    dac_set(DAC_CH0);
    
    ADC->CHDR = 0xFFFF;    
    adc_sel_pin(A6);

    


    if(cursor_pos > (4095 - LARG_CURSOR))
        cursor_pos = 4095 - LARG_CURSOR;

    if(cursor_pos < LARG_CURSOR)
        cursor_pos = LARG_CURSOR;
    int slope = POS;
    
    do{
            if(DAC->ISR & 0x1){
                DAC->CDR = i;
            }
            
            if(ADC->ISR & ADC_ISR_DRDY){
                cursor_pos = adc_read(ADC);
            }
            
            if( i >= cursor_pos - LARG_CURSOR && i <= cursor_pos + LARG_CURSOR && slope == POS)
                gpio_set_pin(GPIOC, 25);
            else
                gpio_clear_pin(GPIOC, 25);
            
            if(i == 4095)
                slope = NEG;
            i = i+slope;
        
        
    }while(i > 0);
    
    if(gpio_read_pin(GPIOC, 24))
        reactor_add_handler(rampa_interna, 0);
    else
        reactor_add_handler(rampa_externa, 0);
}




int main(void) {
    gpio_set_pin_mode(GPIOC, 24, GPIO_INPUT_MODE||GPIO_PULLUP);
    gpio_set_pin_mode(GPIOC, 25, GPIO_OUTPUT_MODE);
    


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



    

    //gpio_set_pin_mode(GPIOC, 22, GPIO_INPUT_MODE);
    //gpio_set_pin(GPIOB, 27);
    //adc_start(&adc, &adc_config);
    //adc_start_conversion(&adc, adc_buffer, ADC_BUFFER_SIZE);
    reactor_add_handler(rampa_externa, 0);
    reactor_start();
    return 0;
}