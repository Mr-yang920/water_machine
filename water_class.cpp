#include "water_class.h"

#define YELLOW_LED 5//水满
#define RED_LED 19//缺水
#define GREEN_LED 18//正常制水

void updateBin(String otaAddress);
bool connWifi(String data)
{
	StaticJsonDocument<96> doc;

	DeserializationError error = deserializeJson(doc , data);

	if ( error )
	{
		Serial.print("deserializeJson() failed: ");
		Serial.println(error.c_str());
		disposeErrData(JSON_PROCESSOR_ERR);
		return false;
	}

	const char* WIFISSID = doc["WIFISSID"]; // "111"
	const char* WIFIPWD = doc["WIFIPWD"]; // "111"

	Serial.println(WIFISSID);
	Serial.println(WIFIPWD);
	WiFi.begin(WIFISSID , WIFIPWD);
	Serial.print("wifi连接中，请稍等...");
	int conn_try_num = 0;//尝试连接wifi的时间
	while ( WiFi.status() != WL_CONNECTED )
	{ // 尝试进行wifi连接。 
		delay(1000);
		conn_try_num++;
		Serial.print(".");
		if ( conn_try_num > 15 )
		{
			disposeErrData(WIFI_CONN_TIMEOUT);
			Serial.println("wifi连接超时，请检查信息是否正确，现在最终");
			return false;
		}
	}
	Serial.print("连接成功，局域网ip地址:\t");            // 以及
	Serial.println(WiFi.localIP());           // NodeMCU的IP地址
	return true;
}

void configWifi()
{
	WiFiServer server(9080);
	WiFiClient serverClients[MAX_SRV_CLIENTS];

	WiFi.softAP("niuniulife");
	//启动UART传输和服务器
	server.begin();
	server.setNoDelay(true);
	while ( true )
	{
		uint8_t i;
		////检测服务器端是否有活动的客户端连接
		if ( server.hasClient() )
		{
			for ( i = 0; i < MAX_SRV_CLIENTS; i++ )
			{
				//查找空闲或者断开连接的客户端，并置为可用
				if ( !serverClients[i] || !serverClients[i].connected() )
				{
					if ( serverClients[i] ) serverClients[i].stop();
					serverClients[i] = server.available();
					Serial.print("New client: "); Serial.println(i);
					continue;
				}
			}
			//若没有可用客户端，则停止连接
			WiFiClient serverClient = server.available();
			serverClient.stop();
		}
		////检查客户端的数据
		for ( i = 0; i < MAX_SRV_CLIENTS; i++ )
		{
			if ( serverClients[i] && serverClients[i].connected() )
			{
				if ( serverClients[i].available() )
				{
					//从Telnet客户端获取数据，并推送到URAT端口
					String data = "";
					while ( serverClients[i].available() )
					{
						data += (char) serverClients[i].read();
						delay(10);
					}
					saveWifiData(data);
					delay(1000);
					serverClients[i].print(data);
					Serial.println("stop");
					return;
				}
			}
		}
	}
}

