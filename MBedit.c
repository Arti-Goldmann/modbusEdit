//The file was generated automatically.
#include "MBedit.h"

void MBhandlerHR_R(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 1:
			reg->data = IQtoInt16(varName1,0.1,drv_params.speed_nom,0);
			break;
		case 3:
			USER_CODE_FOR_READ;

			break;
		case 5:
			reg->data = varName5;
			break;
		case 7:
			USER_CODE_FOR_READ;
			break;
		case 8:
			reg->data = IQtoInt16(varName8,0.5,drv_params.test,6);
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
		if(!MBEDIT_IS_DRV_IN_STOP()){
			varName1 = Int16toIQ(reg->data,0.1,drv_params.speed_nom,0);
			MBEDIT_SAT_MIN(-20.5, &varName1, drv_params.speed_nom, 0);
			MBEDIT_SAT_MAX(10, &varName1, drv_params.speed_nom, 0);
		}
			break;
		case 3:
			USER_CODE_FOR_WRITE;
			break;
		case 5:
			varName5 = reg->data;
			if (varName5 < -10) varName5 = -10;
			if (varName5 > 10) varName5 = 10;
			break;
		case 7:
			USER_CODE_FOR_WRITE;
			break;
		case 8:
		if(!MBEDIT_IS_DRV_IN_STOP()){
			varName8 = Int16toIQ(reg->data,0.5,drv_params.test,6);
			MBEDIT_SAT_MIN(-10, &varName8, drv_params.test, 6);
			MBEDIT_SAT_MAX(10, &varName8, drv_params.test, 6);
		}
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
			USER_CODE_FOR_READ;
			break;
		case 4:
			reg->data = IQtoInt16(varName4,0.1,drv_params.Unom,6);
			break;
		case 6:
			reg->data = varName6;
			break;
		default:
			break;
	}
}

// R/W-variables.
TModbusSlaveDictObj*mbodHR[]=
{
	1, 0,   //varName1
	3, 0,   //USER CODE
	5, 0,   //varName5
	7, 0,   //USER CODE
	8, 0,   //varName8
	0, 0xFFFF   //end
};

TModbusSlaveDictObj*mbodIR[]=
{
	2, 0,   //USER CODE
	4, 0,   //varName4
	6, 0,   //varName6
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

