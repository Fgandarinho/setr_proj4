#ifndef RTDB_H_
#define RTDB_H_

#define BUFF_NUM_MAX_MSN 4      /*Tamanho do Buffer num maximo de mensagens armagedas em simultaneo*/
#define LENGTH_MAX_MSN 20       /*Cumpirmento máximo de cada mensagem a ser armazenada ex.20Bytes(chars)*/
#define SOF_SYM '#'	          /* Start of Frame Symbol */
#define EOF_SYM '!'           /* End of Frame Symbol */

#define Valid 0
#define Err_Empty_str -1
#define Err_Invalid_cmd -2
#define Err_CS -3
#define Err_str_format -4
#define Err_val_temperature -5

/*estrutura que contem o estado dos botoes*/
struct state_but
{
    _Bool n0;
    _Bool n1;
    _Bool n2;
    _Bool n3;
    _Bool busy;
};
/*estrutura para armaenar o estado dos Leds*/
struct state_led
{
    _Bool n0;
    _Bool n1;
    _Bool n2;
    _Bool n3;
    _Bool busy;
};

/**
 * Estrutura que armazena a temperatura lida pelo sensor TC47
 * E armazena o SetPoint defenido pelo utilizador
 *  
*/
struct temp
{
    int graus;
    unsigned char set_point;

};

/**
 * Estrutura que armazema os valores dos periodos dos threads
 * Estes valores são apresentados/escritos em ms-> miliSegundos
*/
struct periodo
{
    unsigned int thread_A;
    unsigned int thread_B;
    unsigned int thread_D;

};

/*função que inicializa a estruitua do estado dos butões*/
void initStateButs(void);

/*função que atualiza o estado dos botões a RTDB*/
void writeButsInRtdb(struct state_but bt);

/*função que inicializa a estrutuua do estado dos LEDS*/
void initStateLeds(void);

/*função que atualiza o estado dos LEDS a RTDB*/
struct state_led writeLedsInRtdb(void);

/**
 * Inicializa o buffer
 * coloca a tail head e count com os valores corretos
*/
void initMsnBuffer(void);

/**
 * Função que processa as informações pedidas pelo utilizador
 * Estas informações estão armazenadas numa lista tipo FIFO (struct msn)
 * É verificada se existe novas mensagens verificada os códigos de erro
 * é atualizada a RTDB, para que os outros threads possa fazer o seu trabalho.
 * Retorna um valor a informar sobre o estado da execução
 * Esta função é invocada pela Thread_C 
 * As tramas que podem ser Precessadas pelo sistema têm de ter o seguinte formato 
 *    [#][d][w][0-4][0/1][CS][!]        ->escrever nas saida digitais LEDS
 *    [#][d][r][0-4][0/1][CS][!]           ->Ler as entradas digitais Botões    
 *    [#][a][w][addr][0-512][CS][!]     ->escrever set poit para sensor temperatura Ex.: 234-> 23.4ºC    
 *    [#][a][r][addr][0-512][CS][!]            ->Ler a temperatura do sensor Es.: 503 -> 50.3ºC  
*/
int readSmsInBuf(unsigned char *ptr_sms, int buf_size);


/**
 * Inicializa a estrutura referida ao Buffer
 * Invocada no inicio do main
*/
void initTempInRtdb();



/**
 * Esta função tem como objetivo principal escrever a temperatura recebida da RTDB
 * Esta função é invocada pela trhead_D 
*/
void writeTempInRtdb(unsigned char *data);


/**
 * Esta função tem como objetivo principal escrever o SetPoint temperatura recebida do teclado(usart)
 * Esta função é invocada pela trhead_C 
*/
void writeSetPointTempInRtdb(unsigned char *data);
/**
 * Inicia a estrutura com os perido dos threads defenidos no mais no #defines
 * parametros de entrada 

*/
void initPeridosTheads(int a, int b, int c, int d);


/**
 * Lê os peridos dos trheas existentes na RTDB e devolve o se valor na main()
 * entrada o nome do thread a considerar
 * saida o valor do perido do tread sinalizado no parametor de entrada valor armaenado na RTDB
*/
unsigned int readPeriodThread(unsigned char letra_do_thread);


/**
 * Escreve na RTDB novos valores para os Periodos das threads
 * Parametro de entrada
 * nome da thread; valor do Periodo
 *  
*/
void writPeriodThread(unsigned char letra_do_thread, unsigned int value_periodo);

#endif



