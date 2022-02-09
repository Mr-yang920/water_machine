#pragma once
#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <WiFi.h>
#include <WString.h> 
#include <MD5.h>
//#include <ArduinoJson.h>
#include <HTTPUpdate.h>

#define MAX_SRV_CLIENTS 2
#define OTA_PUBLIC_PWD "0123456789"

enum errType
{
	ILLEGAL_WIFI_INFORMATION ,//客户端传递的wif信息非法
	JSON_PROCESSOR_ERR ,      //json处理器错误
	WIFI_CONN_TIMEOUT ,       //wifi连接超时
	WIFI_INFO_NOT_FOUND ,     //找不到wifi配置文件
}; 
enum machineState
{
	STANDBY ,       //备用
	DEACTIVATE ,    //待激活
	FACTORYTEST ,   //出厂测试状态
	PROPERWORK ,    //正常制水
	ARREARAGE ,     //欠费
	MALFUNCTION ,   // 故障
	SHUTDOWN ,      //关机
	WATERFULL ,     //水满
	WATERLITTLE ,   //缺水
	WATERMAKE ,     //漏水
	LXTOBERESET ,   //滤芯待复位
	HARDWARE ,      //硬件测试
	WORK_CONTINUOUSLY_FOR_6_HOURS = 5 ,//连续制水6小时
	START_AND_STOP_IN_3_MINUTES = 10 ,  //3分钟内启停故障
	START_AND_STOP_IN_2_HOURS ,    //2小时内启停故障
	SIM_CARD_LOST ,                //SIM卡丢失
	WIFI_LOST ,                    //Wifi断开
};

struct calculateDay
{
	bool state = false;//true:day_1>day_2 false:day_1<day_2/未进行处理（具体取决于day的值）
	int day = -1;//验证后返回的消息（-1输入不合法）
};
struct heartbeatConfig
{
	machineState mState= DEACTIVATE;          //设备状态，默认待激活
	bool screenState=false;             //屏幕状态,t关闭f打开
	bool workMode = false;                //工作模式,t时间f流量
	int16_t Surplus_Flow = 0;    //剩余流量
	int16_t Surplus_Days = 0;    //剩余天数
	const uint16_t Already_Days = 0;    //已用天数
	const uint16_t Already_Flow = 0;    //已用流量
	int16_t clearwaterTDS= 0;       //净水TDS
	int16_t tapWaterTDS = 0;         //原水TDS
	const uint8_t residue_ON1 = 0;//第一级滤芯剩余值
	const uint8_t residue_ON2 = 0;//第二级滤芯剩余值
	const uint8_t residue_ON3 = 0;//第三级滤芯剩余值
	const uint8_t residue_ON4 = 0;//第四级滤芯剩余值
	const uint8_t residue_ON5 = 0;//第五级滤芯剩余值
	//uint16_t MAX_ON1 = 0;         //第一级滤芯最大值
	//uint16_t MAX_ON2 = 0;         //第二级滤芯最大值
	//uint16_t MAX_ON3 = 0;         //第三级滤芯最大值
	//uint16_t MAX_ON4 = 0;         //第四级滤芯最大值
	//uint16_t MAX_ON5 = 0;         //第五级滤芯最大值
	uint16_t LX_MAX[5] = {};//5个滤芯最大值
	uint8_t RSSI = -1;                 //信号强度值
	const uint8_t LAC_val = 0;    //LAC值
	const uint8_t CID_val = 0;    //CID值
};

bool connWifi(String data);//连接到wifi
void configWifi();//配置wifi，从app获取
void saveWifiData(String wifiData);//保存wifi信息
void disposeErrData(errType err);//反馈错误信息
String getMac();//得到设备mac地址20位
String getEquipmentID();//得到设备的的ID号（MAC后6位）
String decToHex(int Dec);//10进制转16进制
int hexToDec(String Hex);//16进制转10进制
struct calculateDay differenceDay(String day_1 , String day_2);//计算两个日期的差值
String sendHeartbeat(heartbeatConfig mState);//发送心跳包
uint16_t CRC16_Modbus(uint8_t* buf , uint16_t len);//获取数据CRC16值（原生算法）
uint16_t getCRC16(String data);//获取数据CRC16值
void processServerDeliveryInformation(String order,heartbeatConfig *mState);//解析服务器发送的命令 
void sendEquipmentStatus(machineState state, int time=0);//设备状态发生改变时向服务器上报信息
//void test(heartbeatConfig state);
//void test1(heartbeatConfig *state);
String getOtaPwd(String otaAddress ,String time);
bool isOta(String JSONData);
void sendtoTCPServer(String p);
String updataVersion();