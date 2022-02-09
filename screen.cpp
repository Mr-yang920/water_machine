#include "screen.h"
#include <Arduino.h>
#include "water_class.h"

#define sendDataInterval 10

const char zero[] =
{
	0x11,0x21,0x31,0x41,0x51,0x61
};

const char one[] =
{
	0x21,0x31
};

const char two[] =
{
	0x11,0x21,0x71,0x51,0x41
};

const char three[] =
{
	0x11,0x21,0x31,0x41,0x71
};

const char four[] =
{
	0x21,0x31,0x61,0x71
};

const char five[] =
{
	0x11,0x31,0x41,0x61,0x71
};

const char six[] =
{
	0x11,0x31,0x41,0x51,0x61,0x71
};

const char seven[] =
{
	0x11,0x21,0x31
};

const char eight[] =
{
	0x11,0x21,0x31,0x41,0x51,0x61,0x71
};

const char nine[] =
{
	0x11,0x21,0x31,0x61,0x71
};

const char waterAnimation[] =
{
	0x82,0x92,0xA6,0x83,0x93,0x94,0x95,0x84,0x85
};

void sendData(char* data , int dataLen)
{
	Serial2.write(data , dataLen);
	delay(sendDataInterval);
}

void displayWhichScreen(int ScreenNum , int displayNum , bool display)
{
	if ( !( 0 <= ScreenNum && ScreenNum <= 5 ) )
	{
		Serial.println("显示屏序号输入有误，现在终止");
		return;
	}
	char data[7] = { 0x11,0x21,0x31,0x41,0x51,0x61,0x71 };
	int dataLen = 0;
	//先关闭这一个数码管的所有显示
	for ( size_t i = 0; i < 8; i++ )
	{
		char Data[3] = { 0x07 };
		Data[1] = data[i] + ScreenNum;
		Data[2] = 0x0;
		sendData(Data);
		//delay(sendDataInterval);
	}

	switch ( displayNum )
	{
		case 1:
			dataLen = sizeof(one);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = one[i];
			}
			break;
		case 2:
			dataLen = sizeof(two);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = two[i];
			}
			break;
		case 3:
			dataLen = sizeof(three);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = three[i];
			}
			break;
		case 4:
			dataLen = sizeof(four);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = four[i];
			}
			break;
		case 5:
			dataLen = sizeof(five);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = five[i];
			}
			break;
		case 6:
			dataLen = sizeof(six);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = six[i];
			}
			break;
		case 7:
			dataLen = sizeof(seven);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = seven[i];
			}
			break;
		case 8:
			dataLen = sizeof(eight);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = eight[i];
			}
			break;
		case 9:
			dataLen = sizeof(nine);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = nine[i];
			}
			break;
		case 0:
			dataLen = sizeof(zero);
			for ( size_t i = 0; i < dataLen; i++ )
			{
				data[i] = zero[i];
			}
			break;
		default:
			Serial.println("序号输入有误");
			break;
	}

	for ( size_t i = 0; i < dataLen; i++ )
	{
		if ( display )
		{
			char Data[3] = { 0x07 };
			Data[1] = data[i] + ScreenNum;
			Data[2] = 0x1;
			sendData(Data);
			//delay(sendDataInterval);
		} else
		{
			char Data[3] = { 0x07 };
			Data[1] = data[i] + ScreenNum;
			Data[2] = 0x0;
			sendData(Data);
			//delay(sendDataInterval);
		}
	}
}

void displayOriginalWater(bool display , bool displayType)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		if ( displayType )
		{
			//都显示
			Data[1] = 0x81;
			Data[2] = 0x1;
			sendData(Data , 3);
			//delay(sendDataInterval);
			Data[1] = 0x91;
			sendData(Data , 3);
			//delay(sendDataInterval);
		} else
		{
			//只显示左边
			Data[1] = 0x81;
			Data[2] = 0x1;
			sendData(Data , 3);
			//delay(sendDataInterval);
		}
	} else
	{
		Data[1] = 0x81;
		Data[2] = 0x0;
		sendData(Data , 3);
		//delay(sendDataInterval);
		Data[1] = 0x91;
		sendData(Data , 3);
		//delay(sendDataInterval);
	}
}

void displayCleanWater(bool display , bool displayType)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		if ( displayType )
		{
			//都显示
			Data[1] = 0x86;
			Data[2] = 0x1;
			sendData(Data , 3);
			//delay(sendDataInterval);
			Data[1] = 0x96;
			sendData(Data , 3);
			//delay(sendDataInterval);
		} else
		{
			//只显示左边
			Data[1] = 0x86;
			Data[2] = 0x1;
			sendData(Data , 3);
			//delay(sendDataInterval);
		}
	} else
	{
		Data[1] = 0x86;
		Data[2] = 0x0;
		sendData(Data , 3);
		//delay(sendDataInterval);
		Data[1] = 0x96;
		sendData(Data , 3);
		//delay(sendDataInterval);
	}
}

void displayOriginalWaterNum(String WaterNum , bool display , bool displayZero)
{
	if ( display )
	{
		int dataLen = WaterNum.length();
		if ( dataLen <= 3 )
		{
			if ( dataLen == 3 )
			{
				for ( size_t i = 0; i < 3; i++ )
				{
					displayWhichScreen(i , WaterNum[i] - 48);
					//delay(sendDataInterval);
				}
			} else
			{
				if ( displayZero )
				{
					//低位显示0
					for ( size_t i = 0; i < 3 - dataLen; i++ )
					{
						displayWhichScreen(i , 0);
					}
					for ( size_t i = 3 - dataLen; i < 3; i++ )
					{
						displayWhichScreen(i , WaterNum[i - ( 3 - dataLen )] - 48);
					}
				} else
				{
					for ( size_t i = 3 - dataLen; i < 3; i++ )
					{
						displayWhichScreen(i , WaterNum[i - ( 3 - dataLen )] - 48);
					}
				}
			}
		} else
		{
			Serial.println("原水数据输入非法！，现在终止。");
		}

	} else
	{
		for ( size_t i = 0; i < 3; i++ )
		{
			displayWhichScreen(i , 8 , false);
		}
	}
}

