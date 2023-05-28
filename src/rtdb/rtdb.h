#ifndef RTDB_H_
#define RTDB_H_

#define BUFF_NUM_MAX_MSN 5      /*Tamanho do Buffer num maximo de mensagens armagedas em simultaneo*/
#define LENGTH_MAX_MSN 20       /*Cumpirmento máximo de cada mensagem a ser armazenada ex.20Bytes(chars)*/
#define SOF_SYM '#'	          /* Start of Frame Symbol */
#define EOF_SYM '!'           /* End of Frame Symbol */

#define Valid 0
#define Err_Empty_str -1
#define Err_Invalid_cmd -2
#define Err_CS -3
#define Err_str_format -4

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

/*estrutura que armazena as mensagens vindas do teclado USART*/
struct msn
{
    unsigned char buffer [BUFF_NUM_MAX_MSN] [LENGTH_MAX_MSN];
    int tail;
    int head;
    int count;
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
 * função que escreve no buffer as mensagens escritas no teclado ter em atenção o protocolo a seguir apresentado
 * Entra um ponteiro para o inicio da string escrita
 * as tramas que podem ser esvrita pelo utilizador 
 *    [#][d][w][0-4][0/1][CS][!]        ->escrever nas saida digitais LEDS
 *    [#][d][r][0-4][0/1][CS][!]           ->Ler as entradas digitais Botões    
 *    [#][a][w][addr][0-512][CS][!]     ->escrever set poit para sensor temperatura Ex.: 234-> 23.4ºC    
 *    [#][a][r][addr][0-512][CS][!]            ->Ler a temperatura do sensor Es.: 503 -> 50.3ºC   
*/
void writeSmsInBuf(unsigned char *ptr_sms);

/**
 * Função que processa as informações pedidas pelo utilizador
 * Estas informações estão armazenadas numa lista tipo FIFO (struct msn)
 * É verificada se existe novas mensagens verificada os códigos de erro
 * é atualizada a RTDB, para que os outros threads possa fazer o seu trabalho.
 * Retorna um valor a informar sobre o estado da execução
 * 
 * As tramas que podem ser Precessadas pelo sistema têm de ter o seguinte formato 
 *    [#][d][w][0-4][0/1][CS][!]        ->escrever nas saida digitais LEDS
 *    [#][d][r][0-4][0/1][CS][!]           ->Ler as entradas digitais Botões    
 *    [#][a][w][addr][0-512][CS][!]     ->escrever set poit para sensor temperatura Ex.: 234-> 23.4ºC    
 *    [#][a][r][addr][0-512][CS][!]            ->Ler a temperatura do sensor Es.: 503 -> 50.3ºC  
*/
int readSmsInBuf(void);

#endif



