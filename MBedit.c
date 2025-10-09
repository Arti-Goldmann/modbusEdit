//Файл сгенерирован автоматически.
#include "MBedit.h"
void MBhandlerHR_R(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 1:
			reg->data = IQtoInt16(varName1,0.1,drv_params.speed_nom,0);
			break;
		case 3:
			reg->data = IQtoUInt16(varName3,0.3,drv_params.Unom,6);
			break;
		case 5:
			reg->data = IQtoInt16(varName5,0.2,drv_params.Inom,0);
			break;
		default:
			break;
	}
}

void MBhandlerHR_W(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 1:
			varName1 = Int16toIQ(reg->data,0.1,drv_params.speed_nom,0);
			break;
		case 3:
			varName3 = UInt16toIQ(reg->data,0.3,drv_params.Unom,6);
			break;
		case 5:
			varName5 = Int16toIQ(reg->data,0.2,drv_params.Inom,0);
			break;
		default:
			break;
	}
}

void MBhandlerIR_R(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 2:
			reg->data = IQtoInt16(varName2,0.2,drv_params.speed_nom,0);
			break;
		case 4:
			reg->data = IQtoInt16(varName4,0.1,drv_params.Unom,6);
			break;
		case 6:
			reg->data = IQtoUInt16(varName6,0.3,drv_params.Inom,0);
			break;
		default:
			break;
	}
}

// R/W-переменные.
TModbusSlaveDictObj*mbodHR[]=
{
	1, 0,   //varName1
	3, 0,   //varName3
	5, 0,   //varName5
	0, 0xFFFF   //конец
};

TModbusSlaveDictObj*mbodIR[]=
{
	2, 0,   //varName2
	4, 0,   //varName4
	6, 0,   //varName6
	0, 0xFFFF   //конец
};

TModbusSlaveDictObj mbodC[] =
{
    0, 0xFFFF   // конец
};

TModbusSlaveDictObj mbodDI[] =
{
    0, 0xFFFF   // конец
};

