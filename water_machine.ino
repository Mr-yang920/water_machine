#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <FS.h>
//#include <BluetoothSerial.h>
#include <WiFi.h>
#include "water_class.h"
#include <WiFiClient.h>


//��������ַ
#define TCP_SERVER_ADDR "192.168.31.176"
//�������˿ں�
#define TCP_SERVER_PORT "1024"
//�û�˽Կ�����ڿ���̨��ȡ,�޸�Ϊ�Լ���UID
#define UID  "90895d3545b31d9fed8e574329798f99"
//�������֣����ڿ���̨�½�
#define TOPIC  "esp32"
//����ֽ���
#define MAX_PACKETSIZE 512
//��������ֵ30s
#define KEEPALIVEATIME 30*1000
//��������ӵĿͻ�����Ŀ���ֵ
#define MAX_SRV_CLIENTS 2

//tcp�ͻ�����س�ʼ����Ĭ�ϼ���
WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;//�ͻ������һ�κͷ�����ͨѶ��ʱ��
unsigned long preHeartTick = 0;//����
unsigned long preTCPStartTick = 0;//����
bool preTCPConnected = false;//�ͷ�����������״̬

void setup()
{
    Serial.begin(115200);
    Serial.println(getCRC16("898602b103170011718401002801000000640032003200320010006403E803E807D007D0138803E803E807D007D0138810AABBCCDD"));

    //SerialBT.begin("water");
    //��ʼ������ϵͳ
    Serial.print("���ڴ�����ϵͳ...");
    while ( !SPIFFS.begin(true) )
    {
        Serial.print("...");
        delay(1000);
    }
    Serial.println("OK!");
    if ( SPIFFS.exists("/wifiData") )
    {
        Serial.println("����wifi��Ϣ�����ڿ�ʼ��������wifi");
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
            //���wifi���ӳɹ�����ʼ���ӷ�����
            Serial.println(getMac());
        }
    } else
    {
        disposeErrData(WIFI_INFO_NOT_FOUND);
        configWifi();
    }

}


/*
  *�������ݵ�TCP������
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
  *��ʼ���ͷ�������������
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
  *������ݣ���������
*/
void doTCPClientTick()
{
    //����Ƿ�Ͽ����Ͽ�������
    if ( WiFi.status() != WL_CONNECTED ) return;

    if ( !TCPclient.connected() )
    {//�Ͽ�����

        if ( preTCPConnected == true )
        {

            preTCPConnected = false;
            preTCPStartTick = millis();
            Serial.println();
            Serial.println("TCP Client disconnected.");
            TCPclient.stop();
        } else if ( millis() - preTCPStartTick > 1 * 1000 )//��������
        {
            startTCPClient();
        }

    } else
    {
        if ( TCPclient.available() )
        {//������
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
        {//��������
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
