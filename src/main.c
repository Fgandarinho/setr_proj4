/*
 * Paulo Pedreiras, 2023/02
 * Zephyr: Simple thread and digital IO example
 * 
 * Reads a button and sets a led according to the button state
 *
 * Base documentation:
 *  Zephyr kernel: 
 *  https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/kernel/services/index.html#kernel-services
 *  *
 *  HW info
 *      https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html
 *      Section: nRF52840 Product Specification -> Peripherals -> GPIO / GPIOTE
 * 
 *      Board specific HW info can be found in the nRF52840_DK_User_Guide_20201203. I/O pins available at pg 27
 * 
 * 
 */

#include <zephyr/kernel.h>          /* for kernel functions*/
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>   /* for timing services */
#include "rtdb.h"                  /*incluir o modulo da bade de dados em tempo real*/

/* Size of stack area used by each thread (can be thread specific, if necessary)*/
#define STACK_SIZE 1024

/* Thread scheduling priority */
#define thread_A_prio 1

/* Therad periodicity (in ms)*/
#define thread_A_period 200


/* Create thread stack space */
K_THREAD_STACK_DEFINE(thread_A_stack, STACK_SIZE);
  
/* Create variables for thread data */
struct k_thread thread_A_data;

/* Create task IDs */
k_tid_t thread_A_tid;

/* Thread code prototypes */
void thread_A_code(void *argA, void *argB, void *argC);

/* Refer to dts file */
#define LED0_NID DT_NODELABEL(led0)
#define BUT0_NID DT_NODELABEL(button0)


/* Main function */
void main(void) {

    thread_A_tid = k_thread_create(&thread_A_data, thread_A_stack,
        K_THREAD_STACK_SIZEOF(thread_A_stack), thread_A_code,
        NULL, NULL, NULL, thread_A_prio, 0, K_NO_WAIT);

    return;
} 

/* Thread code implementation */
void thread_A_code(void *argA , void *argB, void *argC)
{
    /* Local vars */
    int64_t fin_time=0, release_time=0;     /* Timing variables to control task periodicity */
    const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(LED0_NID, gpios); /* GPIO device structure for LED*/
    const struct gpio_dt_spec but0_dev = GPIO_DT_SPEC_GET(BUT0_NID, gpios); /* GPIO device structure for Button*/
        
    int ret=0;     /* Generic return value variable */
    
    /* Task init code */
    printk("Thread A init (periodic)\n");
    
    /* Check if Led and button devices are ready */
	if (!device_is_ready(led0_dev.port))  
	{
        printk("Fatal error: led0 device not ready!");
		return;
	}
    if (!device_is_ready(but0_dev.port))  
	{
        printk("Fatal error: but0 device not ready!");
		return;
	}

    /* Configure led0 and but0 IOs */
    ret = gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to configure led0 \n\r");
	    return;
    }
    ret = gpio_pin_configure_dt(&but0_dev, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        printk("Failed to configure but0 \n\r");
	    return;
    }
           
    /* Compute next release instant */
    release_time = k_uptime_get() + thread_A_period;

    /* Thread loop */
    while(1) {        
        
        printk("Thread A activated\n\r");  
        
        if(gpio_pin_get_dt(&but0_dev)) {
            gpio_pin_set_dt(&led0_dev,1);
            printk("but0 active\n\r");
        } else {
            gpio_pin_set_dt(&led0_dev,0);
            printk("but0 not active\n\r");
        }
       
        /* Wait for next release instant */ 
        fin_time = k_uptime_get();
        if( fin_time < release_time) {
            k_msleep(release_time - fin_time);
            release_time += thread_A_period;

        }
    }

    /* Stop timing functions */
    timing_stop();
}

