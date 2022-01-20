﻿#include <SPIFFS.h>
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

#define YELLOW_LED 5//水满
#define RED_LED 19//缺水
#define GREEN_LED 18//正常制水
#define HIGH_PRESSSURE 33
#define LOW_PRESSSURE 25

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

	Serial.begin(115200);
	
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
		if ( (bool) doc["workMode"] )
		{
			if ( (int) doc["balance"] > 0 )
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
	while ( !TCPclient.connected() )
	{
		Serial.println("尝试连接到服务器");
		startTCPClient();
		delay(1000);
	}
	Serial.println("服务器连接成功，开始工作");
	//创建监控线程
	xTaskCreatePinnedToCore(
		monitoring                                     //创建任务
		, "monitoring"                                 //创建任务的名称
		, 1024 * 5                                   //分配的运行空间大小
		, NULL                                       //任务句柄
		, 1                                           //任务优先级
		, NULL                                        //回调函数句柄
		, 1);
}


void monitoring(void* pvParameters)
{
	pinMode(RED_LED , OUTPUT);
	pinMode(GREEN_LED , OUTPUT);
	pinMode(YELLOW_LED , OUTPUT);
	pinMode(HIGH_PRESSSURE , INPUT_PULLUP);
	pinMode(LOW_PRESSSURE , INPUT_PULLUP);
	uint8_t workNum = 0;//制水次数2小时
	uint8_t workNum_ = 0;//制水次数3分钟
	long oneWorktime = 0;//第一次制水时间
	long oneWorktime_ = 0;//第一次制水时间
	uint8_t lowWaterTime = 0;//缺水时间
	bool isLowWater = false;
	long disConnectedServerTime = 0;//和服务器断开的时间
	while ( true )
	{
		if ( !TCPclient.connected() )
		{
			disConnectedServerTime++;
			Serial.println("尝试重新连接到服务器");
			if ( disConnectedServerTime>60*60*24*3 )
			{
				//报检修
				mState.mState = WIFI_LOST;
				sendEquipmentStatus(WIFI_LOST);
				Serial.println("频繁启动制水-3分钟");
				while ( true )
				{
					digitalWrite(YELLOW_LED , HIGH);
					digitalWrite(RED_LED , HIGH);
					digitalWrite(GREEN_LED , HIGH);
					vTaskDelay(1000);
					digitalWrite(YELLOW_LED , LOW);
					digitalWrite(RED_LED , LOW);
					digitalWrite(GREEN_LED , LOW);
					vTaskDelay(1000);
				}
			}
		} else
		{
			disConnectedServerTime = 0;
		}
		vTaskDelay(1000);
		//更新计费状态
		if ( mState.workMode )
		{
			//时长模式
			//读取剩余时长
			if ( mState.Surplus_Days > 0 )
			{
				mState.mState = PROPERWORK;
			} else
			{
				mState.mState = ARREARAGE;
			}
		} else
		{
			//流量模式
			//读取剩余流量
			if ( mState.Surplus_Flow > 0 )
			{
				mState.mState = PROPERWORK;
			} else
			{
				mState.mState = ARREARAGE;
			}
		}
		if ( mState.mState == DEACTIVATE || mState.mState == STANDBY || mState.mState == ARREARAGE )//备用，待激活，欠费，都不进行检查
		{
			digitalWrite(RED_LED , HIGH);
			vTaskDelay(1000);
			digitalWrite(RED_LED , LOW);
			Serial.println("欠费");
			continue;
		}
		if ( digitalRead(LOW_PRESSSURE) )//检查是否缺水
		{
			lowWaterTime++;
			Serial.println(lowWaterTime);
			if ( lowWaterTime>=60 )
			{
				//缺水
				digitalWrite(RED_LED , HIGH);
				mState.mState = WATERLITTLE;
				sendEquipmentStatus(WATERLITTLE);
				Serial.println("缺水");
				isLowWater = true;
			}
			continue;
		} else
		{
			
			if ( isLowWater )
			{
				//进行冲洗
				Serial.println("冲洗");
				for ( size_t i = 0; i < 5; i++ )
				{
					digitalWrite(RED_LED , LOW);
					digitalWrite(YELLOW_LED , LOW);
					digitalWrite(GREEN_LED , LOW);

					digitalWrite(GREEN_LED , HIGH);
					vTaskDelay(1000);
					digitalWrite(GREEN_LED , LOW);
					digitalWrite(YELLOW_LED , HIGH);
					vTaskDelay(1000);
					digitalWrite(YELLOW_LED , LOW);
					digitalWrite(RED_LED , HIGH);
					vTaskDelay(1000);
				}
			}
			digitalWrite(RED_LED , LOW);
			isLowWater = false;
			lowWaterTime=0;
		}
		if ( !digitalRead(HIGH_PRESSSURE) )//检查是否缺水
		{
			//水满
			Serial.println("水满");
			digitalWrite(YELLOW_LED , HIGH);
			mState.mState = WATERFULL;
			sendEquipmentStatus(WATERFULL);
			continue;
		} else
		{
			digitalWrite(YELLOW_LED , LOW);
		}
		if ( workNum==0 )
		{
			oneWorktime = millis();
			oneWorktime_ = millis();
		}
		workNum++;
		workNum_++;
		//判断是否达到频繁制水的要求
		if ( millis() - oneWorktime_ < ( 1000 * 60 * 3 ) )//判断3分钟制水有没有超标
		{
			if ( workNum_ > 10 )
			{
				//报检修
				mState.mState = MALFUNCTION;
				sendEquipmentStatus(START_AND_STOP_IN_3_MINUTES);
				Serial.println("频繁启动制水-3分钟");
				while ( true )
				{
					digitalWrite(YELLOW_LED , HIGH);
					digitalWrite(RED_LED , HIGH);
					digitalWrite(GREEN_LED , HIGH);
					vTaskDelay(1000);
					digitalWrite(YELLOW_LED , LOW);
					digitalWrite(RED_LED , LOW);
					digitalWrite(GREEN_LED , LOW);
					vTaskDelay(1000);
				}
			}
		} else
		{
			oneWorktime_ = millis();
		}
		if ( millis()- oneWorktime < (1000*60*60*2) )//先判断有没有达到2小时
		{
			if ( workNum > 48 )
			{
				//报检修
				mState.mState = MALFUNCTION;
				sendEquipmentStatus(START_AND_STOP_IN_2_HOURS);
				Serial.println("频繁启动制水-2小时");
				while ( true )
				{
					digitalWrite(YELLOW_LED , HIGH);
					digitalWrite(RED_LED , HIGH);
					digitalWrite(GREEN_LED , HIGH);
					vTaskDelay(1000);
					digitalWrite(YELLOW_LED , LOW);
					digitalWrite(RED_LED , LOW);
					digitalWrite(GREEN_LED , LOW);
					vTaskDelay(1000);
				}
			}
		} else
		{
			oneWorktime = millis();
		}
		//正常制水，绿灯闪烁
		mState.mState = PROPERWORK;
		uint32_t workTime = 0;//制水时间
		while ( digitalRead(HIGH_PRESSSURE) )
		{
			if ( digitalRead(LOW_PRESSSURE) )
			{
				Serial.println("制水过程中缺水");
				digitalWrite(YELLOW_LED , LOW);
				digitalWrite(GREEN_LED , LOW);

				sendEquipmentStatus(PROPERWORK, workTime);
				break;
			}

			//如果水不满就一直制水
			Serial.println("制水");
			digitalWrite(GREEN_LED , HIGH);
			vTaskDelay(1000);
			workTime++;
			digitalWrite(GREEN_LED , LOW);
			vTaskDelay(1000);
			//if ( workTime > 60 * 60 * 6 )//是否连续工作6小时
			if ( workTime > 10 )
			{
				sendEquipmentStatus(PROPERWORK , workTime);
				vTaskDelay(1000);
				//报检修
				mState.mState = MALFUNCTION;
				sendEquipmentStatus(WORK_CONTINUOUSLY_FOR_6_HOURS);
				Serial.println("连续工作6小时");
				while ( true )
				{
					digitalWrite(YELLOW_LED , HIGH);
					digitalWrite(RED_LED , HIGH);
					digitalWrite(GREEN_LED , HIGH);
					vTaskDelay(1000);
					digitalWrite(YELLOW_LED , LOW);
					digitalWrite(RED_LED , LOW);
					digitalWrite(GREEN_LED , LOW);
					vTaskDelay(1000);
				}
			}
		}
		sendEquipmentStatus(PROPERWORK , workTime);
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
				processServerDeliveryInformation(TcpClient_Buff , &mState);

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
void sendEquipmentStatus(machineState state , int time)
{
	String sendData = getMac();
	sendData += "0C0003";
	sendData += decToHex(state);
	if ( state == PROPERWORK )
	{
		if ( time == 0 )
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