void saveWifiData(String wifiData)
{
	//"ssid","pwd"
	String wifi_t = wifiData.substring(wifiData.indexOf("="));
	String ssid = "" , pwd = "";
	int index[4] = {};//存储4个“引号”的位置
	int j = 0;//当前为第几个引号
	for ( int i = 0; i < wifi_t.length(); i++ )
	{
		if ( wifi_t[i] == '\"' )
		{
			index[j] = i;
			j++;
		}
	}
	ssid = wifi_t.substring(index[0] + 1 , index[1]);
	pwd = wifi_t.substring(index[2] + 1 , index[3]);
	//验证信息是否合法
	if ( ssid == "" || pwd.length() < 8 )
	{
		Serial.println("信息非法，现在最终");
		disposeErrData(ILLEGAL_WIFI_INFORMATION);
		return;
	}
	Serial.println(ssid);
	Serial.println(pwd);
	WiFi.begin(ssid.c_str() , pwd.c_str());
	Serial.print("wifi连接中，请稍等...");
	int conn_try_num = 0;//尝试连接wifi的时间
	while ( WiFi.status() != WL_CONNECTED )
	{ // 尝试进行wifi连接。
		delay(1000);
		conn_try_num++;
		Serial.print(".");
		if ( conn_try_num > 15 )
		{
			disposeErrData(WIFI_CONN_TIMEOUT);
			Serial.println("wifi连接超时，请检查信息是否正确，现在最终");
			return;
		}
	}
	String wifiData_;
	StaticJsonDocument<64> doc;

	doc["WIFISSID"] = WiFi.SSID();
	doc["WIFIPWD"] = WiFi.psk();

	serializeJson(doc , wifiData_);
	Serial.println(wifiData_);
	File dataFile = SPIFFS.open("/wifiData" , "w");// 建立File对象用于向SPIFFS中的file对象写入信息
	dataFile.print(wifiData_);
	dataFile.close();
	Serial.println("写入完成,现在复位设备");
	ESP.restart();
}

void disposeErrData(errType err)
{
	Serial.print("收到错误数据errType：");
	Serial.println(err);
}

String decToHex(int Dec)
{
	int a[10];
	int i = 0;
	int m = 0;
	int yushu;
	char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
	int num = Dec;
	while ( num > 0 )
	{
		yushu = num % 16;
		a[i++] = yushu;
		num = num / 16;
	}
	String str = "";
	for ( i = i - 1; i >= 0; i-- )//倒序输出 
	{
		m = a[i];
		str += hex[m];
	}
	if ( str.length()==0 )
	{//防止返回空字符串
		str = "0";
	}
	if ( str.length()%2 != 0 )
	{//返回长度为单数就在其前补0
		String d_t = "0";
		d_t += str;
		return d_t;
	}
	return str;
}

int hexToDec(String Hex)
{
	return strtol(Hex.c_str() , NULL , 16);
}

