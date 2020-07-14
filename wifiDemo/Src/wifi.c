
#include "wifi.h"

Userdatatype Espdatatype;

UART_HandleTypeDef *wifiCom = &huart1;			// ***��Ҫ����ʵ�ʴ��ڽ����޸�***

uint8_t persent_mode;
bool atFlag = FALSE;
bool rstFlag = FALSE;
bool modeFlag = FALSE;
bool sendReadyFlag = FALSE;
bool sendOkFlag = FALSE;
bool wifiConnectFlag = FALSE;
bool serverConnectFlag = FALSE;
bool serverCreateFlag = FALSE;
bool hotspotFlag =FALSE;
bool mulConFlag = FALSE;
bool dataAnalyzeFlag = TRUE; // �Ƿ�Խ��յ����ݽ��н���

//uint8_t data[512];

/* �û����ݴ�����
* ����������û��������飬���ݳ��ȣ��������ݣ���ַ����
*/
void userToDo(uint8_t *data, uint16_t length, uint16_t con, uint16_t addr)
{
	// write user code here.
}


// �����жϳ�ʼ������
void EnableUsart_IT(void)
{
    __HAL_UART_ENABLE_IT(wifiCom, UART_IT_RXNE);//�������ڽ����ж�
    __HAL_UART_ENABLE_IT(wifiCom, UART_IT_IDLE);//��������1���н����ж�
    __HAL_UART_CLEAR_IDLEFLAG(wifiCom);         //�������1�����жϱ�־λ
    HAL_UART_Receive_DMA(wifiCom,Espdatatype.DMARecBuffer,DMA_REC_SIZE);  //ʹ��DMA����
		HAL_TIM_Base_Start_IT(&htim2);		// ʹ�ܶ�ʱ��
}


// �û��Զ����жϴ�����
void USER_UART_Handler(void)
{
	if(__HAL_UART_GET_FLAG(wifiCom,UART_FLAG_IDLE) ==SET)  //���������ж�
    {
        uint16_t temp = 0;                        
        __HAL_UART_CLEAR_IDLEFLAG(wifiCom);       //�������1�����жϱ�־λ
        HAL_UART_DMAStop(wifiCom);                //�ر�DMA
        temp = wifiCom->Instance->SR;              //���SR״̬�Ĵ���  F0  ISR
        temp = wifiCom->Instance->DR;              //��ȡDR���ݼĴ��� F0  RDR    ��������ж�
        temp = hdma_usart1_rx.Instance->CNDTR;   //��ȡDMA��δ��������ݸ���
        Espdatatype.DMARecLen = DMA_REC_SIZE - temp;           //�ܼ�����ȥδ��������ݸ������õ��Ѿ����յ����ݸ���
        HAL_UART_RxCpltCallback(wifiCom);		  //���ڽ��ջص�����
    }
}

/* wifi��ʼ������
*	���������wifi����ģʽ
*/
void wifiInit(uint8_t MODE)
{
	EnableUsart_IT();
	HAL_Delay(10);
	if(!wifiStart())
		while(1);
	if(MODE == CLIENT)
		clientStart();
	else if(MODE == SERVER)
		serverStart();
	#if(PASSTHROUGH)
		HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIPMODE=1\r\n", 14, 0xFFFF); // ����Ϊ͸����ʽ
		HAL_Delay(20);
		HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIPSEND\r\n", 12, 0xFFFF); // ����͸��
	#endif
}


// �ر�͸��
void closePassThrough(void)
{
	HAL_UART_Transmit(wifiCom, (uint8_t *)"+++", 3, 0xFFFF); // ����͸��
	HAL_Delay(20);
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIPMODE=0\r\n", 14, 0xFFFF); // ����Ϊ͸����ʽ
}

// �ͻ���ģʽ��ʼ��
void clientStart(void)
{
	HAL_Delay(1000);
	//Send_AT_commend("ATE1", "", 100);
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CWMODE?", 10, 0xFFFF); // ��ȡģ�鵱ǰ����ģʽ
	HAL_UART_Transmit(wifiCom, (uint8_t *)"\r\n", 2, 0xFFFF);	
	HAL_Delay(500);
	if(persent_mode != CLIENT)
	{
		if(Send_AT_commend("AT+CWMODE=1", &modeFlag, 100))
				modeFlag = TRUE;
		if(Send_AT_commend("AT+RST", &rstFlag, 500))
				rstFlag = FALSE;
	}	
	HAL_Delay(2000);
	printf("�ͻ���������\r\n");
	if(!wifiConnectFlag)
		Send_AT_commend(strConnect(5, "AT+CWJAP=\"", CLIENT_WIFI_NAME, "\",\"", CLIENT_WIFI_PWD, "\""), &wifiConnectFlag, 5000);
	printf("wifi���ӳɹ�\r\n");
	if(Send_AT_commend(strConnect(4, "AT+CIPSTART=\"TCP\",\"", CLIENT_IP, "\",", CLIENT_PORT), &serverConnectFlag, 3000))
		printf("���������ӳɹ�\r\n");
	return;
	
}

