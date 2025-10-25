//The file was generated automatically.
#include "MBedit.h"

void MBhandlerHR_R(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 1:
			while(0) {};
			break;
		case 3:
			
			break;
		case 5:
			reg->data = IQtoInt16(varName5,0.2,drv_params.Inom,0);
			break;
		case 7:
			test code R
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
			while(1) {};
			break;
		case 3:
			
			break;
		case 5:
			varName5 = Int16toIQ(reg->data,0.2,drv_params.Inom,0);
			break;
		case 7:
			test code W
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
			while(0) {};
			break;
		case 4:
			reg->data = IQtoInt16(varName4,0.1,drv_params.Unom,6);
			break;
		case 6:
			reg->data = IQtoUInt16(varName6,0.3,drv_params.Inom,0);
			break;
		case 8:
			test code for only R
			break;
		default:
			break;
	}
}

// R/W-переменные.
TModbusSlaveDictObj*mbodHR[]=
{
	1, 0,   //USER CODE
	3, 0,   //USER CODE
	5, 0,   //varName5
	7, 0,   //USER CODE
	0, 0xFFFF   //end
};

TModbusSlaveDictObj*mbodIR[]=
{
	2, 0,   //USER CODE
	4, 0,   //varName4
	6, 0,   //varName6
	8, 0,   //USER CODE
	0, 0xFFFF   //end
};

TModbusSlaveDictObj mbodC[] =
{
    0, 0xFFFF   // конец
};

TModbusSlaveDictObj mbodDI[] =
{
    0, 0xFFFF   // конец
};

