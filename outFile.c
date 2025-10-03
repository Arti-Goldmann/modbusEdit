void MBhandler_R(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 1:
			reg->data = IQtoInt16(var,100,base,0);
			break;
		case 3:
			reg->data = IQtoInt16(var,100,base,0);
			break;
		case 4:
			reg->data = IQtoInt16(var,100,base,0);
			break;
		case 6:
			reg->data = IQtoInt16(var,100,base,0);
			break;
		default:
			break;
	}
}

void MBhandler_W(TModbusSlaveDictObj* reg)
{
	switch (reg->mbIndex)
	{
		case 2:
			reg->data = Int16toIQ(var,100,base,0);
			break;
		case 3:
			reg->data = Int16toIQ(var,100,base,0);
			break;
		case 5:
			reg->data = Int16toIQ(var,100,base,0);
			break;
		case 6:
			reg->data = Int16toIQ(var,100,base,0);
			break;
		default:
			break;
	}
}

// R/W-переменные.
TModbusSlaveDictObj mbodHR[] =
    {
        0, 0xFFFF   // конец
    };

// R-переменные (наблюдаемые).
TModbusSlaveDictObj mbodIR[] =
    {
        0, 0xFFFF   // конец
    };

TModbusSlaveDictObj mbodC[] =
    {
        0, 0xFFFF   // конец
    };

TModbusSlaveDictObj mbodDI[] =
    {
        0, 0xFFFF   // конец
    };

