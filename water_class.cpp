#include "water_class.h"

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
		mac_s += hexToDec((String )mac[i]);
	}
	if ( mac_s.length()<20&& mac_s.length()>=16 )
	{
		for ( size_t i = 0; i <= (20- mac_s.length()); i++ )
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

bool sendHeartbeat()
{

} 

void processServerDeliveryInformation(String order)
{
	String d = (String) order[0];
	Serial.println(d.toInt());
	Serial.println(order[0]=='0');
	
	Serial.print("命令如下：");
	Serial.println(order);
	//防止传输错误
	if ( ( d.toInt() == 0 ) && ( !( order[0] == '0' ) ) )
	{
		Serial.println("此命令无效！现在终止");
		return;
	}
}