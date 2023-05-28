/**
 * Autor:
 * Date:
 * MAIN
 *
 * 
 * 
 */

#include <zephyr/kernel.h>          /* for kernel functions*/
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>    /* for ADC API*/
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>   /* for timing services */
#include <stdio.h>                  /* for sprintf() */
#include <stdlib.h>
#include <string.h>
#include "rtdb.h"                  /*incluir o modulo da bade de dados em tempo real*/

/* definição do espaço alocado para a stack -> mundaças de contyexto de execução entre threads*/
#define STACK_SIZE 1024

/**
 * 
 * thread_A -> trata das atualizações do estado do botões na RTDB
 * trhead_B -> trata das atualiações dos LEDS fisiccos consuante a RTDB
 * trhead_C -> trata de executar as intruções/ordens fornecidas pelo utilizador(teclado)
*/
/* prioridade do agendamento da Thread */
#define thread_A_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/ 
#define thread_B_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/
#define thread_C_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/

/* Periodo da ativação do Therad (in ms)*/
#define thread_A_period 1000
#define thread_B_period 1500
#define thread_C_period 2000

/*PARAMETROS DA USART*/
#define MAIN_SLEEP_TIME_MS 5000         /* por causa da usart !!! analisar */
#define FATAL_ERR -1                    /* Fatal error return code, app terminates */
#define RXBUF_SIZE 60                   /* RX buffer size */
#define TXBUF_SIZE 60                   /* TX buffer size */    
#define RX_TIMEOUT 1000                 /* Inactivity period after the instant when last char was received that triggers an rx event (in us) */

/* Struct for UART configuration (if using default values is not needed) */
const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

/* alocação do espaço para a thread*/
K_THREAD_STACK_DEFINE(thread_A_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_B_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_C_stack, STACK_SIZE);
  
/* criar uma estrutura do tipo K_thread */
struct k_thread thread_A_data;
struct k_thread thread_B_data;
struct k_thread thread_C_data;

/* Criar o ID da tarefa */
k_tid_t thread_A_tid;
k_tid_t thread_B_tid;
k_tid_t thread_C_tid;

/* Thread code prototypes */
void thread_A_code(void *argA, void *argB, void *argC);
void thread_B_code(void *argA, void *argB, void *argC);
void thread_C_code(void *argA, void *argB, void *argC);

/* Refer to dts file dos LEDS*/
#define LED0_NID DT_NODELABEL(led0)
#define LED1_NID DT_NODELABEL(led1)
#define LED2_NID DT_NODELABEL(led2)
#define LED3_NID DT_NODELABEL(led3)

/* Refer to dts file dos Botões*/
#define BUT0_NID DT_NODELABEL(button0)
#define BUT1_NID DT_NODELABEL(button1)
#define BUT2_NID DT_NODELABEL(button2)
#define BUT3_NID DT_NODELABEL(button3)

/*refer to dts file UART0 node ID*/
#define UART_NODE DT_NODELABEL(uart0)   /* UART0 node ID*/


/*criar as estruuras dos dispositivos de GPIO*/
const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(LED0_NID,gpios);
const struct gpio_dt_spec led1_dev = GPIO_DT_SPEC_GET(LED1_NID,gpios);
const struct gpio_dt_spec led2_dev = GPIO_DT_SPEC_GET(LED2_NID,gpios);
const struct gpio_dt_spec led3_dev = GPIO_DT_SPEC_GET(LED3_NID,gpios);

const struct gpio_dt_spec but0_dev = GPIO_DT_SPEC_GET(BUT0_NID,gpios);
const struct gpio_dt_spec but1_dev = GPIO_DT_SPEC_GET(BUT1_NID,gpios);
const struct gpio_dt_spec but2_dev = GPIO_DT_SPEC_GET(BUT2_NID,gpios);
const struct gpio_dt_spec but3_dev = GPIO_DT_SPEC_GET(BUT3_NID,gpios);