// ������ģʽ��ʼ��
void serverStart(void)
{
	HAL_Delay(1000);
		//Send_AT_commend("ATE1", "", 100);
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CWMODE?", 10, 0xFFFF); // ��ȡģ�鵱ǰ����ģʽ
	HAL_UART_Transmit(wifiCom, (uint8_t *)"\r\n", 2, 0xFFFF);	
	HAL_Delay(500);
	if(persent_mode != SERVER)
	{
		if(Send_AT_commend("AT+CWMODE=2", &modeFlag, 100))
				modeFlag = TRUE;
		if(Send_AT_commend("AT+RST", &rstFlag, 500))
				rstFlag = FALSE;
	}	
	printf("������������\r\n");
	if(Send_AT_commend(strConnect(8, "AT+CWSAP=\"", SERVER_WIFI_NAME, "\",\"", SERVER_WIFI_PASSWORD, "\",", SERVER_CHANNEL, ",", SERVER_ENCRYPTION), &hotspotFlag, 2000))
		printf("wifi�ȵ㴴���ɹ�\r\n");
	#if(MULTIPLE_CON)
		if(Send_AT_commend("AT+CIPMUX=1", &mulConFlag, 200))
			printf("����Ϊ������ģʽ\r\n");
	#else
		if(Send_AT_commend("AT+CIPMUX=0", &mulConFlag, 200))
			printf("����Ϊ������ģʽ\r\n");
	#endif
	if(Send_AT_commend(strConnect(2, "AT+CIPSERVER=1,", SERVER_PORT), &serverCreateFlag, 200))
		printf("�����������ɹ�\r\n");
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIFSR\r\n", 10, 0xFFFF); // ��ȡģ�鱾��IP��ַ
	return;
}

/* �������ݴ�����
*/
void recDataHandle(void)
{
	if(Espdatatype.AtRecFlag == 1)
	{
		if(findStr("AT\r\r\n\r\nOK"))
			atFlag = TRUE;
		else if(findStr("AT+RST") && findStr("ready"))
			rstFlag = FALSE;
		else if(findStr("AT+CWMODE=") && findStr("OK"))
			modeFlag = TRUE;
		else if(findStr("AT+CWMODE?"))
			persent_mode = Espdatatype.AtBuffer[21]-0x30;
		else if(findStr("WIFI CONNECTED"))
			wifiConnectFlag = TRUE;
		else if((findStr("AT+CIPSTART") && findStr("CONNECT") && findStr("OK")) || findStr("ALREADY CONNECTED"))
			serverConnectFlag = TRUE;
		else if(findStr("AT+CIPSEND") && findStr("OK") && findStr(">"))
			sendReadyFlag = TRUE;
		else if(findStr("SEND OK"))
			sendOkFlag = TRUE;
		else if(findStr("AT+CWSAP=") && findStr("OK"))
			hotspotFlag = TRUE;
		else if(findStr("AT+CIPSERVER") && findStr("OK"))
			serverCreateFlag = TRUE;
		else if(findStr("AT+CIPMUX=1") && findStr("OK"))
			mulConFlag = TRUE;
		else if(findStr("AT+CIPMUX=0") && findStr("OK"))
			mulConFlag = TRUE;
		else if(findStr("AT+CIPMUX") && findStr("link") && findStr("builded"))
			mulConFlag = TRUE;
		else if(findStr("AT+CIFSR") && findStr("OK"))
			;
		//else if(findStr(""))
		#if(DISPLAY_AT_INFO)
				printf((uint8_t *)Espdatatype.AtBuffer);
		#endif
		memset(Espdatatype.AtBuffer, 0, AT_BUFF_SIZE);
		Espdatatype.AtRecFlag = 0;
		Espdatatype.AtRecLen = 0;
	}
	if(Espdatatype.UserRecFlag == 1)
	{
		//printf("���յ����ݣ�%s\r\n", Espdatatype.UserBuffer);
		recDataAnalyze(Espdatatype.UserBuffer);
		memset(Espdatatype.UserBuffer, 0, USER_BUFF_SIZE);
		Espdatatype.UserRecFlag = 0;
		Espdatatype.UserRecLen = 0;
	}
}