//获取公历年初至某整月的天数
int year_sumday(int year , int month)
{
	int sum = 0;
	int rui[12] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
	int ping[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	int ruiflag = 0;
	if ( ( year % 4 == 0 && year % 100 != 0 ) || year % 400 == 0 ) ruiflag = 1;
	for ( int index = 0; index < month - 1; index++ )
	{
		if ( ruiflag == 1 ) sum += rui[index]; else sum += ping[index];
	}
	return sum;
}

//获取公历年的天数
int year_alldays(int year)
{
	if ( ( year % 4 == 0 && year % 100 != 0 ) || year % 400 == 0 ) return 366; else return 365;
}

//获取从公历1800年1月25日至当前日期的总天数
int get_g_alldays(int year , int month , int day)
{
	int i = 1800 , days = -24;
	while ( i < year )
	{
		days += year_alldays(i);
		i++;
	}
	int days2 = year_sumday(year , month);
	return days + days2 + day;
}

struct calculateDay differenceDay(String day_1 , String day_2)//计算两个日期的插值(yymmdd)
{
	calculateDay day_;

	if ( day_1.length() != 6 || day_2.length() != 6 )
	{
		return day_;
	}
	int yy_1 , yy_2 , mm_1 , mm_2 , dd_1 , dd_2;
	yy_1 = day_1.substring(0 , 2).toInt();
	mm_1 = day_1.substring(2 , 4).toInt();
	dd_1 = day_1.substring(4).toInt();

	yy_2 = day_2.substring(0 , 2).toInt();
	mm_2 = day_2.substring(2 , 4).toInt();
	dd_2 = day_2.substring(4).toInt();

	int Day_1 = get_g_alldays(yy_1 + 2000 , mm_1 , dd_1);
	int Day_2 = get_g_alldays(yy_2 + 2000 , mm_2 , dd_2);
	int diff = Day_1 - Day_2;
	if ( diff >= 0 )
	{
		day_.state = true;
		day_.day = diff;
	} else
	{
		day_.state = false;
		day_.day = abs(diff);
	}
	return day_;
}

String getMac()
{
	byte mac[6];// 存放 MAC 位址的阵列
	WiFi.macAddress(mac);
	String mac_s = "00";
	for ( size_t i = 0; i < 6; i++ )
	{
		mac_s += hexToDec((String) mac[i]);
	}
	if ( mac_s.length() < 20 && mac_s.length() >= 16 )
	{
		for ( size_t i = 0; i <= ( 20 - mac_s.length() ); i++ )
		{
			mac_s += "0";
		}
	}
	return mac_s;
}

uint16_t CRC16_Modbus(uint8_t* buf , uint16_t len)
{
	uint16_t crc = 0xFFFF;
	uint16_t i , j;
	for ( j = 0; j < len; j++ )
	{
		crc = crc ^ *buf++;
		for ( i = 0; i < 8; i++ )
		{
			if ( ( crc & 0x0001 ) > 0 )
			{
				crc = crc >> 1;
				crc = crc ^ 0xa001;
			} else
			{
				crc = crc >> 1;
			}
		}
	}
	return ( crc );
}

uint16_t getCRC16(String data)
{
	uint8_t buff[64] = {};
	int j = 0;
	for ( size_t i = 0; i < data.length(); i += 2 )
	{
		String d_t = "";
		d_t += data[i];
		d_t += data[i + 1];
		buff[j] = hexToDec(d_t);
		j++;
	}
	return CRC16_Modbus(buff , j);
}

String sendHeartbeat(heartbeatConfig mState)
{
	Serial.print("设备状态");
	Serial.println(mState.mState);
	Serial.print("屏幕状态");
	Serial.println(mState.screenState ? "关闭" : "打开");
	Serial.print("工作模式");
	Serial.println(mState.workMode ? "时长模式" : "流量模式");

	if ( mState.mState )
	{
		//时长模式
		Serial.print("剩余流量");
		Serial.println(mState.Surplus_Flow);
	} else
	{
		//流量模式
		Serial.print("剩余天数");
		Serial.println(mState.Surplus_Days);
	}
	Serial.println("已用流量0");
	Serial.println("已用天数0");
	Serial.print("净水TDS");
	Serial.println(mState.clearwaterTDS);
	Serial.print("原水TDS");
	Serial.println(mState.tapWaterTDS);

	Serial.println("第1级滤芯剩余值0");
	Serial.println("第2级滤芯剩余值0");
	Serial.println("第3级滤芯剩余值0");
	Serial.println("第4级滤芯剩余值0");
	Serial.println("第5级滤芯剩余值0");

	Serial.print("第1级滤芯最大值");
	Serial.println(mState.LX_MAX[0]);
	Serial.print("第2级滤芯最大值");
	Serial.println(mState.LX_MAX[1]);
	Serial.print("第3级滤芯最大值");
	Serial.println(mState.LX_MAX[2]);
	Serial.print("第4级滤芯最大值");
	Serial.println(mState.LX_MAX[3]);
	Serial.print("第5级滤芯最大值");
	Serial.println(mState.LX_MAX[4]);

	Serial.print("信号强度值");
	mState.RSSI = WiFi.RSSI();
	
	if ( mState.RSSI > 150 )
	{
		mState.RSSI -= 130;
	}
	//想要映射
	/*
	0 -113 dBm or less 
	1 -111 dBm 
	2...30 -109... -53 dBm 
	31 -51 dBm or greater 
	99 not known or not detectable

	*/
	if ( mState.RSSI>=113 )
	{
		mState.RSSI = 0;
	} else 
	{
		mState.RSSI = ( map(mState.RSSI , 112 , 0 , 1 , 31) );
	}
	Serial.println(mState.RSSI);

	Serial.println("LAC值0");
	Serial.println("LAC值0");
	String sendData = getMac();
	sendData += "010028";
	sendData += decToHex(mState.mState);//设备状态
	sendData += decToHex(mState.screenState);//设备状态
	sendData += decToHex(mState.workMode);//工作模式
	if ( mState.workMode )
	{
		//时长模式
		sendData += "0000";
		String d_t = decToHex(mState.Surplus_Days);
		if ( d_t.length()<4 )
		{//检查看看是否够2字节
			String d_t1 = "";
			for ( size_t i = 0; i < 4-d_t.length(); i++ )
			{
				d_t1 += "0";
			}
			d_t1 += d_t;
			sendData += d_t1;
		} else
		{
			sendData += d_t;
		}
	} else
	{
		//流量模式
		String d_t = decToHex(mState.Surplus_Flow);
		if ( d_t.length() < 4 )
		{//检查看看是否够2字节
			String d_t1 = "";
			for ( size_t i = 0; i < 4 - d_t.length(); i++ )
			{
				d_t1 += "0";
			}
			d_t1 += d_t;
			sendData += d_t1;
		} else
		{
			sendData += d_t;
		}
		sendData += "0000";
	}
	sendData += "00000000";//已用流量,已用天数
	String d_t = decToHex(mState.clearwaterTDS);
	if ( d_t.length() < 4 )
	{//检查看看是否够2字节
		String d_t1 = "";
		for ( size_t i = 0; i < 4 - d_t.length(); i++ )
		{
			d_t1 += "0";
		}
		d_t1 += d_t;
		sendData += d_t1;
	} else
	{
		sendData += d_t;
	}

	d_t = decToHex(mState.tapWaterTDS);
	if ( d_t.length() < 4 )
	{//检查看看是否够2字节
		String d_t1 = "";
		for ( size_t i = 0; i < 4 - d_t.length(); i++ )
		{
			d_t1 += "0";
		}
		d_t1 += d_t;
		sendData += d_t1;
	} else
	{
		sendData += d_t;
	}

	for ( size_t i = 0; i < 5; i++ )//1-5级滤芯剩余值
	{
		sendData += "0000";
	}
	for ( size_t i = 0; i < 5; i++ )//1-5级滤芯z最大值
	{
		//检查看看是否够2字节
		//sendData += decToHex(mState.LX_MAX[i]);
		d_t = decToHex(mState.LX_MAX[i]);
		if ( d_t.length() < 4 )
		{//检查看看是否够2字节
			String d_t1 = "";
			for ( size_t i = 0; i < 4 - d_t.length(); i++ )
			{
				d_t1 += "0";
			}
			d_t1 += d_t;
			sendData += d_t1;
		} else
		{
			sendData += d_t;
		}
	}
	sendData += decToHex(mState.RSSI);//信号强度值
	sendData += "00000000";//LAC值,LAC值
	sendData += decToHex(getCRC16(sendData));
	Serial.println(sendData);
	return sendData;
}

void processServerDeliveryInformation(String order , heartbeatConfig* mState)
{
	String d = (String) order[0];
	if ( order[0] == '{' )
	{
		Serial.println("ota命令");
		isOta(order);
		return;
	}

	Serial.print("命令如下：");
	Serial.println(order);
	//防止传输错误
	if ( ( d.toInt() == 0 ) && ( !( order[0] == '0' ) ) )
	{
		Serial.println("此命令无效！现在终止");
		return;
	}
	//先验证传输是否出现错误
	String data = order.substring(0 , order.length() - 4);
	String CRC16 = order.substring(order.length() - 4);
	Serial.println(data);
	Serial.println(CRC16);
	if ( getCRC16(data) == hexToDec(CRC16) )
	{
		Serial.println("命令传输无误");
		//解析设备号
		String ID = data.substring(0 , 20);
		//解析命令类型
		String orderType = data.substring(20 , 26);
		Serial.println(ID);
		Serial.println(orderType);
		if ( ID == getMac() )
		{
			String orderData = data.substring(26);
			String outData;

			if ( orderType == "02000B" )
			{
				StaticJsonDocument<128> doc;
				bool RST = false;
				if ( SPIFFS.exists("/setMealData") )
				{
					RST = true;//重启设备，防止计费混乱
				}
				Serial.println("绑定套餐");
				if ( orderData.substring(0 , 2) == "00" )
				{
					Serial.println("流量模式");
					doc["workMode"] = 0;
					mState->workMode = 0;
				} else if ( orderData.substring(0 , 2) == "01" )
				{
					Serial.println("时间模式");
					doc["workMode"] = 1;
					mState->workMode = 1;
				}
				//解析滤芯最大值
				uint16_t LX_MAX_[5] = {};
				JsonArray LX_MAX = doc.createNestedArray("LX_MAX");
				for ( size_t i = 0; i < 5; i++ )
				{
					String d_t = orderData.substring(2 + i * 4 , 6 + i * 4);
					LX_MAX_[i] = hexToDec(d_t);
					Serial.println(LX_MAX_[i]);
					LX_MAX.add(LX_MAX_[i]);
					mState->LX_MAX[i] = LX_MAX_[i];
				}
				doc["balance"] = 0;
				serializeJson(doc , outData);
				Serial.println(outData);
				File dataFile = SPIFFS.open("/setMealData" , "w");// 建立File对象用于向SPIFFS中的file对象写入信息
				dataFile.print(outData);
				dataFile.close();
				mState->mState = ARREARAGE;//欠费
				if ( RST )
				{
					Serial.println("套餐修改完毕，现在重启设备");
					ESP.restart();//重启设备
				}
			} else if ( orderType == "070001" )
			{
				Serial.println("强制冲洗");
				if ( !SPIFFS.exists("/setMealData") )
				{
					Serial.println("请先绑定套餐，现在终止");
					return;
				}
				mState->mState = LXTOBERESET;
				
			} else if ( orderType == "080002" )
			{
				//Serial.println("充正值");
				Serial.print("充正值天数/流量");
				Serial.println(hexToDec(orderData));
				if ( !SPIFFS.exists("/setMealData") )
				{
					Serial.println("请先绑定套餐，现在终止");
					return;
				}
				StaticJsonDocument<128> doc;
				doc["workMode"] = mState->workMode;
				//解析滤芯最大值
				//uint16_t LX_MAX_[5] = {};
				JsonArray LX_MAX = doc.createNestedArray("LX_MAX");
				for ( size_t i = 0; i < 5; i++ )
				{
					LX_MAX[i] = mState->LX_MAX[i];
				}
				if ( mState->workMode )
				{
					//时长模式
					//读取剩余时长
					mState->Surplus_Days += hexToDec(orderData);
					if ( mState->Surplus_Days > 0 )
					{
						mState->mState = PROPERWORK;
					} else
					{
						mState->mState = ARREARAGE;
					}
				} else
				{
					//流量模式
					//读取剩余流量
					mState->Surplus_Flow += hexToDec(orderData);
					if ( mState->Surplus_Flow > 0 )
					{
						mState->mState = PROPERWORK;
					} else
					{
						mState->mState = ARREARAGE;
					}
				}
				doc["balance"] = mState->Surplus_Flow;
				serializeJson(doc , outData);
				Serial.println(outData);
				File dataFile = SPIFFS.open("/setMealData" , "w");// 建立File对象用于向SPIFFS中的file对象写入信息
				dataFile.print(outData);
				dataFile.close();

			} else if ( orderType == "090002" )
			{
				//Serial.println("负正值");
				Serial.print("充负值天数/流量");
				Serial.println(hexToDec(orderData));
				if ( !SPIFFS.exists("/setMealData") )
				{
					Serial.println("请先绑定套餐，现在终止");
					return;
				}
				StaticJsonDocument<128> doc;
				doc["workMode"] = mState->workMode;
				//解析滤芯最大值
				//uint16_t LX_MAX_[5] = {};
				JsonArray LX_MAX = doc.createNestedArray("LX_MAX");
				for ( size_t i = 0; i < 5; i++ )
				{
					LX_MAX[i] = mState->LX_MAX[i];
				}
				if ( mState->workMode )
				{
					//时长模式
					//读取剩余时长
					mState->Surplus_Days -= hexToDec(orderData);
					if ( mState->Surplus_Days > 0 )
					{
						mState->mState = PROPERWORK;
					} else
					{
						mState->mState = ARREARAGE;
					}
				} else
				{
					//流量模式
					//读取剩余流量
					mState->Surplus_Flow -= hexToDec(orderData);
					if ( mState->Surplus_Flow > 0 )
					{
						mState->mState = PROPERWORK;
					} else
					{
						mState->mState = ARREARAGE;
					}
				}
				if ( mState->Surplus_Flow<0 )//防止余额出现负值
				{
					Serial.println("冲的负值大于当前余额，余额计为0");
					mState->Surplus_Flow = 0;
				}
				doc["balance"] = mState->Surplus_Flow;
				serializeJson(doc , outData);
				Serial.println(outData);
				File dataFile = SPIFFS.open("/setMealData" , "w");// 建立File对象用于向SPIFFS中的file对象写入信息
				dataFile.print(outData);
				dataFile.close();
			} else if ( orderType == "100001" )
			{
				Serial.println("恢复出厂设置");
			} else if ( orderType == "110021" )
			{
				Serial.println("修改域名和端口号");
			} else if ( orderType == "130002" )
			{
				Serial.println("修改心跳周期");
			} else
			{
				Serial.println("非法命令，现在终止");
			}
		} else
		{
			Serial.println("此命令对此设备无效");
		}
	} else
	{
		Serial.print(getCRC16(data));
		Serial.println("-此命令无效！现在终止");
	}
}

String getOtaPwd(String otaAddress , String time)
{
	String data = getMac();
	data += otaAddress;
	data += time;
	data += OTA_PUBLIC_PWD;

	//Serial.println(data);
	
	
	char a[200];
	int dataLen = data.length();
	//Serial.println(dataLen);
	if ( dataLen>195 )
	{
		Serial.println("数据亮过大！现在最终");
		return "";
	}
	for ( size_t i = 0; i < dataLen; i++ )
	{
		a[i] = data[i];
	}
	unsigned char* hash = MD5::make_hash(a, dataLen);
	char* md5str = MD5::make_digest(hash , 16);
	String retData = md5str;
	free(hash);
	//Serial.println(md5str);
	free(md5str);
	return retData;
}
bool isOta(String JSONData)
{
	// Stream& input;

	StaticJsonDocument<256> doc;

	DeserializationError error = deserializeJson(doc , JSONData);

	if ( error )
	{
		Serial.print("deserializeJson() failed: ");
		Serial.println(error.c_str());
		return false;
	}

	String otaAddress = doc["otaAddress"]; // "www.baidu.com"
	String time = doc["time"]; // "1642941623"
	String verifyVal = doc["verifyVal"]; // "06789c678049c1e27d5005efb3bbd93a"
	String a = getOtaPwd(otaAddress , time);
	verifyVal.toLowerCase();
	if ( getOtaPwd(otaAddress , time)== verifyVal )
	{
		Serial.println("ota效验成功，开始升级");
		sendtoTCPServer("{\"type\":\"otaResult\",\"code\":1}");
		updateBin(otaAddress);
		return true;
	} else
	{
		Serial.println("ota效验错误");
		sendtoTCPServer("{\"type\":\"otaResult\",\"code\":0}");
		Serial.println(getOtaPwd(otaAddress , time));
		Serial.println(otaAddress);
		Serial.println(time);
		Serial.println(verifyVal);
		return false;
	}

}

void updateBin(String otaAddress)
{
	Serial.println("start update");
	WiFiClient UpdateClient;
	t_httpUpdate_return ret = httpUpdate.update(UpdateClient , otaAddress);
	switch ( ret )
	{
		case HTTP_UPDATE_FAILED:      //当升级失败
			Serial.println("[update] Update failed.");
			break;
		case HTTP_UPDATE_NO_UPDATES:  //当无升级
			Serial.println("[update] Update no Update.");
			break;
		case HTTP_UPDATE_OK:         //当升级成功
			Serial.println("[update] Update ok.");
			break;
	}
}