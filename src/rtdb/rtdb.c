#include "rtdb.h"                 
#include <zephyr/sys/printk.h>

/*inicializar a estrutura para os estado dos botões na RTDB*/
static struct state_but butRtdb;
static struct state_led ledRtdb;
static struct msn message;

/*função que inicializa o estado dos botoes*/
void initStateButs(void)
{
    /*colocar todos a false->0*/
    butRtdb.n0=0;
    butRtdb.n1=0;
    butRtdb.n2=0;
    butRtdb.n3=0;
    butRtdb.busy=0;

}

/*função que inicializa o estado dos botoes*/
void initStateLeds(void)
{
    /*Inicializado com um ordem ao acaso*/
    /*Esta ordem deve refletir-se de imediato no momento do arranque*/
    ledRtdb.n0=1;
    ledRtdb.n1=0;
    ledRtdb.n2=0;
    ledRtdb.n3=1;
    ledRtdb.busy=0;
}
/**
 * Atulisa a estrura butRtdb do exterior para a rtdb
*/
void writeButsInRtdb(struct state_but bt)
{
   
    butRtdb.n0=bt.n0;
    butRtdb.n1=bt.n1;
    butRtdb.n2=bt.n2;
    butRtdb.n3=bt.n3;
    butRtdb.busy=bt.busy;
    //printk("------------------------>b0:%d  b0:%d  b0:%d  b0:%d \n", butRtdb.n0, butRtdb.n1, butRtdb.n2, butRtdb.n3);
}

/*faz a atualiação dos estado dos LEDs da RTDB para Exterior*/
 struct state_led writeLedsInRtdb(void)
{
    struct state_led ret;

    ret.n0=ledRtdb.n0;
    ret.n1=ledRtdb.n1;
    ret.n2=ledRtdb.n2;
    ret.n3=ledRtdb.n3;
    ret.busy=ledRtdb.busy;
    //printk("------------------------>LED0:%d  LED1:%d  LED2:%d  LED3:%d \n", ret.n0, ret.n1, ret.n2, ret.n3);

    return ret;
}

/**inicializa o buffer para receber as sms do tipo FIFO Circular
 * 
 *     |###SMS_1###| <- head
 *     |###SMS_2###|
 *     |###SMS_3###|
 *     |    ...    |
 *     |    ...    |
 *     |###SMS_N###| <-tail 
*/
void initMsnBuffer(void)
{
    message.count=0;
    message.head=0;
    message.tail=BUFF_NUM_MAX_MSN;
    for(int i=0; i<BUFF_NUM_MAX_MSN; i++)
    {
        for(int j=0; j<LENGTH_MAX_MSN; j++)
        {
            message.buffer[i][j]='*';               //inicalizar o buffer com o caracter '*'
        }
    }
   printk("------------------------>Buffer inicializado %c \n", message.buffer[3][3]); 
}//fim de void initMsnBuffer(void)


/*escreve as sms vindas do teclado no buffer*/
void writeSmsInBuf(unsigned char *ptr_sms)
{

    if( message.count<BUFF_NUM_MAX_MSN)                 /*Verifica se o Buffer está cheio*/
    {
        unsigned char cs=0;                             /*variavel que acomula o checkSum para verificaçao de erros*/
        message.buffer[message.head][0] = SOF_SYM;      /*Insere o Simbolo de INICIO de trama*/
        int h=1;                                        /*indica a posição do catater na string*/      
        while((*ptr_sms!=13) && (h!=LENGTH_MAX_MSN) )       /*Equanto não encontrar o ENTER && não atingir a capacidade maxima do buffer*/
        {
            message.buffer[message.head][h]=*ptr_sms;    /*carrega o Buffer com a SMS introduzida no teclado*/  
            cs= cs + message.buffer[message.head][h];    /*Atualiza o checkSum*/   
            
            //printk("----->%d, %c, %c\n",h,message.buffer[message.head][h],*ptr_sms);
            ptr_sms++;
            h++;
        }
        message.buffer[message.head][h++] = cs;            /*insere o valor do checkSum*/  
        message.buffer[message.head][h++] = EOF_SYM;          /*Insere o Simbolo de FIM de trama*/
        message.buffer[message.head][h++] = 0;                /*Insere o Simbolopara final de sring*/

        if(message.head<BUFF_NUM_MAX_MSN)                   /*incrementa para a proxima posição do Buffer SE o Buffer não chegar ao fim*/
        {
            message.head = message.head + 1;              
        }else                                              /*ou caso contrario inicia na primeria posição*/                             
            {
                message.head=0;                                 
            }

        message.count=message.count+1;                      /*sinaliza mais uma mensagem inserida*/ 

       printk("EM Write::: head->%d, tail->%d. count->%d\n",message.head,message.tail,message.count);
       
    }else
        {
          printk("ATENÇÂO: buffer cheio....ultima mensagem pedida\n\r");      
        }
}// fim de void writeSmsInBuf(unsigned char *ptr_sms)


