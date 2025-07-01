/**************************************************************
WFSensor�����������ļ�
***************************************************************/
#include "WF_i2c.h"
#include "wfsensor.h"
#include "g.h"
#include "x_sf.h"
#define		WFSensorIICDevice  0XDA			
#define		CMD_GROUP_CONVERT  0X0A			
u8 QT[5];
/********************************************************************
* Function Name  : WFSensor_ReadByte.
* Description    : IIC д������
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void WFSensor_WriteByte(u8 addr,u8 Data)
{
	SCL_OUTPUT();
	iic_start();
	iic_write_byte(WFSensorIICDevice); //д�豸��
	iic_wait_ack();
	iic_write_byte(addr); //дҪд��ļĴ�����ַ
	iic_wait_ack();
	iic_write_byte(Data); //��ȡ����
	iic_wait_ack();
	iic_stop();
	SDA_INPUT();
	SCL_INPUT();
}
/********************************************************************
* Function Name  : WFSensor_ReadByte.
* Description    : IIC д������
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void WFSensor_ReadContiune(u8 addr,u8 *p,u8 Length)
{
	u8 i = 0;
	SDA_OUTPUT();
	SCL_OUTPUT();
	iic_start();
	iic_write_byte(WFSensorIICDevice); //д�豸��
	iic_wait_ack();
	iic_write_byte(addr); //дҪд��ļĴ�����ַ
	iic_wait_ack();
	iic_start();
	iic_write_byte(WFSensorIICDevice|0X01); //��ȡ����
	iic_wait_ack();
	for(i=0;i<Length;i++)
	{
		p[i]= iic_read_byte();
		if(i < Length-1)
		{
			iic_ack();
		}
	}
	
	iic_noAck();
	iic_stop();
	SDA_INPUT();
	SCL_INPUT();
}

void WFSensor_indicateGroupConvert(void)
{
	WFSensor_WriteByte(0X30,CMD_GROUP_CONVERT); 
}

void WFSensor_getTPData(void)
{
	WFSensor_ReadContiune(0X06,QT,5);
}

float calculatePress(void)
{	
    float fDat;
	float Press_Data;  //unit = kpa
	
	s32 dat;
//	u32 TRP;
	
	dat = QT[0];
	dat <<= 8;
	dat |= QT[1];
	dat <<= 8;
	dat |= QT[2];
	if (dat > 8388608) 
	{
		fDat = (dat - 16777216) / 8388608.0f;
	} else {
		fDat = dat / 8388608.0f;
	}
	
	Press_Data = 50 * fDat + 5;

	return Press_Data;
}

float calculatetemp(void)//�¶Ȼ���
{
	float Temp_Data;   //unit = c ���϶�
	s32 dat;
	//�¶�
	dat = QT[3];
	dat = dat << 8;
	dat |= QT[4];
	if (dat > 32768) 
	{
		Temp_Data = (dat - 65536) / 256.0f;
	} 
	else 
	{
		Temp_Data = dat / 256.0f;
	}
	return Temp_Data;
}

//��ȡ��ѹ����������
void getwfData(void)
{
	float press=0;
	WFSensor_indicateGroupConvert(); //����ת������
	WFSensor_getTPData(); //��ȡ����
	press=calculatePress(); //��������
	airbagsetfile->airpressure=(uint16_t)(press*1000);//��ǰ��ѹֵ����ͨ�ű�����
	g_sysparam_st.airpump.pa_cur = (uint16_t)(press*1000);
	
}




