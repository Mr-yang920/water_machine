#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <FS.h>
//#include <BluetoothSerial.h>
#include <WiFi.h>
#include "water_class.h"
#include <WiFiClient.h>


//服务器地址
#define TCP_SERVER_ADDR "192.168.31.176"
//服务器端口号
#define TCP_SERVER_PORT "1024"
//最大字节数
#define MAX_PACKETSIZE 512
//设置心跳值30s
#define KEEPALIVEATIME 30*1000
//定义可连接的客户端数目最大值
#define MAX_SRV_CLIENTS 2

//tcp客户端相关初始化，默认即可
WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;//客户端最后一次和服务器通讯的时间
unsigned long preHeartTick = 0;//心跳
unsigned long preTCPStartTick = 0;//连接
bool preTCPConnected = false;//和服务器的连接状态
heartbeatConfig mState;

void setup()
{
	
	/*pinMode(PIN , INPUT_PULLDOWN);*/
	Serial.begin(115200);
	/*Serial.println(getCRC16("0011823372515831040001002801000000640032003200320010006403E803E807D007D0138803E803E807D007D0138810AABBCCDD"));
	Serial.println(getCRC16("0011823372515831040002000B0003E803E807D007D01388"));
	Serial.println(getCRC16("0011823372515831040007000100"));*/
	//Serial.println(decToHex(false));
	//SerialBT.begin("water");
	sendEquipmentStatus(STANDBY);
	sendEquipmentStatus(DEACTIVATE);
	sendEquipmentStatus(ARREARAGE);
	sendEquipmentStatus(PROPERWORK);
	sendEquipmentStatus(PROPERWORK,999);
	//初始化闪存系统
	Serial.print("正在打开闪存系统...");
	while ( !SPIFFS.begin(true) )
	{
		Serial.print("...");
		delay(1000);
	}
	Serial.println("OK!"); 
	if ( SPIFFS.exists("/setMealData") )
	{
		Serial.println("存在配置文件");
		File dataFile = SPIFFS.open("/setMealData" , "r");
		long dataSize = dataFile.size();
		String fsData;
		for ( int i = 0; i < dataSize; i++ )
		{
			fsData += (char) dataFile.read();
		}
		dataFile.close();
		// Stream& input;

		StaticJsonDocument<192> doc;

		DeserializationError error = deserializeJson(doc , fsData);
		if ( error )
		{
			Serial.print("deserializeJson() failed: ");
			Serial.println(error.c_str());
			disposeErrData(JSON_PROCESSOR_ERR);
			return;
		}
		mState.workMode = doc["workMode"]; // 0
		if ( (bool)doc["workMode"] )
		{
			if ( (int)doc["balance"]>0 )
			{
				mState.mState = PROPERWORK;
			} else
			{
				mState.mState = ARREARAGE;
			}
		} else
		{
			if ( (int) doc["balance"] > 0 )
			{
				mState.mState = PROPERWORK;
			} else
			{
				mState.mState = ARREARAGE;
			}
		}
		JsonArray LX_MAX = doc["LX_MAX"];
		for ( size_t i = 0; i < 5; i++ )
		{
			mState.LX_MAX[i] = LX_MAX[i];
		}
		if ( (bool) doc["workMode"] )
		{
			//时长模式
			mState.Surplus_Days = doc["balance"]; // 1
		} else
		{
			//流量模式
			mState.Surplus_Flow = doc["balance"]; // 0
		}
	} else
	{
		Serial.println("不存在配置文件");
		mState.mState = DEACTIVATE;//待激活
	}
	if ( SPIFFS.exists("/wifiData") )
	{
		Serial.println("存在wifi信息，现在开始尝试连接wifi");
		File dataFile = SPIFFS.open("/wifiData" , "r");
		long dataSize = dataFile.size();
		String fsData;
		for ( int i = 0; i < dataSize; i++ )
		{
			fsData += (char) dataFile.read();
		}
		dataFile.close();
		if ( connWifi(fsData) )
		{
			//如果wifi连接成功，开始连接服务器
			Serial.println(getMac());
		}
	} else
	{
		disposeErrData(WIFI_INFO_NOT_FOUND);
		configWifi();
	}
}


/*
  *发送数据到TCP服务器
 */
void sendtoTCPServer(String p)
{

	if ( !TCPclient.connected() )
	{
		Serial.println("Client is not readly");
		return;
	}
	TCPclient.print(p);
	Serial.println("[Send to TCPServer]:String");
	Serial.println(p);
}


/*
  *初始化和服务器建立连接
*/
void startTCPClient()
{
	if ( TCPclient.connect(TCP_SERVER_ADDR , atoi(TCP_SERVER_PORT)) )
	{
		Serial.print("\nConnected to server:");
		Serial.printf("%s:%d\r\n" , TCP_SERVER_ADDR , atoi(TCP_SERVER_PORT));
		sendtoTCPServer(sendHeartbeat(mState));
		preTCPConnected = true;
		preHeartTick = millis();
		TCPclient.setNoDelay(true);
	} else
	{
		Serial.print("Failed connected to server:");
		Serial.println(TCP_SERVER_ADDR);
		TCPclient.stop();
		preTCPConnected = false;
	}
	preTCPStartTick = millis();
}


/*
  *检查数据，发送心跳
*/
void doTCPClientTick()
{
	//检查是否断开，断开后重连
	if ( WiFi.status() != WL_CONNECTED ) return;

	if ( !TCPclient.connected() )
	{//断开重连

		if ( preTCPConnected == true )
		{

			preTCPConnected = false;
			preTCPStartTick = millis();
			Serial.println();
			Serial.println("TCP Client disconnected.");
			TCPclient.stop();
		} else if ( millis() - preTCPStartTick > 1 * 1000 )//重新连接
		{
			startTCPClient();
		}

	} else
	{
		if ( TCPclient.available() )
		{//收数据
			while ( TCPclient.available() )
			{
				TcpClient_Buff += (char) TCPclient.read();
			}
			if ( TcpClient_Buff.length() > 0 )
			{
				processServerDeliveryInformation(TcpClient_Buff, &mState);

			}
			TcpClient_Buff = "";
		}
		if ( millis() - preHeartTick >= KEEPALIVEATIME )
		{//保持心跳
			preHeartTick = millis();
			Serial.println("--Keep alive:");
			sendtoTCPServer(sendHeartbeat(mState));
		}
	}
	//mState.Surplus_Flow=666;
}
void sendEquipmentStatus(machineState state,int time)
{
	String sendData = getMac();
	sendData += "0C0003";
	sendData += decToHex(state);
	if ( state== PROPERWORK )
	{
		if ( time==0 )
		{
			Serial.println("请传值！现在终止");
			return;
		}
		//正常制水
		String d_t = decToHex(time);
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
	} else
	{
		sendData += "0000";
	}
	sendData += decToHex(getCRC16(sendData));
	Serial.println(sendData);
	sendtoTCPServer(sendData);
}

void loop()
{
	doTCPClientTick();
}