/*���ݽ�������
*�������: �û����ݣ��������ݳ���
*�����յ����Է����������ݽ��н������õ����õĲ���
*����У��͵�У�鷽ʽ��~(������+��ַ��+�û�����) + (������+��ַ��+�û�����) = 0xff
*���Դ�������csֵ������򡢵�ַ���û�������������ټ�1���ӦΪ0
*/
void recDataAnalyze(uint8_t *recData)
{
	uint16_t counter = 0, length = 0, con = 0, addr = 0;
	uint8_t cs = 0;
	if(dataAnalyzeFlag) // Ĭ�϶Խ��յ������ݽ��н���
		while(recData[counter] != NULL)	// �����֡����
		{
			cs = 0;
			if(recData[counter] != 0x68)
				return;
			length = recData[++counter];
			length <<=  8;
			length += recData[++counter];
			uint8_t *userData = (uint8_t *)malloc(sizeof(uint8_t)*(length-4));
			if(recData[counter+1] != 0x68 || recData[counter + length + 3] != 0x16)	// �жϵڶ���֡ͷ��֡β�Ƿ���ȷ
				return;
			counter += 2;
			con = recData[counter++];		// ��ȡ�������ݸ�8λ
			con <<= 8;									
			con += recData[counter++];// ��ȡ�������ݵ�8λ
			cs += con;
			addr = recData[counter++];
			addr <<= 8;
			addr += recData[counter++];
			cs += addr;
			for(int i=0;i<length-4;i++)
			{
				userData[i] = recData[counter++];
				cs += userData[i];
			}
			if(cs+recData[counter]+1 == 0x00)	// ����ͼ��㣬cs+1=0��У��ͨ��
				return;
			counter += 2;
			userToDo(userData, length-4, con, addr);
			free(userData);
		}
	else
	{
		while(recData[counter] != NULL)counter++;
		uint8_t *userData = (uint8_t *)malloc(sizeof(uint8_t)*(counter));
		for(int i=0;i<counter;i++)
			userData[i] = recData[i];
		userToDo(userData, length-4, con, addr);
		free(userData);
	}
	
	
}


/*���ݷ��ͺ���
*�������: �û����ݣ��������ݳ���
*���ܷ�������͸��ģʽ�����200ms����һ��
*/ 
void sendData(uint8_t *userdata, uint16_t userLength)
{
	printf("��������\r\n");
	#if(!PASSTHROUGH)
		sendCommandCreate(userLength);
		while(sendReadyFlag == FALSE);
	#endif
	HAL_UART_Transmit(wifiCom, userdata, userLength, 0xFFFF);
	#if(!PASSTHROUGH)
		while(sendOkFlag == FALSE);
		sendReadyFlag = FALSE;
		sendOkFlag = FALSE;
	#endif
	printf("���ͳɹ�\r\n");
	
}


/*���ݷ��ͺ�����101��Լ��
*�������:�������ݣ���ַ���ݣ��û����ݣ��������ݳ���
*���ơ���ַ����Ϊ16λ���ݣ��û����ݴ���ֵΪ���飬��ֻ����ָ�����ȵ�����
*֡��ʽ��֡ͷ(68H) ����(2�ֽ�) ֡ͷ(68H) ������(2�ֽ�) ��ַ��(2�ֽ�) �û�����(length���ֽ�) У��λ(1�ֽ�) ֡β(16H)
*У��λCS = ~(������ + ��ַ�� + �û�����)
*���ݳ��� = �������û����ݽ�β�����ֽڳ���
*���ܷ�������͸��ģʽ�����300ms����һ��
*/ 