/**
 * Lê as mensagens no buffer e executa-as
 * Não é necessário nenhum parametro de entrada pois os Buffer esté criado dentro dete módulo rtdb.c/rtdb.h
 * Saidas:  *Valid ->0 || Err_Empty_str ->1 || Err_Invalid_cmd -> -2 || Err_CS ->-3 || Err_str_format -> -4
 */
int readSmsInBuf(void)
{
    int ret=0;                                  /*variavel de retorno*/
    int indx_SOF=0, numbs_EOF=0;
	int indx_EOF=0, numbs_SOF=0;
	unsigned char CS=0;
    
    if(message.count>0)                        /*Verifica se existe novas mensagens a processar*/
    {
        if(message.tail<BUFF_NUM_MAX_MSN)       /*O ptr tail está sempre a apontar para a ultima sms executada-> temos de atualizar primeiro*/
        {
            message.tail++;
        }else{                                  /*Se chegou ao final du buffer volta ao inicio*/
               message.tail=0;                  
             }
        message.count=message.count-1;
        printk("EM Read::: head->%d, tail->%d. count->%d\n",message.head,message.tail,message.count);

        /*Encontrar o indice de EOF e o SOF*/
	    for(int i=0; i<LENGTH_MAX_MSN; i++)
	    {
	    	if(message.buffer[message.tail][i] == SOF_SYM)
	    	{
	    		if(numbs_SOF==0)		/*So atribui o indice ao primeiro SOF encontrado*/		
	    		{
	    			indx_SOF=i;			
	    		}
	    		numbs_SOF++;	
	    	}
    
    		if(message.buffer[message.tail][i] == EOF_SYM)
    		{
    			if(numbs_EOF==0)		/*So atribui o indice ao primeiro EOF encontrado*/		
    			{
	    			indx_EOF=i;
		    	}
			    numbs_EOF++;	
	    	}
	    }

        /*Validar a String Introduzida no */
        if(numbs_SOF!=1 || numbs_EOF!=1)	/*caso a mensagem tenha mais que um SOF e EOF ou nao tenha nenhum*/
	    {
	    	return Err_str_format;
		}else{ 
		        if(indx_SOF>indx_EOF)		/*caso o indece do SOF esteja depois do EOF*/	
			    {
		        	return Err_str_format;
		        }
		    } 
         //if((indx_EOF-indx_SOF) < 3)
         //{
           // printk("errooor -> %d",Err_Empty_str);
            //return Err_Empty_str;
         //}   
            


        /* If a SOF was found look for commands */
        if( message.buffer[message.tail][indx_SOF+1] == 'd') 
        {
                /*calcular o CS*/
                CS = message.buffer[message.tail][indx_SOF+1]+message.buffer[message.tail][indx_SOF+2]+message.buffer[message.tail][indx_SOF+3]+message.buffer[message.tail][indx_SOF+4];
                /*Verficar o CS Enviado com o recebido*/			
                if(message.buffer[message.tail][indx_SOF+5] == CS) /*verificar se o CS enviado e igual ao CS calculado no local*/
                {
                      ret=Valid;				/*TUdO OK..... e estrutura atualizada Dos LEDS :)*/
                }else{
                    return Err_CS;			/*erro na verificaÃ§o do CHECKSUM(CS)*/
                    }

               /* w command detected && entre [# - !] tiver 5 char -> # ['d' 'w' '2' '0' 'CS'] ! */ 
            if(message.buffer[message.tail][indx_SOF+2]=='w' && (indx_EOF-indx_SOF)==6) 
            { 	
                /*ATUALIZAR a estrutura da RTDB referente aos LEDS*/ 
                if(message.buffer[message.tail][indx_SOF+3]=='0')           /*LED_0*/
                {
                    if(message.buffer[message.tail][indx_SOF+4]=='0')
                    {
                        ledRtdb.n0=0;         
                    }else{
                        ledRtdb.n0=1;
                        }
                }
                if(message.buffer[message.tail][indx_SOF+3]=='1')           /*LED_1*/
                {
                    if(message.buffer[message.tail][indx_SOF+4]=='0')
                    {
                        ledRtdb.n1=0;         
                    }else{
                        ledRtdb.n1=1;
                        }
                }
                if(message.buffer[message.tail][indx_SOF+3]=='2')           /*LED_2*/
                {
                    if(message.buffer[message.tail][indx_SOF+4]=='0')
                    {
                        ledRtdb.n2=0;         
                    }else{
                        ledRtdb.n2=1;
                        }
                }
                if(message.buffer[message.tail][indx_SOF+3]=='3')           /*LED_3*/
                {
                    if(message.buffer[message.tail][indx_SOF+4]=='0')
                    {
                        ledRtdb.n3=0;         
                    }else{
                        ledRtdb.n3=1;
                        }
                }

                /**/

            }//if(message.buffer[message.tail][indx_SOF+2] == 'w' && (indx_EOF-indx_SOF)== 6)

            /*LER a RTDB e atualizar a informação para o utilizador*/
            /* r command detected && entre [# - !] tiver 5 char -> # ['d' 'r' '2' '0' 'CS'] ! */     
            if(message.buffer[message.tail][indx_SOF+2] == 'r' && (indx_EOF-indx_SOF)== 6)
            {
                /*ATUALIZAR a estrutura da RTDB referente aos LEDS*/ 
                if(message.buffer[message.tail][indx_SOF+3]=='0')           /*BOTAO_0*/
                {
                    if(butRtdb.n0==1)
                    {
                      printk("Botão N_0 -> ON \n\r");  
                    }else{
                            printk("Botão N_0 -> OFF \n\r");
                         }
                }

                if(message.buffer[message.tail][indx_SOF+3]=='1')           /*BOTAO_1*/
                {
                    if(butRtdb.n1==1)
                    {
                      printk("Botão N_1 -> ON \n\r");  
                    }else{
                            printk("Botão N_1 -> OFF \n\r");
                         }
                }

                if(message.buffer[message.tail][indx_SOF+3]=='2')           /*BOTAO_2*/
                {
                    if(butRtdb.n2==1)
                    {
                      printk("Botão N_2 -> ON \n\r");  
                    }else{
                            printk("Botão N_2 -> OFF \n\r");
                         }
                }

                if(message.buffer[message.tail][indx_SOF+3]=='3')           /*BOTAO_3*/
                {
                    if(butRtdb.n3==1)
                    {
                      printk("Botão N_3 -> ON \n\r");  
                    }else{
                            printk("Botão N_3 -> OFF \n\r");
                         }    
                }               

            } //if(message.buffer[message.tail][indx_SOF+2] == 'r' && (indx_EOF-indx_SOF)== 6)

        }//if( message.buffer[message.tail][indx_SOF+1] == 'd') 
        printk("------------------------------------------------------------------->SMS a processada....\n");

    }else{
            //printk("------------------------------------------------------------------->N exite ordens a processar\n");
            
         }

    return ret;
}//fim de int readSmsInBuf(void)