/*estrutura para a callback da interrupt*/
static struct gpio_callback but_cb_data;        

/* variavel que será chamada pela ISR dos butões-> !!volatil!*/
volatile int ledstat = 0;

/*inicialização da estrutura local*/
volatile struct state_but but;              /*Para os botões*/
volatile struct state_but led;              /*Para os Leds*/

/* UART related variables */
const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
static uint8_t rx_buf[RXBUF_SIZE];      /* RX buffer, to store received data */
static uint8_t rx_chars[RXBUF_SIZE];    /* chars actually received  */
volatile int uart_rxbuf_nchar=0;        /* Number of chars currrntly on the rx buffer */


void butPressCBFunction(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    /* Test each button ...*/
    if(BIT(but0_dev.pin) & pins) {
        if(gpio_pin_get_dt(&but0_dev))
        {
            //printk("BUT0 pressed \n\r");
            but.n0=true;

        }else{
               //printk("BUT0 Release \n\r");
               but.n0=false; 
            }
    }

    if(BIT(but1_dev.pin) & pins) 
    {
        if(gpio_pin_get_dt(&but1_dev))
        {
            //printk("BUT1 pressed \n\r");
            but.n1=true;
        }else{
               //printk("BUT1 Release \n\r");
               but.n1=false; 
            }
    }

    if(BIT(but2_dev.pin) & pins)
    {
       if(gpio_pin_get_dt(&but2_dev))
        {
            //printk("BUT2 pressed \n\r");
            but.n2=true;

        }else{
               //printk("BUT2 Release \n\r");
               but.n2=false; 
            }
    }

    if(BIT(but3_dev.pin) & pins)
    {
        if(gpio_pin_get_dt(&but3_dev))
        {
            //printk("BUT3 pressed \n\r");
            but.n3=true;
        }else{
                //printk("BUT3 Release \n\r"); 
                but.n3=false;
             }
    }

}//fim de void butPressCBFunction(const struct device *dev, struct gpio_callback *cb, uint32_t pins)



