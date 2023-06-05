#include <unity.h>
#include "../src/rtdb/rtdb.h"

#define buf_size 60					/*defenir o tamanho de um bufLocal para testes locais*/

unsigned char sms[buf_size];		/*buf_local*/

void setUp(void)
{
	return;
}
void tearDown(void)
{
	return;
}

/* Constroi a sms dentro deste módulo */
void insertStrg(const char *ptr_string, int size)
{
	/*apaga o buffer local*/
	int i;
	for(i=0;i<size; i++)
	{
		sms[i]=0;
	}

	/*insere a string no buf_local*/
	int j;
	for(j=0;j<size; j++)
	{
		sms[j]=*ptr_string;
		ptr_string++;
	
	}

}


void test_readSmsInBuf_Returns_vall_Ok(void)
{
    insertStrg("# !",3);	
	TEST_ASSERT_EQUAL_INT(Err_Empty_str,readSmsInBuf(&sms[0], buf_size)); 				/*STRING VAZIA*/
	
	insertStrg("! #",3);	
	TEST_ASSERT_EQUAL_INT(Err_str_format,readSmsInBuf(&sms[0], buf_size)); 				/*Formato Errado*/

    insertStrg("234#A123S0",10);														/*Formato Errado->testa um cmd invalido*/
	TEST_ASSERT_EQUAL_INT(Err_str_format, readSmsInBuf(&sms[0], buf_size));

	
	insertStrg("dr006!",5); 															/*Formato Errado-> testa um cmd sem SOF*/
	TEST_ASSERT_EQUAL_INT(Err_str_format, readSmsInBuf(&sms[0], buf_size));


	insertStrg("dw00!",5);
	TEST_ASSERT_EQUAL_INT(Err_str_format, readSmsInBuf(&sms[0], buf_size));				/*Erro no formato da frame*/
	
	insertStrg("#dr107!",7);
	TEST_ASSERT_EQUAL_INT(Valid, readSmsInBuf(&sms[0], buf_size));						/*Formato CERTO-> testa um cmd correto com dr*/
	
	insertStrg("#dw00;!",7);
	TEST_ASSERT_EQUAL_INT(Valid,  readSmsInBuf(&sms[0], buf_size));						/*FORMATO CERTO -> testa um cmd correto com dw*/


    insertStrg("#dr105!",7);
	TEST_ASSERT_EQUAL_INT(Err_CS,  readSmsInBuf(&sms[0], buf_size));					/*Erro no checkSum-> testa um cmd correto com P*/
}


int main(void)
{
	UNITY_BEGIN();
	
	RUN_TEST(test_readSmsInBuf_Returns_vall_Ok);
		
	return UNITY_END();
}
