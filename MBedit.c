void MBhandlerHR_R(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 1:
			reg->data = IQtoInt16(varName,0.1,base,0);
			break;
		case 3:
			reg->data = IQtoUInt16(varName,0.3,base,0);
			break;
		case 5:
			reg->data = IQtoInt16(varName,0.2,base,0);
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
			varName = Int16toIQ(reg->data,0.1,base,0);
			break;
		case 3:
			varName = UInt16toIQ(reg->data,0.3,base,0);
			break;
		case 5:
			varName = Int16toIQ(reg->data,0.2,base,0);
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
			reg->data = IQtoInt16(varName,0.2,base,0);
			break;
		case 4:
			reg->data = IQtoInt16(varName,0.1,base,0);
			break;
		case 6:
			reg->data = IQtoUInt16(varName,0.3,base,0);
			break;
		default:
			break;
	}
}

// R/W-переменные.
TModbusSlaveDictObj*mbodHR[]=
{
	1, 0,   //varName
	3, 0,   //varName
	5, 0,   //varName
	0, 0xFFFF   //конец
};

TModbusSlaveDictObj*mbodIR[]=
{
	2, 0,   //varName
	4, 0,   //varName
	6, 0,   //varName
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

