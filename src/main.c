/**
 * @file main.c
 * 
 * @brief Main file of the project
 * 
 * @version 1.0
 * 
 * @date 2021-06-06
 * 
 * @author Fernando Gandarinho e Tomás Silva
 * 
 * @note Project developed for the subject of Real Time Operating Systems
 * @note This project is based on the Zephyr Project RTOS
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
#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include "rtdb.h"                  /*incluir o modulo da bade de dados em tempo real*/

/* definição do espaço alocado para a stack -> mundaças de contyexto de execução entre threads*/
/**
 * @brief Stack size of the threads
*/
#define STACK_SIZE 1024

/**
 * @brief I2C address of the TC74
*/
#define I2CAddr 0x4D

/*
  thread_A -> trata das atualizações do estado do botões na RTDB
  trhead_B -> trata das atualiações dos LEDS fisiccos consuante a RTDB
  trhead_C -> trata de executar as intruções/ordens fornecidas pelo utilizador(teclado)
  thread_D -> trata de atualizar a temperatura na RTDB
*/
/* prioridade do agendamento da Thread */

/**
 * @brief Priority of the threads
*/
#define thread_A_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/ 

/**
 * @brief Priority of the threads
*/
#define thread_B_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/

/**
 * @brief Priority of the threads
*/
#define thread_C_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/

/**
 * @brief Priority of the threads
*/
#define thread_D_prio 1 /* 1-> maior prooridade 2-> menos prioridade*/

/* Periodo da ativação do Therad (in ms)*/

/**
 * @brief Period of the thread A
*/
#define thread_A_period 1000

/**
 * @brief Period of the thread B
*/
#define thread_B_period 1500

/**
 * @brief Period of the thread C
*/
#define thread_C_period 2500

/**
 * @brief Period of the thread D
*/
#define thread_D_period 2000

/*PARAMETROS DA USART*/
/**
 * @brief Baudrate of the UART
*/
#define MAIN_SLEEP_TIME_MS 5000         /* por causa da usart !!! analisar */

/**
 * @brief Error code
*/
#define FATAL_ERR -1                    /* Fatal error return code, app terminates */

/**
 * @brief RX buffer size
*/
#define RXBUF_SIZE 60                   /* RX buffer size */

/**
 * @brief TX buffer size
*/
#define TXBUF_SIZE 60                   /* TX buffer size */    

/**
 * @brief RX timeout
*/
#define RX_TIMEOUT 1000                 /* Inactivity period after the instant when last char was received that triggers an rx event (in us) */


/* Struct for UART configuration (if using default values is not needed) */
/**
 * @brief UART configuration struct
*/
const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

/* alocação do espaço para a thread*/
/**
 * @brief Stack allocation for the threads
*/
K_THREAD_STACK_DEFINE(thread_A_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_B_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_C_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_D_stack, STACK_SIZE);
  
/* criar uma estrutura do tipo K_thread */
/**
 * @brief Thread A data
*/
struct k_thread thread_A_data;

/**
 * @brief Thread B data
*/
struct k_thread thread_B_data;

/**
 * @brief Thread C data
*/
struct k_thread thread_C_data;

/**
 * @brief Thread D data
*/
struct k_thread thread_D_data;

/* Criar o ID da tarefa */
/**
 * @brief Thread A ID
*/
k_tid_t thread_A_tid;

/**
 * @brief Thread B ID
*/
k_tid_t thread_B_tid;

/**
 * @brief Thread C ID
*/
k_tid_t thread_C_tid;

/**
 * @brief Thread D ID
*/
k_tid_t thread_D_tid;

/* Thread code prototypes */
/**
 * @brief Thread A code prototype
*/
void thread_A_code(void *argA, void *argB, void *argC);

/**
 * @brief Thread B code prototype
*/
void thread_B_code(void *argA, void *argB, void *argC);

/**
 * @brief Thread C code prototype
*/
void thread_C_code(void *argA, void *argB, void *argC);

/**
 * @brief Thread D code prototype
*/
void thread_D_code(void *argA, void *argB, void *argC);

/* Refer to dts file dos LEDS*/
/**
 * @brief LED 0 node ID
*/
#define LED0_NID DT_NODELABEL(led0)