/* UART callback implementation */
/* Note that callback functions are executed in the scope of interrupt handlers. */
/* They run asynchronously after hardware/software interrupts and have a higher priority than all threads */
/* Should be kept as short and simple as possible. Heavier processing should be deferred to a task with suitable priority*/
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    int err;

    switch (evt->type) {
	
        case UART_TX_DONE:
		    printk("UART_TX_DONE event \n\r");
            break;

    	case UART_TX_ABORTED:
	    	printk("UART_TX_ABORTED event \n\r");
		    break;
		
	    case UART_RX_RDY:
		    /* Just copy data to a buffer. Usually it is preferable to use e.g. a FIFO to communicate with a task that shall process the messages*/
            memcpy(&rx_chars[uart_rxbuf_nchar],&(rx_buf[evt->data.rx.offset]),evt->data.rx.len); 
            printk("%c",rx_chars[uart_rxbuf_nchar]);
		    if(rx_chars[uart_rxbuf_nchar] == 13 )                /*13-> tecla enter pressionada */
            {
                //printk("\n..........string terminada com ENTER \n");
                rx_chars[uart_rxbuf_nchar++] = 13;                 /* adiciona o caracter do enter no seguinte*/
                rx_chars[uart_rxbuf_nchar++] = 0;                 /* Terminate the string na seguinte posição */
                uart_rxbuf_nchar = 0;                           /*Reset ao ponteiro do buffer*/
                
                err = uart_tx(uart_dev, rx_chars, strlen(rx_chars), SYS_FOREVER_MS);
            if (err) 
	    {
                printk("uart_tx() error. Error code:%d\n\r",err);
                return;
            }
                
                writeSmsInBuf(&rx_chars[uart_rxbuf_nchar]);     /*Escreve no buffer*/                 
            }else{
                   uart_rxbuf_nchar++; 
                 }
            
            break;

	    case UART_RX_BUF_REQUEST:
		    printk("UART_RX_BUF_REQUEST event \n\r");
		    break;

	    case UART_RX_BUF_RELEASED:
		    printk("UART_RX_BUF_RELEASED event \n\r");
		    break;
		
	    case UART_RX_DISABLED: 
            /* When the RX_BUFF becomes full RX is is disabled automaticaly.  */
            /* It must be re-enabled manually for continuous reception */
            printk("UART_RX_DISABLED event \n\r");
		    err =  uart_rx_enable(uart_dev ,rx_buf,sizeof(rx_buf),RX_TIMEOUT);
            if (err) {
                printk("uart_rx_enable() error. Error code:%d\n\r",err);
                exit(FATAL_ERR);                
            }
		    break;

	    case UART_RX_STOPPED:
		    printk("UART_RX_STOPPED event \n\r");
		    break;
		
	    default:
            printk("UART: unknown event \n\r");
		    break;
    }

}// FIM DE static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
/**
 * Função que configura os IO's e suas i
*/
void ioConfig()
{
    int ret;

    /*Verificar o estados dos devices de IO -- LEDS --*/    
    if(!device_is_ready(led0_dev.port))
    {
        printk("Fatal error: led0 device not ready!");
		return; 
    }    
    if(!device_is_ready(led1_dev.port))
    {
        printk("Fatal error: led1 device not ready!");
		return; 
    }

    if(!device_is_ready(led2_dev.port))
    {
        printk("Fatal error: led2 device not ready!");
		return; 
    }

    if(!device_is_ready(led3_dev.port))
    {
        printk("Fatal error: led3 device not ready!");
		return; 
    }

    /*Verificar o estados dos devices de IO -- Botões --*/
    if(!device_is_ready(but0_dev.port))
    {
        printk("Fatal error: but0 device not ready!");
		return; 
    } 

    if (!device_is_ready(but1_dev.port))  
	{
        printk("Fatal error: but1 device not ready!");
		return;
	}

    if (!device_is_ready(but2_dev.port))  
	{
        printk("Fatal error: but2 device not ready!");
        return;
	}

    if (!device_is_ready(but3_dev.port))  
	{
        printk("Fatal error: but3 device not ready!");
		return;
	} 

    /*configurar os IOs --LEDS-- como inputs*/
    ret = gpio_pin_configure_dt(&led0_dev, led0_dev.dt_flags | GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: Failed to configure LED 0 \n\r", ret);
	    return;
    }
    ret = gpio_pin_configure_dt(&led1_dev, led1_dev.dt_flags | GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: Failed to configure LED 1 \n\r", ret);
	    return;
    }
    ret = gpio_pin_configure_dt(&led2_dev, led2_dev.dt_flags | GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: Failed to configure LED 2 \n\r", ret);
	    return;
    }
    ret = gpio_pin_configure_dt(&led3_dev, led3_dev.dt_flags | GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: Failed to configure LED 3 \n\r", ret);
	    return;
    }

    /*configurar os IOs --Botões-- como /outputs/pullups.....*/
    ret = gpio_pin_configure_dt(&but0_dev, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Error %d: Failed to configure BUT 0 \n\r", ret);
	    return;
    }

    ret = gpio_pin_configure_dt(&but1_dev, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Error %d: Failed to configure BUT 1 \n\r", ret);
	    return;
    }

    ret = gpio_pin_configure_dt(&but2_dev, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Error %d: Failed to configure BUT 2 \n\r", ret);
	    return;
    }

    ret = gpio_pin_configure_dt(&but3_dev, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Error %d: Failed to configure BUT 3 \n\r", ret);
	    return;
    }

    /* ajustar o pinos dos botõeses como interrupção e de que forma gera a interrupção */
    ret = gpio_pin_interrupt_configure_dt(&but0_dev, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
	    printk("Error %d: failed to configure interrupt on BUT0 pin \n\r", ret);
	    return;
    }
    ret = gpio_pin_interrupt_configure_dt(&but1_dev, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
	    printk("Error %d: failed to configure interrupt on BUT1 pin \n\r", ret);
	    return;
    }
    ret = gpio_pin_interrupt_configure_dt(&but2_dev, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
	    printk("Error %d: failed to configure interrupt on BUT2 pin \n\r", ret);
	    return;
    }
    ret = gpio_pin_interrupt_configure_dt(&but3_dev, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
	    printk("Error %d: failed to configure interrupt on BUT3 pin \n\r", ret);
	    return;
    }

    /*preparar a collback*/
    gpio_init_callback(&but_cb_data, butPressCBFunction, BIT(but0_dev.pin)| BIT(but1_dev.pin)| BIT(but2_dev.pin) | BIT(but3_dev.pin));

    gpio_add_callback(but0_dev.port, &but_cb_data);
    gpio_add_callback(but1_dev.port, &but_cb_data);
    gpio_add_callback(but2_dev.port, &but_cb_data);
    gpio_add_callback(but3_dev.port, &but_cb_data);

}//fim de void IoConfig()
 