void sendData101(uint16_t con, uint16_t addr, uint8_t *userdata, uint16_t userLength)
{
	uint16_t len = 0;
	uint8_t cs = 0;
	if(userLength>1024)
		return;
	uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t)*(10+userLength));		// ��̬�������飬����Ϊ10+�û����ݳ���
	//data = (uint8_t *)malloc(sizeof(uint8_t)*16);		// ��̬�������飬����Ϊ10+�û����ݳ���
	//memset(data, 0, 512);
	data[len++] = 0x68;
	data[len++] = (uint8_t)((userLength+4)>>8);				// ���ݳ��ȸ�8λ
	data[len++] = (uint8_t)((userLength+4) & 0x00ff);  // ���ݳ��ȵ�8λ
	data[len++] = 0x68;
	data[len++] = (uint8_t)(con >> 8);
	data[len++] = (uint8_t)(con & 0x00ff);
	data[len++] = (uint8_t)(addr >> 8);
	data[len++] = (uint8_t)(addr & 0x00ff);
	for(int i=0; i<userLength; i++)
		data[len++] = userdata[i];
	for(int i=4; i<userLength+8; i++)
		cs += data[i];
	cs = ~cs;
	data[len++] = cs;
	data[len++] = 0x16;
	//data[len] = 0;
	printf("��������");
	#if(!PASSTHROUGH)
		sendCommandCreate(userLength+10);
		//HAL_Delay(500);
		while(sendReadyFlag == FALSE);
	#endif
//	for(int i=0;i<userLength+10;i++)
//		HAL_UART_Transmit(&wifiCom, &data[i], 1, 0xFFFF);
	HAL_UART_Transmit(wifiCom, data, userLength+10, 0xFFFF);
	//HAL_UART_Transmit(&wifiCom, "1234567890123456", userLength+10, 0xFFFF);
	#if(!PASSTHROUGH)
		while(sendOkFlag == FALSE);
		sendReadyFlag = FALSE;
		sendOkFlag = FALSE;
	#endif
	printf("���ͳɹ�");
	free(data);
}


/* ��̬���ɲ����͡�AT+CIPSEND=?���ַ���
* ��͸��ģʽ�¼��㷢�͵����ݳ��Ȳ����͸�wifiģ��
*/
int counter = 0;
void sendCommandCreate(uint16_t length)
{
	uint8_t j=11,atlen;
	uint8_t len[4] = {0,0,0,0};
	counter =0;
	while(length)
	{
		len[counter++] = (length % 10) + 0x30;
		length /= 10;
	}
	uint8_t *at_send = (uint8_t *)malloc(sizeof(uint8_t)*(counter + 11 + 2));  // counterΪ�����ֽڳ��� 11λ��AT+CIPSEND=�� 2Ϊ��\r\n��
	atlen = counter--;
	memset(at_send, 0, counter + 11 + 2);
	strcat(at_send, "AT+CIPSEND=");
	while(counter>=0)
		at_send[j++] = len[counter--];
	at_send[j++] = 0x0d;
	at_send[j++] = 0x0a;
	at_send[j] = 0;
	HAL_UART_Transmit(wifiCom, at_send, atlen + 11 + 2, 0xFFFF); // ����ָ��
	free(at_send);
	return;
}


/*�ַ���ƴ��
*�������:ƴ���ַ����ĸ�������κ���numΪ�ɱ�����ĸ�����
*�������δ���Ҫƴ�ӵ��ַ�����������
*/
uint8_t* strConnect(int num, ...)
{
    uint8_t len = 0;
    uint8_t temp[100] = "", *format;
    va_list arg;
    va_start(arg, num);
    uint8_t* ret = va_arg(arg, uint8_t*);
    for(int i=0; i<num; i++)
    {
        while (*ret)
        {
            temp[len++] = *ret;
            ret++;
        }
        ret = va_arg(arg, uint8_t*);
    }
    temp[len] = '\0';	// ���һ���ֽڲ�0��������������
    format = (uint8_t *)malloc(sizeof(uint8_t)*len);
    memset(format, 0, len);
    strcat(format,temp);
    //printf("%s", format);
    va_end(arg);
    return format;
}

/*����ATָ��
*�������:atָ��������ݱ�־����ʱʱ��
*/
uint8_t Send_AT_commend(char *at_commend, bool *re_flag, uint16_t time_out)
{
	for(uint8_t i=0;i<3;i++)
	{
		HAL_UART_Transmit(wifiCom, (uint8_t *)at_commend, strlen(at_commend), 0xFFFF);
		HAL_UART_Transmit(wifiCom, (uint8_t *)"\r\n", 2, 0xFFFF);				//�Զ����ͻس�����
		HAL_Delay(time_out);
		if(*re_flag)
			return 1;
	}
	return 0;
}

/*�����ַ���
*�����������Ҫƥ����ַ���
*/
uint8_t findStr(char *a)
{
	
	char *addr = strstr((char *)Espdatatype.AtBuffer,a);
	if(addr)
		return 1;
	return 0;
	
}