/**
 * @brief LED 1 node ID
*/
#define LED1_NID DT_NODELABEL(led1)

/**
 * @brief LED 2 node ID
*/
#define LED2_NID DT_NODELABEL(led2)

/**
 * @brief LED 3 node ID
*/
#define LED3_NID DT_NODELABEL(led3)

/* Refer to dts file dos Botões*/
/**
 * @brief Button 0 node ID
*/
#define BUT0_NID DT_NODELABEL(button0)

/**
 * @brief Button 1 node ID
*/
#define BUT1_NID DT_NODELABEL(button1)

/**
 * @brief Button 2 node ID
*/
#define BUT2_NID DT_NODELABEL(button2)

/**
 * @brief Button 3 node ID
*/
#define BUT3_NID DT_NODELABEL(button3)

/*refer to dts file UART0 node ID*/
/**
 * @brief UART node ID
*/
#define UART_NODE DT_NODELABEL(uart0)   /* UART0 node ID*/

/**
 * @brief I2C node ID
*/
#define I2C0_NODE DT_NODELABEL(tc74)    /*I2C node ID*/

/*criar as estruuras dos dispositivos de GPIO*/
/**
 * @brief LED 0 device
*/
const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(LED0_NID,gpios);

/**
 * @brief LED 1 device
*/
const struct gpio_dt_spec led1_dev = GPIO_DT_SPEC_GET(LED1_NID,gpios);

/**
 * @brief LED 2 device
*/
const struct gpio_dt_spec led2_dev = GPIO_DT_SPEC_GET(LED2_NID,gpios);

/**
 * @brief LED 3 device
*/
const struct gpio_dt_spec led3_dev = GPIO_DT_SPEC_GET(LED3_NID,gpios);

/**
 * @brief Button 0 device
*/
const struct gpio_dt_spec but0_dev = GPIO_DT_SPEC_GET(BUT0_NID,gpios);

/**
 * @brief Button 1 device
*/
const struct gpio_dt_spec but1_dev = GPIO_DT_SPEC_GET(BUT1_NID,gpios);

/**
 * @brief Button 2 device
*/
const struct gpio_dt_spec but2_dev = GPIO_DT_SPEC_GET(BUT2_NID,gpios);

/**
 * @brief Button 3 device
*/
const struct gpio_dt_spec but3_dev = GPIO_DT_SPEC_GET(BUT3_NID,gpios);

/*estrutura para a callback da interrupt*/
/**
 * @brief GPIO callback data
*/
static struct gpio_callback but_cb_data;        

/* variavel que será chamada pela ISR dos butões-> !!volatil!*/
volatile int ledstat = 0;

/*inicialização da estrutura local*/
/**
 * @brief Button state
*/
volatile struct state_but but;              /*Para os botões*/

/**
 * @brief LED state
*/
volatile struct state_but led;              /*Para os Leds*/

/* UART related variables */
/**
 * @brief UART device struct
*/
const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);                 /* USART Struct */ 

/**
 * @brief I2C device struct
*/
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C0_NODE);     /* I2C struct */

/**
 * @brief RX buffer
*/
static uint8_t rx_buf[RXBUF_SIZE];                                        /* RX buffer, to store received data */

/**
 * @brief TX buffer
*/
static uint8_t rx_chars[RXBUF_SIZE];                                      /* chars actually received  */

/**
 * @brief RX buffer number of chars
*/
volatile int uart_rxbuf_nchar=0;                                          /* Number of chars currrntly on the rx buffer */



/**
 * @brief Function to read the state of the buttons
 * 
 * @param[in] dev Device struct
 * @param[in] cb Callback struct
 * @param[in] pins Pins
 * 
*/
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

/**
 * @brief UART callback
 * 
 * @param[in] dev UART Device struct
 * @param[in] evt Event struct
 * @param[in] user_data User data
 * 
*/
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
                rx_chars[uart_rxbuf_nchar++] = 0;                 /* Terminate the string na seguinte posição */
                uart_rxbuf_nchar = 0;                           /*Reset ao ponteiro do buffer*/
                //printk("Acordei\n\r");
                k_thread_resume(thread_C_tid);          //adorda p thread que vai ler o buffer e processar a sua informação
                              
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

/*
 * Função que configura os IO's
*/