void usartConfig()
{
    int err=0; /* Generic error variable */
    uint8_t welcome_mesg[] = "UART_Config_init\n\r"; 
    

    /* Configure UART */
    err = uart_configure(uart_dev, &uart_cfg);
    if (err == -ENOSYS) { /* If invalid configuration */
        printk("uart_configure() error. Invalid configuration\n\r");
        return; 
    }
        
    /* Register callback */
    err = uart_callback_set(uart_dev, uart_cb, NULL);
    if (err) {
        printk("uart_callback_set() error. Error code:%d\n\r",err);
        return;
    }
		
    /* Enable data reception */
    err =  uart_rx_enable(uart_dev ,rx_buf,sizeof(rx_buf),RX_TIMEOUT);
    if (err) {
        printk("uart_rx_enable() error. Error code:%d\n\r",err);
        return;
    }

    /* Send a welcome message */ 
    /* Last arg is timeout. Only relevant if flow controll is used */
    err = uart_tx(uart_dev, welcome_mesg, sizeof(welcome_mesg), SYS_FOREVER_MS);
    if (err) {
        printk("uart_tx() error. Error code:%d\n\r",err);
        return;
    }

 }

/* Main function */
void main(void) 
{
    
   

    printk("\n ################################### \n\r ");
    printk("######## Smart I/O Module ######### \n\r ");
    printk("################################### \n\r ");
    printk(" SMSTerminal:\n\r ");
    printk(" Ex_1--------> dw01 ->Digital_Write_Led0_ON\n\r ");
    printk(" Ex_1--------> dw20 ->Digital_Write_Led2_OFF\n\r ");
    printk(" Ex_2--------> dr10 ->Digital_Read_Botao_1\n\r ");
    printk(" Ex_1--------> dr31 ->Digital_Read_Noptao_3\n\r ");
    printk("################################### \n\r ");


     /*iniciar a estrutura local dos botões a FALSE*/
       but.n0=false;
       but.n1=false;
       but.n2=false;
       but.n3=false;
       but.busy=false;
       /*inicializa a estrututura de dados para os botões em rtdb*/
       initStateButs();         /* inicaliação do estdo dos botões na RTDB*/
       initStateLeds();         /*inicialização do estado dos LEDS na RTDB*/
       ioConfig();              /*configura e inicializa os IOS Interupts e callbacks*/ 
       usartConfig();          /*configura e inicializa a usart com interrupts e callback*/
       initMsnBuffer();         /*inicaliza o buffer para armazenar as sms escrita pelo teclado*/


    /*Criar o thread -> atualiza a RTDB com o estado dos Botões*/
    thread_A_tid = k_thread_create(&thread_A_data, thread_A_stack,
        K_THREAD_STACK_SIZEOF(thread_A_stack), thread_A_code,
        NULL, NULL, NULL, thread_A_prio, 0, K_NO_WAIT);

    /*Criar o thread -> atualiza os Leds com o estado da RTDB*/
    thread_B_tid = k_thread_create(&thread_B_data, thread_B_stack,
        K_THREAD_STACK_SIZEOF(thread_B_stack), thread_B_code,
        NULL, NULL, NULL, thread_B_prio, 0, K_NO_WAIT);

    /*Criar o thread -> atualiza a RTDB Os dados inseridos pelo utilizador*/
    thread_C_tid = k_thread_create(&thread_C_data, thread_C_stack,
        K_THREAD_STACK_SIZEOF(thread_C_stack), thread_C_code,
        NULL, NULL, NULL, thread_C_prio, 0, K_NO_WAIT);

    return;
} 