void displayCleanWaterNum(String WaterNum , bool display , bool displayZero)
{
	if ( display )
	{
		int dataLen = WaterNum.length();
		if ( dataLen <= 3 )
		{
			if ( dataLen == 3 )
			{
				for ( size_t i = 0; i < 3; i++ )
				{
					displayWhichScreen(i + 3 , WaterNum[i] - 48);
					//delay(sendDataInterval);
				}
			} else
			{
				if ( displayZero )
				{
					//低位显示0
					for ( size_t i = 0; i < 3 - dataLen; i++ )
					{
						displayWhichScreen(i + 3 , 0);
					}
					for ( size_t i = 3 - dataLen; i < 3; i++ )
					{
						displayWhichScreen(i + 3 , WaterNum[i - ( 3 - dataLen )] - 48);
					}
				} else
				{
					for ( size_t i = 3 - dataLen; i < 3; i++ )
					{
						displayWhichScreen(i + 3 , WaterNum[i - ( 3 - dataLen )] - 48);
					}
				}
			}
		} else
		{
			Serial.println("原水数据输入非法！，现在终止。");
		}

	} else
	{
		for ( size_t i = 0; i < 3; i++ )
		{
			displayWhichScreen(i + 3 , 8 , false);
		}
	}
}

void displayWaterDrop(int FPS)
{
	//waterAnimation
	char Data[3] = { 0x07 };
	Data[2] = 0x1;
	switch ( FPS )
	{
		case 6:
			Data[1] = waterAnimation[8];
			sendData(Data);
			Data[1] = waterAnimation[7];
			sendData(Data);
		case 5:
			Data[1] = waterAnimation[6];
			sendData(Data);
			Data[1] = waterAnimation[5];
			sendData(Data);
		case 4:
			Data[1] = waterAnimation[4];
			sendData(Data);
			Data[1] = waterAnimation[3];
			sendData(Data);
		case 3:
			Data[1] = waterAnimation[2];
			sendData(Data);
		case 2:
			Data[1] = waterAnimation[1];
			sendData(Data);
		case 1:
			Data[1] = waterAnimation[0];
			sendData(Data);
			break;
		case 0:
			Data[2] = 0x0;
			for ( size_t i = 0; i < sizeof(waterAnimation); i++ )
			{
				Data[1] = waterAnimation[i];
				sendData(Data);
			}
			break;
		default:
			Serial.println("序号输入有误");
			break;
	}
}

void displayWashing(bool display , bool displayType)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		if ( displayType )
		{
			//都显示
			Data[1] = 0xA1;
			Data[2] = 0x1;
			sendData(Data);
			Data[1] = 0xB1;
			sendData(Data);
		} else
		{
			//只显示下边
			Data[1] = 0xB1;
			Data[2] = 0x1;
			sendData(Data);
		}
	} else
	{
		Data[1] = 0xA1;
		Data[2] = 0x0;
		sendData(Data);
		Data[1] = 0xB1;
		sendData(Data);
	}
}

void displayWaterFull(bool display , bool displayType)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		if ( displayType )
		{
			//都显示
			Data[1] = 0xA2;
			Data[2] = 0x1;
			sendData(Data);
			Data[1] = 0xB2;
			sendData(Data);
		} else
		{
			//只显示下边
			Data[1] = 0xB2;
			Data[2] = 0x1;
			sendData(Data);
		}
	} else
	{
		Data[1] = 0xA2;
		Data[2] = 0x0;
		sendData(Data);
		Data[1] = 0xB2;
		sendData(Data);
	}
}

void displayWaterLittle(bool display , bool displayType)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		if ( displayType )
		{
			//都显示
			Data[1] = 0xA3;
			Data[2] = 0x1;
			sendData(Data);
			Data[1] = 0xB3;
			sendData(Data);
		} else
		{
			//只显示下边
			Data[1] = 0xB3;
			Data[2] = 0x1;
			sendData(Data);
		}
	} else
	{
		Data[1] = 0xA3;
		Data[2] = 0x0;
		sendData(Data);
		Data[1] = 0xB3;
		sendData(Data);
	}
}

void displayWaterFix(bool display , bool displayType)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		if ( displayType )
		{
			//都显示
			Data[1] = 0xA4;
			Data[2] = 0x1;
			sendData(Data);
			Data[1] = 0xB4;
			sendData(Data);
		} else
		{
			//只显示下边
			Data[1] = 0xB4;
			Data[2] = 0x1;
			sendData(Data);
		}
	} else
	{
		Data[1] = 0xA4;
		Data[2] = 0x0;
		sendData(Data);
		Data[1] = 0xB4;
		sendData(Data);
	}
}

void displaySignalIco(bool display)
{
	char Data[3] = { 0x07 };
	if ( display )
	{
		Data[1] = 0xB6;
		Data[2] = 0x1;
		sendData(Data);

	} else
	{
		Data[1] = 0xB6;
		Data[2] = 0x0;
		sendData(Data);
	}
}

void displayFacilityId(bool display)
{
	String data = getEquipmentID();
	for ( size_t i = 0; i < 6; i++ )
	{
		if ( display )
		{
			displayWhichScreen(i , data[i] - 48);
		} else
		{
			displayWhichScreen(i , 8 , false);
		}
	}
}