/**
 * @brief IO configuration
 * 
 * @note This function configures the IOs
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
 

/**
 * @brief UART initialization and configuration
 * 
 * @note This function initializes and configures the UART
*/
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

/*
 * Funcção que inicializa e configura a comunicação I2C,
 * para solicitar e ler a temperatura do TC74
*/

/**
 * @brief I2C initialization and configuration for TC74
 * 
 * @note This function initializes and configures the I2C for the temperature sensor
*/
void i2cConfig()
{
    
  
	if (!device_is_ready(dev_i2c.bus)) 
	{
		printk("I2C bus %s is not ready!\n\r",dev_i2c.bus->name);
		return;
	}
  


}// fim de void i2cConfig()

/* Main function */

/**
 * @brief Main function
 * 
 * @note This function is the main function of the project
*/
void main(void) 
{
    printk("\n ################################### \n\r ");
    printk("######## Smart I/O Module ######### \n\r ");
    printk("################################### \n\r ");
    printk(" SMS_Terminal:\n\r ");
    printk(" Global Format -> [#][d;a;t][w;r;a][num_device][value][CS][!]\n\r ");
    printk(" Ex_1->  #dw00;!   ->Led0_OFF\n\r ");
    printk(" Ex_2->  #dw01<!   ->Led0_ON\n\r ");
    printk(" Ex_3->  #dw10<!   ->Led1_OFF\n\r ");
    printk(" Ex_4->  #dw11=!   ->Led1_ON\n\r ");
    printk(" Ex_5->  #dw20=!   ->Led2_OFF\n\r ");
    printk(" Ex_6->  #dw21>!   ->Led2_ON\n\r ");
    printk(" Ex_7->  #dw30>!   ->Led3_OFF\n\r ");
    printk(" Ex_8->  #dw31?!   ->Led4_ON\n\r ");
    printk(" Ex_9->  #dr006!   ->Staus_Bot_0\n\r ");
    printk(" Ex_10-> #dr107!   ->Staus_Bot_1\n\r ");
    printk(" Ex_11-> #dr208!   ->Staus_Bot_2\n\r ");
    printk(" Ex_12-> #dr309!   ->Staus_Bot_3\n\r ");
    printk(" Ex_13-> #ar003!   ->View_Temp.& St.Point\n\r ");
    printk(" Ex_14-> #aw370!   ->Write temp=37\n\r ");
    printk(" Ex_15-> #ad997!   ->ReadAll\n\r ");
    printk(" Ex_16-> #ta1351>! ->threadA com Periodo 1351\n\r ");
    printk(" Ex_17-> #tt99999! ->ReadAllPeriodTreads\n\r" );
    printk("###################################\n\n\r");


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
       i2cConfig();             /*configura e inicializa a comunicação I2C com o TC74*/
       initTempInRtdb();        /*Inicializar o setpoint e a temp com valores conhecidos*/
       initPeridosTheads(thread_A_period,thread_B_period,thread_D_period);      /*copia os periodos por defeito dos defines para a estrutura*/

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

    thread_D_tid = k_thread_create(&thread_D_data, thread_D_stack,
        K_THREAD_STACK_SIZEOF(thread_D_stack), thread_D_code,
        NULL, NULL, NULL, thread_D_prio, 0, K_NO_WAIT);

    return;
} 


/*
 *  Thread_A ->code implementation
 * Este thread é responsável por atualizar o estado dos botões fisicos na RTDB
  */

 /**
  * @brief Thread A code implementation
  * 
  * @param[in] argA Argument A
  * @param[in] argB Argument B
  * @param[in] argC Argument C
  * 
  * @note Este thread é responsável por atualizar o estado dos botões fisicos na RTDB
 */
void thread_A_code(void *argA , void *argB, void *argC)
{
    /* variaveis locauis */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    
    /* Task init code */
    printk("Thread de atualizacao da RTDB com os estado dos inputs-> Botões\n\r");
    
    /* Compute next release instant */
    release_time = k_uptime_get() + readPeriodThread('a');

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
            release_time += readPeriodThread('a');

        }
    }

    /* Stop timing functions */
    timing_stop();
}// fim de void thread_A_code(void *argA , void *argB, void *argC)