/**
 *  Thread_A ->code implementation
 * Este thread é responsável por atualizar o estado dos botões fisicos na RTDB
  */
void thread_A_code(void *argA , void *argB, void *argC)
{
    /* variaveis locauis */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    
    /* Task init code */
    printk("Thread de atualizacao das RTDB com os estado dos inputs-> Botões\n\r");
    
    /* Compute next release instant */
    release_time = k_uptime_get() + thread_A_period;

    /* Thread loop */
    while(1) 
    {        
         /*actualiza a rtdb com o estado atual dos botões*/
            writeButsInRtdb(but);
       
        /* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time)
         {
            k_msleep(release_time - fin_time);
            release_time += thread_A_period;

        }
    }

    /* Stop timing functions */
    timing_stop();
}// fim de void thread_A_code(void *argA , void *argB, void *argC)


/**
 *  Thread_B ->code implementation
 * Este thread é responsável Ler os estado dos LEDs na RTDB e atualizar as saídas fisicas
 * 
  */
void thread_B_code(void *argA , void *argB, void *argC)
{
    /* variaveis locauis */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    struct state_led ret;
    /* Task init code */
    printk("Thread de atualizacao dos LEDs mediate as infos da RTDB \n\r");
    
    /* Compute next release instant */
    release_time = k_uptime_get() + thread_B_period;

    /* Thread loop */
    while(1) 
    {        
         /*actualiza a os estado atual dos LEDS -> Saidas com a informação presente na RTDB*/
         ret=writeLedsInRtdb();                     /*Lê RTDB e atualiza as saídas fisicas(LEDs) com esse valor*/
         gpio_pin_set_dt(&led0_dev,ret.n0);
         gpio_pin_set_dt(&led1_dev,ret.n1);
         gpio_pin_set_dt(&led2_dev,ret.n2);
         gpio_pin_set_dt(&led3_dev,ret.n3);
            
       
        /* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time)
         {
            k_msleep(release_time - fin_time);
            release_time += thread_B_period;

        }
    }

    /* Stop timing functions */
    timing_stop();
}//fim de void thread_B_code(void *argA , void *argB, void *argC)

/**
 *  Thread_B ->code implementation
 * Este thread é responsável Ler os estado dos LEDs na RTDB e atualizar as saídas fisicas
 * 
  */
void thread_C_code(void *argA , void *argB, void *argC)
{
    /* variaveis locauis */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    int ret=0;
     /* Task init code */
    printk("Thread Execucao das leitiuras(botpes) e escritas(leds) SMS do utilizador \n\r");
    
    /* Compute next release instant */
    release_time = k_uptime_get() + thread_B_period;

    /* Thread loop */
    while(1) 
    {        
         /*codido da thread_C*/
         ret=readSmsInBuf();
         if(ret != 0)
         {
            printk("Erro no formato da sms CodError: %d\n",ret);
         }
        
       
        /* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time)
         {
            k_msleep(release_time - fin_time);
            release_time += thread_B_period;

        }
    }

    /* Stop timing functions */
    timing_stop();
}//fim de void thread_B_code(void *argA , void *argB, void *argC)
