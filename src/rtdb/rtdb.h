/**
 * @file rtdb.h
 * 
 * @brief Real Time Data Base functions
 * 
 * @version 1.0
 * 
 * @date 2021-06-06
 * 
 * @author Fernando Gandarinho e Tomás Silva
*/
#ifndef RTDB_H_
#define RTDB_H_

/**
 * @brief Tamanho Maximo do Buffer de comandos
*/
#define BUFF_NUM_MAX_MSN 4      /*Tamanho do Buffer num maximo de mensagens armagedas em simultaneo*/

/**
 * @brief Tamanho máximo de cada comando
*/
#define LENGTH_MAX_MSN 20       /*Cumpirmento máximo de cada mensagem a ser armazenada ex.20Bytes(chars)*/

/**
 * @brief Caracter de início de comando
*/
#define SOF_SYM '#'	          /* Start of Frame Symbol */

/**
 * @brief Caracter de final de comando
*/
#define EOF_SYM '!'           /* End of Frame Symbol */

/**
 * @brief Sucesso
*/
#define Valid 0

/**
 * @brief Erro de comando Vazio
*/
#define Err_Empty_str -1

/**
 * @brief Erro do comando inválido
*/
#define Err_Invalid_cmd -2

/**
 * @brief Erro de CheckSum no comando
*/
#define Err_CS -3

/**
 * @brief Erro de formato de comando
*/
#define Err_str_format -4

/**
 * @brief Erro de temperatura
*/
#define Err_val_temperature -5


/*estrutura que contem o estado dos botoes*/
/**
 * @brief Estrutura de estado dos botões
*/
struct state_but
{
    _Bool n0;
    _Bool n1;
    _Bool n2;
    _Bool n3;
    _Bool busy;
};

/*estrutura para armaenar o estado dos Leds*/
/**
 * @brief Estrutura de estado dos leds
*/
struct state_led
{
    _Bool n0;
    _Bool n1;
    _Bool n2;
    _Bool n3;
    _Bool busy;
};

/*
 * Estrutura que armazena a temperatura lida pelo sensor TC47
 * E armazena o SetPoint defenido pelo utilizador
 *  
*/

/**
 * @brief Estrutura de dados da temperatura
*/
struct temp
{
    int graus;
    unsigned char set_point;

};

/*
 * Estrutura que armazema os valores dos periodos dos threads
 * Estes valores são apresentados/escritos em ms-> miliSegundos
*/

/**
 * @brief Estrutura dos periodos das Threads
*/
struct periodo
{
    unsigned int thread_A;
    unsigned int thread_B;
    unsigned int thread_D;

};

/*função que inicializa a estruitua do estado dos botões*/
/**
 * @brief Função que inicializa a estrutura do estado dos botões
*/
void initStateButs(void);

/*função que atualiza o estado dos botões a RTDB*/
/**
 * @brief Função que atualiza o estado dos botões na RTDB
 * 
 * @param[in] bt Estrutura de estado dos botões
 * 
*/
void writeButsInRtdb(struct state_but bt);

/*função que inicializa a estrutuua do estado dos LEDS*/
/**
 * @brief Função que inicializa a estrutura do estado dos LEDS
*/
void initStateLeds(void);

/*função que atualiza o estado dos LEDS a RTDB*/
/**
 * @brief Função que atualiza o estado dos LEDs na RTDB
 * 
 * @param[out] state_led Estrutura de estado dos LEDs
 * 
*/
struct state_led writeLedsInRtdb(void);

/*
 * Inicializa o buffer
 * coloca a tail head e count com os valores corretos
*/
/**
 * @brief Função que inicializa o buffer de comandos
 * 
 * @note Nesta função é inicializada a tail, o head e o count do buffer
*/
void initMsnBuffer(void);

/*
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
/**
 * @brief Função que processa os comandos
 * 
 * @param[in] ptr_sms Ponteiro para o comando
 * @param[in] buf_size Tamanho do buffer
 * @param[out] CMD Comando a executar
 * 
*/
int readSmsInBuf(unsigned char *ptr_sms, int buf_size);


/*
 * Inicializa a estrutura referida ao Buffer
 * Invocada no inicio do main
*/
/**
 * @brief Inicialização do sensor de temperatura na RTDB
*/
void initTempInRtdb(void);



/*
 * Esta função tem como objetivo principal escrever a temperatura recebida da RTDB
 * Esta função é invocada pela trhead_D 
*/
/**
 * @brief Escrita da temperatura na RTDB
 * 
 * @param[in] data Temperatura
*/
void writeTempInRtdb(unsigned char *data);


/*
 * Esta função tem como objetivo principal escrever o SetPoint temperatura recebida do teclado(usart)
 * Esta função é invocada pela trhead_C 
*/
/**
 * @brief Escrita do SetPoint tendo em conta o comando executar na RTDB
 * 
 * @param[in] data SetPoint da Temperatura
*/
void writeSetPointTempInRtdb(unsigned char *data);

/*
 * Inicia a estrutura com os perido dos threads defenidos no mais no #defines
 * parametros de entrada 
*/
/**
 * @brief Inicialização dos periodos das Threads
 * 
 * @param[in] a Periodo da Thread A
 * @param[in] b Periodo da Thread B
 * @param[in] d Periodo da Thread D
*/
void initPeridosTheads(int a, int b, int d);


/*
 * Lê os peridos dos trheas existentes na RTDB e devolve o se valor na main()
 * entrada o nome do thread a considerar
 * saida o valor do perido do tread sinalizado no parametor de entrada valor armaenado na RTDB
*/
/**
 * @brief Leitura dos periodos das Threads
 * 
 * @param[in] letra_do_thread Thread solicitada
 * @param[out] Periodo Periodo da Thread solicitada
*/
unsigned int readPeriodThread(unsigned char letra_do_thread);


/*
 * Escreve na RTDB novos valores para os Periodos das threads
 * Parametro de entrada
 * nome da thread; valor do Periodo
 *  
*/
/**
 * @brief Definição do periodo da Thread
 * @param[in] letra_do_thread Thread solicitada
 * @param[in] values_periodo Periodo a configurar na Thread solicitada
*/
void writPeriodThread(unsigned char letra_do_thread, unsigned int value_periodo);

#endif



