#include "rtdb.h"                 
#include <zephyr/sys/printk.h>
#include <inttypes.h>

/*inicializar a estrutura para os estado dos botões na RTDB*/
static struct state_but butRtdb;            /*estrutura de dados que armazena o estado dos botoes*/
static struct state_led ledRtdb;            /*estrutura de dados que armazena o estado dos LEds*/
volatile static struct msn message;         /*Estrutura de dados que armazena a informaçao recebida pelo utilizador(USART)*/
static struct temp temperatura;             /*Estrutura que armazena os dados relativos ao sensore de temnperatura TC47*/   


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
 * Actulisa a estrura butRtdb do exterior para a rtdb
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
   //printk("------------------------>Buffer inicializado %c \n", message.buffer[3][3]); 
}//fim de void initMsnBuffer(void)


/**
 * Lê as mensagens no buffer e executa-as
 * Não é necessário nenhum parametro de entrada pois os Buffer esté criado dentro dete módulo rtdb.c/rtdb.h
 * Saidas:  *Valid ->0 || Err_Empty_str ->1 || Err_Invalid_cmd -> -2 || Err_CS ->-3 || Err_str_format -> -4
 */
int readSmsInBuf(unsigned char *ptr_sms, int buf_size)
{
    int ret=Valid;                                  /*variavel de retorno parte do principio que está tudo bem!!!*/
    int indx_SOF=0, numbs_EOF=0;
	int indx_EOF=0, numbs_SOF=0;
	unsigned char CS=0;
    unsigned char buf_local[buf_size];
  
    for(int i=0; i<buf_size; i++)               /*copia a string para um array de caracteres*/
    {
        buf_local[i]=*ptr_sms;
        printk("%c,",buf_local[i]);
        ptr_sms++;
    }
    CS = buf_local[indx_SOF+1]+buf_local[indx_SOF+2]+buf_local[indx_SOF+3]+buf_local[indx_SOF+4];
    printk("-> %c\n\r",CS);
    
        /*Encontrar o indice de EOF e o SOF*/
	    for(int i=0; i<buf_size; i++)
	    {
	    	if(buf_local[i] == SOF_SYM)
	    	{
	    		if(numbs_SOF==0)		/*So atribui o indice ao primeiro SOF encontrado*/		
	    		{
	    			indx_SOF=i;			
	    		}
	    		numbs_SOF++;	
	    	}
    
    		if(buf_local[i] == EOF_SYM)
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
         if((indx_EOF-indx_SOF) < 6)
         {
            printk("errooor -> %d",Err_Empty_str);
            return Err_Empty_str;
         }   
            

         if(buf_local[indx_SOF+1] !='a' || buf_local[indx_SOF+2] != 'w')   /*não verificar o CS no caso de escrever a temperatura*/
         {
            /*calcular o CS*/
            CS = buf_local[indx_SOF+1]+buf_local[indx_SOF+2]+buf_local[indx_SOF+3]+buf_local[indx_SOF+4];
            /*Verficar o CS Enviado com o recebido*/			
            if(buf_local[indx_SOF+5] != CS) /*verificar se o CS enviado e igual ao CS calculado no local*/
            {
                return Err_CS;			/*erro na verificaÃ§o do CHECKSUM(CS)*/
            }
         }


        if(buf_local[indx_SOF+1]=='d')
        {
                    /* w command detected */
                if(buf_local[indx_SOF+2]=='w')
                { 	
                    /*ATUALIZAR a estrutura da RTDB referente aos LEDS*/ 
                    if(buf_local[indx_SOF+3]=='0')           /*LED_0*/
                    {
                        if(buf_local[indx_SOF+4]=='0')
                        {
                            ledRtdb.n0=0;                   
                        }else{
                            ledRtdb.n0=1;
                            }
                    }
                    if(buf_local[indx_SOF+3]=='1')           /*LED_1*/
                    {
                        if(buf_local[indx_SOF+4]=='0')
                        {
                            ledRtdb.n1=0;         
                        }else{
                            ledRtdb.n1=1;
                            }
                    }
                    if(buf_local[indx_SOF+3]=='2')           /*LED_2*/
                    {
                        if(buf_local[indx_SOF+4]=='0')
                        {
                            ledRtdb.n2=0;         
                        }else{
                            ledRtdb.n2=1;
                            }
                    }
                    if(buf_local[indx_SOF+3]=='3')           /*LED_3*/
                    {
                        if(buf_local[indx_SOF+4]=='0')
                        {
                            ledRtdb.n3=0;         
                        }else{
                            ledRtdb.n3=1;
                            }
                    }

                        /**/

                }//if(buf_local[indx_SOF+2] == 'w')

                    
                /* r command detected -> LER a RTDB e atualizar a informação para o utilizador */     
                if(buf_local[indx_SOF+2] == 'r')
                {
                    /*ATUALIZAR a estrutura da RTDB referente aos LEDS*/ 
                    if(buf_local[indx_SOF+3]=='0')           /*BOTAO_0*/
                    {
                        if(butRtdb.n0==1)
                        {
                            printk("Botao N_0 -> ON \n\r");  
                        }else{
                                printk("Botao N_0 -> OFF \n\r");
                                }
                    }

                    if(buf_local[indx_SOF+3]=='1')           /*BOTAO_1*/
                    {
                        if(butRtdb.n1==1)
                        {
                            printk("Botao N_1 -> ON \n\r");  
                        }else{
                                printk("Botao N_1 -> OFF \n\r");
                                }
                    }

                    if(buf_local[indx_SOF+3]=='2')           /*BOTAO_2*/
                    {
                        if(butRtdb.n2==1)
                        {
                            printk("Botao N_2 -> ON \n\r");  
                        }else{
                                printk("Botao N_2 -> OFF \n\r");
                                }
                    }

                    if(buf_local[indx_SOF+3]=='3')           /*BOTAO_3*/
                    {
                        if(butRtdb.n3==1)
                        {
                            printk("Botao N_3 -> ON \n\r");  
                        }else{
                                printk("Botao N_3 -> OFF \n\r");
                                }    
                    }               

                } //if(buf_local[indx_SOF+2] == 'r')

        } //if(buf_local[indx_SOF+1]=='d')        

        /* a command detected -> LER a RTDB e atualizar a informação para o utilizador */     
        if(buf_local[indx_SOF+1]=='a')
        {
                    /* r command detected -> LER a RTDB e atualizar a informação para o utilizador */     
                if(buf_local[indx_SOF+2] == 'r')
                {
                    printk("Temperatura Atual= %d \nSet_Point= %d \n\r ",temperatura.graus,temperatura.set_point);
                }

                    /* w command detected -> recebe a info do utilizador e atualiza a RTDB */ 
                   
                if(buf_local[indx_SOF+2] == 'w' ) // comand w && valor inserido está entre [00 - 99]
                {
                    
                    unsigned char unid, dec, temp;
                    unid= buf_local[indx_SOF+4]-48;
                    dec= buf_local[indx_SOF+3]-48;
                    temp=dec*10+unid;
                    if(temp>=0 && temp<=99 )
                    {
                        writeSetPointTempInRtdb(&temp); 
                        printk("SetPoint inserida:%d\n\r",temp); 
                        
                    }else
                        {
                            return Err_val_temperature;       
                        }
                      
                }
                if(buf_local[indx_SOF+2] == 'd' )           /*MOSTRA TODOS OS VALORES PRESENTES NA rtdb*/
                {
                    printk("LED_0:%d\n\r",ledRtdb.n0);
                    printk("LED_1:%d\n\r",ledRtdb.n1); 
                    printk("LED_2:%d\n\r",ledRtdb.n2); 
                    printk("LED_3:%d\n\r",ledRtdb.n3);

                    printk("Bot_0:%d\n\r",butRtdb.n0);
                    printk("Bot_1:%d\n\r",butRtdb.n1);
                    printk("Bot_2:%d\n\r",butRtdb.n2);
                    printk("Bot_3:%d\n\r",butRtdb.n3); 

                    printk("Temp: %d\n\r",temperatura.graus);
                    printk("Set.Point: %d\n\r",temperatura.set_point);
                } 

        }



       return ret;
}//fim de int readSmsInBuf(void)

void initTempInRtdb()
{
    temperatura.graus=0;
    temperatura.set_point=0;
}

/**
 * Função que escreve na estrutura da tempertaura a temperatura do sensor
 * Função incocada pela thread_D
*/
void writeTempInRtdb(unsigned char *data)
{
   //printk("Temperature Reading a armazenada a RTDB: %d\n",*data);
   temperatura.graus=*data;
}

/**
 * Função que escreve na estrutura da tempertaura o SetPoint vindo do utilizador
 * Função incocada pela thread_C
*/
void writeSetPointTempInRtdb(unsigned char *data)
{
    temperatura.set_point=*data;
}

