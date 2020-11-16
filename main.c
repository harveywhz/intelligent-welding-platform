#include "embARC.h"
#include "embARC_debug.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include "uart.h"

#define uchar unsigned char

static DEV_UART *port1_uart;
static DEV_UART *port2_uart;
DEV_GPIO_PTR BEEP = NULL;
DEV_GPIO_PTR HUO = NULL;
DEV_GPIO_PTR yanwu = NULL;
DEV_GPIO_PTR hongwai = NULL;
DEV_GPIO_PTR jidianqi = NULL;
DEV_GPIO_PTR power = NULL;
uint32_t huodata;
uint32_t yanwudata;
uint32_t hongwaidata;
uint32_t baudrate = 115200;

char at1[]="\r\nAT\r\n";              //�ж�AT�Ƿ����
char at2[]="AT+CIMI?\r\n";         //��ѯ IMSI ����
char at3[]="AT+CPWROFF\r\n";     //�ر�l610
char at4[]="AT+CFUN=1\r\n";       //�����ģʽ
char at5[]="AT+CSQ?\r\n";       //��ѯ��ǰ�ź�ǿ�ȣ���ʱ 3 ��
char at6[]="AT+CPIN?\r\n";      //��� SIM ��״̬����ʱ 3 �롣�յ�+CPIN: READY OK �������һ��
char at7[]="AT+CPMS=\"SM\"\r\n";  //��������ѡ����Ŵ洢�� SIM ����
char at8[]="AT+CMGF=1\r\n";     //���ö���ģʽΪPDUģʽ(0)��TXTģʽ(1)
char at9[]="at+cloudauth=\"a1gJX4Iypsu\",\"l610\",\"3f7a9cece31819bae4c11c7649554670\",\"iot-auth.aliyun.com\"\r\n";
char at10[]="at+cloudconn=80,0,4\r\n";//���Ӱ�����������ƽ̨
char at11[]="AT+MIPCALL?\r\n";  //��ѯ�������ڲ�ѯ��ǰ�Ƿ��Ѿ���ȡ�� IP
char at12[]="AT+CLOUDDISCONN\r\n";//�Ͽ� MQTT ���Ӳ�����Դ�ͷ�
char at13[]="AT+CFUN=0\r\n";    //������С����ģʽ
char at14[]="AT+CLOUDSUB=\"/a1gJX4Iypsu/l610/user/get\",0\r\n"; //����
char at15[]="AT+CGDCONT=1,\"IP\",\"APN\"\r\n";//���� APN
char at16[]="ATE0\r\n";          //�رջ���
char at17[]="AT+CSMP=17,167,0,0\r\n";//���ö����ı�ģʽ����
char at18[]="AT+CSCA?\r\n";     //��ѯ����+CSCA: "+8613800200569",145 OK 
char at19[]="AT+CMGR=1\r\n";    //��ȡ���Ϊ1�Ķ���
char atre[100];
char getdata[100];
void huo();
void beep();
void yan();
void hong();
void ji();
int waiting(char *A, char *B, char *C);
void uart_tts_init();
void tts(char *text);
void uart_l610_init();
void l610_send(char *AT);
int l610_open();
void updata(char *upda);
void SMS(char *mess);