/*
 *  Thread_B ->code implementation
 * Este thread é responsável Ler os estado dos LEDs na RTDB e atualizar as saídas fisicas
 * 
  */

 /**
  * @brief Thread B code implementation
  * 
  * @param[in] argA Argument A
  * @param[in] argB Argument B
  * @param[in] argC Argument C
  * 
  * @note Este thread é responsável Ler os estado dos LEDs na RTDB e atualizar as saídas fisicas
 */
void thread_B_code(void *argA , void *argB, void *argC)
{
    /* variaveis locauis */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    struct state_led ret;
    /* Task init code */
    printk("Thread de atualizacao dos LEDs mediante as infos da RTDB \n\r");
    
    /* Compute next release instant */
    release_time = k_uptime_get() + readPeriodThread('b');

    /* Thread loop */
    while(1) 
    {     //printk("thread_B\n\r");      
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
            release_time += readPeriodThread('b');

        }
    }

    /* Stop timing functions */
    timing_stop();
}//fim de void thread_B_code(void *argA , void *argB, void *argC)


/*
 *  Thread_C ->code implementation
 * Thread_C leitura e execução das SMS vindas do utilizador
 * 
  */

 /**
  * @brief Thread C code implementation
  * 
  * @param[in] argA Argument A
  * @param[in] argB Argument B
  * @param[in] argC Argument C
  * 
  * @note Esta thread é responsável pela leitura e execução dos comandos do utilizador
 */
void thread_C_code(void *argA , void *argB, void *argC)
{
  
    int ret=0;
     /* Task init code */
    printk("Thread_C leitura e execução das SMS vindas do utilizador \n\r");
    
    /* Thread loop */
    while(1) 
    {    
          k_thread_suspend(thread_C_tid);       //suspend o thread_c, vai ser acordado pela interrupt da usart ao pressionar enter
           
         /*codido da thread_C*/
         ret=readSmsInBuf(&rx_chars[0], RXBUF_SIZE );         //vai ler as infos do buffer e processa-las de acordo com a codificação
         for(int i=0; i<RXBUF_SIZE ; i++)        //limpa o buffer para não existir conlfitos de novas sms  
         {
            rx_chars[i]=0;
         }
          
        //uart_rxbuf_nchar = 0; 
         if(ret != 0)
         {
            printk("Erro no formato da sms CodError: %d\n",ret);
         }
        
        //suspend_myself()
       
     }

    /* Stop timing functions */
    timing_stop();
}//fim de void thread_B_code(void *argA , void *argB, void *argC)


/*
 * Thread_D
 * Este Thread tem como objetivo atualizar periodicamete a RTDB com as informações do
 * Sensor TC47 que comunica por I2C
*/

/**
 * @brief Thread D code implementation
 * 
 * @param[in] argA Argument A
 * @param[in] argB Argument B
 * @param[in] argC Argument C
 * 
 * @note Esta thread é responsável por atualizar periodicamente a RTDB com as informações do sensor de temperatura
*/
void thread_D_code(void *argA , void *argB, void *argC)
{
    /* variaveis locauis */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    
    /* Task init code */
    printk("Thread_C de atualizacao da temperatura na RTDB mediante sensor_TC47\n\r");
    
    /* Compute next release instant */
    release_time = k_uptime_get() + readPeriodThread('d');
    
    int ret;
    uint8_t config = 0x00;
    uint8_t data;
    
    /* Thread loop */
    while(1) 
    {   //printk("thread_D\n\r");        
       
		ret = i2c_write_dt(&dev_i2c, &config, sizeof(config));
		if(ret != 0)
        {
			printk("Failed to write to I2C device address %x at reg. %x \n\r", dev_i2c.addr,config);
		}

		ret = i2c_read_dt(&dev_i2c, &data, sizeof(data));
		if(ret != 0)
        {
			printk("Failed to read from I2C device address %x at Reg. %x \n\r", dev_i2c.addr,config);
		}

		//printk("Temperature Reading a armazenada a RTDB: %d\n",data);
        writeTempInRtdb(&data);    
       
        /* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time)
         {
            k_msleep(release_time - fin_time);
            release_time += readPeriodThread('d');
        }
    }

    /* Stop timing functions */
    timing_stop();
}//fim de void thread_D_code(void *argA , void *argB, void *argC)