/* wifi��ʼ��logo+
*	��ʾlogo������ESP8266����ATָ�����
*/
uint8_t wifiStart(void)
{
	printf("\r\n");
	printf("*****************************************\r\n");
	printf("*                                       *\r\n");
	printf("*    JHX_ESP8266_DEMO    version 1.0    *\r\n");
	printf("*                                       *\r\n");
	printf("*****************************************\r\n");
	for(int i=0;i<=3;i++)
	{
		if(i>=3)
		{
			printf("ATָ�����ʧ�ܣ�������ߡ���������ģ��\r\n");
			return 0;
		}
		if(Send_AT_commend("AT", &atFlag, 100))
			break;
		if(Send_AT_commend("AT+RST", &rstFlag, 3000))
			rstFlag = FALSE; // ����ģ��
		
		HAL_Delay(500);
	}
	//printf("ATָ�����OK");
	return 1;
}
	

/*���ڻص�����
*	���ڿ����ж�ʱ����ص�������DMARecBuffer�к����ַ�����+IPD����Ϊ�û����յ�������
*	����ΪATָ���������
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char *addr = strstr((char *)Espdatatype.DMARecBuffer,"+IPD");
	uint16_t lenaddr = 0, data_len=0, copyaddr;
	if(huart->Instance == USART1)
	{
		if(addr)
		{
			while(Espdatatype.DMARecBuffer[++lenaddr] != ':');
			while(Espdatatype.DMARecBuffer[--lenaddr] != ',');
			lenaddr++;
			while(Espdatatype.DMARecBuffer[lenaddr] != ':')data_len = data_len*10 + (Espdatatype.DMARecBuffer[lenaddr++]-0x30);
			copyaddr=lenaddr+1;
			if(Espdatatype.UserRecLen>0)      		// �ж��û������������Ƿ���δ��������      
			{
					memcpy(&Espdatatype.UserBuffer[Espdatatype.UserRecLen], &Espdatatype.DMARecBuffer[copyaddr],data_len); // ת�浽�û����ݴ���������
					Espdatatype.UserRecLen +=  data_len;
			}
			else
			{
					memcpy(Espdatatype.UserBuffer, &Espdatatype.DMARecBuffer[copyaddr],data_len);                          // ת�浽�û����ݴ���������
					Espdatatype.UserRecLen =  data_len;
			}
			Espdatatype.UserRecFlag = 1;
		}
		else
		{
			if(Espdatatype.AtRecLen>0)           	// �ж�ATָ�������������Ƿ���δ��������  
			{
					memcpy(&Espdatatype.AtBuffer[Espdatatype.AtRecLen],Espdatatype.DMARecBuffer,Espdatatype.DMARecLen); 	// ת�浽ATָ�����ݴ���������
					Espdatatype.AtRecLen +=  Espdatatype.DMARecLen;
			}
			else
			{
					memcpy(Espdatatype.AtBuffer,Espdatatype.DMARecBuffer,Espdatatype.DMARecLen);                          // ת�浽ATָ�����ݴ���������
					Espdatatype.AtRecLen =  Espdatatype.DMARecLen;
			}
			Espdatatype.AtRecFlag = 1;
		}
	}
	memset(Espdatatype.DMARecBuffer, 0x00, DMA_REC_SIZE);
	
//    if(huart->Instance == USART1)
//    {
//        if(Usart2type.UsartRecLen>0)            
//        {
//            memcpy(&Usart2type.Usart2RecBuffer[Usart2type.UsartRecLen],Usart2type.Usart2DMARecBuffer,Usart2type.UsartDMARecLen); //ת�浽����������
//            Usart2type.UsartRecLen +=  Usart2type.UsartDMARecLen;
//        }
//        else
//        {
//            memcpy(Usart2type.Usart2RecBuffer,Usart2type.Usart2DMARecBuffer,Usart2type.UsartDMARecLen);                          //ת�浽����������
//            Usart2type.UsartRecLen =  Usart2type.UsartDMARecLen;
//        }
//        memset(Usart2type.Usart2DMARecBuffer, 0x00, sizeof(Usart2type.Usart2DMARecBuffer));                                     //�����DMA������
//        Usart2type.UsartRecFlag = 1;
//    }
}



// �ض���printf
int fputc(int ch, FILE *stream)
{
    /* �����жϴ����Ƿ������ */
    while((USART2->SR & 0X40) == 0);

    /* ���ڷ�����ɣ������ַ����� */
    USART2->DR = (uint8_t) ch;

    return ch;
}