int main(void)
{
    huo();
	beep();
	yan();
	hong();
	ji();
	uart_tts_init();
    uart_l610_init();
    printf("readying\r\n");
    if(l610_open()) printf("��ʧ��\r\n��);
    else printf("success\r\n");
    while(1){
		port1_uart->uart_read(getdata, 100);	//���հ�����������ƽ̨������������
		if(waiting(atre, "shutdown", "open"))
		{
			jidianqi->gpio_write(1<<4, 1<<4);
			tts("���յ��ر�����");
		}
		else{
			jidianqi->gpio_write(0, 1<<4);
			tts("���յ���������");
		}
		BEEP->gpio_write(1,1<<0);
        HUO->gpio_read(&huodata, 1<<1);
        yanwu->gpio_read(&yanwudata, 1<<2);
        hongwai->gpio_read(&hongwaidata, 1<<3);
        if(huodata==0||yanwudata==0){			//����⵽��������ʱ
            BEEP->gpio_write(0,1<<0);
            board_delay_ms(500, 1);           
            jidianqi->gpio_write(1<<4,1<<4);	//�Ͽ���Դ
            tts("Σ�գ���⵽����");
            printf("updata\r\n");
            updata("danger");//�ϴ�Σ����Ϣ
			board_delay_ms(5000, 1);
            SMS("0011640B815166315553F70008AA1068C06D4B52306709706B707E53D1751F");//���;������
        }
        else{		
            if(hongwaidata==0){
                BEEP->gpio_write(1, 1<<0);
                jidianqi->gpio_write(0, 1<<4);
            }
            else{
                BEEP->gpio_write(0, 1<<0); 		//���뿪ʱ�Զ��رպ�̨��������ʾ��Ϣ
                board_delay_ms(500, 1);
                jidianqi->gpio_write(1<<4, 1<<4);
                board_delay_ms(500, 1);
                tts("�ص����ص���һ��Ҫ�ص�");
                printf("updata\r\n");
                updata("leave");
				board_delay_ms(5000, 1);
            }	    
        }
    }
    return E_SYS;
}
void tts(char *text){
    uint32_t  length;
    length = strlen(text);
    char ecc[1] ={0x00};
    char head[5]={0xfd,0x00,0x00,0x01,0x01};
    head[2] = length + 3;
    for(int i=0; i<5; i++){  
        ecc[0]=ecc[0]^(head[i]);  
    }
    for(int i=0; i<length; i++){  
        ecc[0]=ecc[0]^(text[i]);
    }
    port2_uart->uart_write(head, 5);
    port2_uart->uart_write(text, length);
    port2_uart->uart_write(ecc, 1);
}
void l610_send(char *AT){
    port1_uart->uart_write(AT, strlen(AT));
}
int waiting(char *A, char *B, char *C){
    for (int i=0;i<20;i++) {
        board_delay_ms(500, 1);
        if ((strstr(A, B) != NULL)||(strstr(A, C) != NULL)) {
            if ((strstr(A, B) != NULL))
                return 0;
            else
                return 1;
        }
    }
    printf("time out\r\n");
	return 1;
}
int l610_open(){
    int rd_avail = 0;
    power->gpio_write(1<<5,1<<5);
    board_delay_ms(2000, 1);
    power->gpio_write(0,1<<5);
    printf("AT\r\n");
    board_delay_ms(3000, 1);
    for(int i=0;i<4;i++)
    {
        l610_send(at1);
        board_delay_ms(1000, 1);
		port1_uart->uart_read(atre, 4);
		if(waiting(atre, "OK", "ERROR")==0) break;
    }
    l610_send(at16);
    l610_send(at9);
    board_delay_ms(1000, 1);
    l610_send(at12);
    board_delay_ms(500, 1);
    l610_send(at10);
    board_delay_ms(3000, 1);
    port1_uart->uart_read(atre, 10);
    if(waiting(atre, "CLOUDAUTH: OK", "CLOUDCONN: FA")) return 1;
    return 0;
}
void updata(char *upda){
    char atup[100]="AT+CLOUDPUB=\"/a1gJX4Iypsu/l610/user/update\",0,\"";
    strcat(atup, upda);
    strcat(atup, "\"\r\n");
    l610_send(atup);
}//�ϴ�����
void SMS(char *mess){
    char atms[] = "AT+CMGS=30\r\n";
    strcat(mess, "\x1A");
    l610_send(atms);
    l610_send(mess);
}//���Ͷ���
void uart_tts_init(){
	io_arduino_config_uart(IO_PINMUX_ENABLE);
    port2_uart = uart_get_dev(2);
	port2_uart->uart_open(9600);
	port2_uart->uart_control(UART_CMD_SET_BAUD, (void *)(9600));
}
void uart_l610_init(){
    io_arduino_config(ARDUINO_PIN_9, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    io_pmod_config(PMOD_C, PMOD_UART, IO_PINMUX_ENABLE);
    port1_uart = uart_get_dev(DFSS_UART_1_ID);
	port1_uart->uart_open(baudrate);
	port1_uart->uart_control(UART_CMD_SET_BAUD, (void *)(baudrate));
    power = gpio_get_dev(DFSS_GPIO_8B2_ID);
    power->gpio_open(1<<5);
    power->gpio_control(GPIO_CMD_SET_BIT_DIR_OUTPUT, (void *)(1<<5));
}
void beep(){
	io_arduino_config(ARDUINO_PIN_4, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    BEEP = gpio_get_dev(DFSS_GPIO_8B2_ID);
    BEEP->gpio_open(1<<0);   
    BEEP->gpio_control(GPIO_CMD_SET_BIT_DIR_OUTPUT,(void *)(1<<0));
    BEEP->gpio_write(1,1<<0);
}
void huo(){
	io_arduino_config(ARDUINO_PIN_5, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    HUO = gpio_get_dev(DFSS_GPIO_8B2_ID);
    HUO->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT,(void *)(1<<1));
}
void yan(){
	io_arduino_config(ARDUINO_PIN_6, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    yanwu = gpio_get_dev(DFSS_GPIO_8B2_ID);
    yanwu->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT,(void *)(1<<2));
}
void hong(){
	io_arduino_config(ARDUINO_PIN_7, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    hongwai = gpio_get_dev(DFSS_GPIO_8B2_ID);
    hongwai->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT,(void *)(1<<3));
}
void ji(){
	io_arduino_config(ARDUINO_PIN_8, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    jidianqi = gpio_get_dev(DFSS_GPIO_8B2_ID);
    jidianqi->gpio_open(1<<4);   
    jidianqi->gpio_control(GPIO_CMD_SET_BIT_DIR_OUTPUT,(void *)(1<<4));
}
void man(){
	io_arduino_config(ARDUINO_PIN_11, ARDUINO_GPIO, IO_PINMUX_ENABLE);
    manf = gpio_get_dev(DFSS_GPIO_8B2_ID);
    manf->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT,(void *)(1<<7));
}