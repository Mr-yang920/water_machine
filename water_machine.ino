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
//用户私钥，可在控制台获取,修改为自己的UID
#define UID  "90895d3545b31d9fed8e574329798f99"
//主题名字，可在控制台新建
#define TOPIC  "esp32"
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

void setup()
{
    Serial.begin(115200);
    Serial.println(getCRC16("898602b103170011718401002801000000640032003200320010006403E803E807D007D0138803E803E807D007D0138810AABBCCDD"));

    //SerialBT.begin("water");
    //初始化闪存系统
    Serial.print("正在打开闪存系统...");
    while ( !SPIFFS.begin(true) )
    {
        Serial.print("...");
        delay(1000);
    }
    Serial.println("OK!");
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
        char tcpTemp[128];
        sprintf(tcpTemp , "cmd=1&uid=%s&topic=%s\r\n" , UID , TOPIC);

        sendtoTCPServer(tcpTemp);
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
                processServerDeliveryInformation(TcpClient_Buff);

            }
            TcpClient_Buff = "";
        }
        if ( millis() - preHeartTick >= KEEPALIVEATIME )
        {//保持心跳
            preHeartTick = millis();
            Serial.println("--Keep alive:");
            sendtoTCPServer("cmd=0&msg=keep\r\n");
        }
    }
}

void loop()
{
    doTCPClientTick();
}
