#include <WiFi.h>
#include <WiFiClient.h>


//�ͷ��Ʒ�������ַĬ�ϼ���
#define TCP_SERVER_ADDR "bemfa.com"

///****************��Ҫ�޸ĵĵط�*****************///

//�������˿�//TCP�����ƶ˿�8344//TCP�豸�ƶ˿�8340
#define TCP_SERVER_PORT "8344"
//WIFI���ƣ����ִ�Сд����Ҫд��
#define DEFAULT_STASSID  "long"
//WIFI����
#define DEFAULT_STAPSW   "yy201011"
//�û�˽Կ�����ڿ���̨��ȡ,�޸�Ϊ�Լ���UID
#define UID  "90895d3545b31d9fed8e574329798f99"
//�������֣����ڿ���̨�½�
#define TOPIC  "esp32"
//��Ƭ��LED����ֵ
const int LED_Pin = 27;

///*********************************************///

//led ���ƺ���
void turnOnLed();
void turnOffLed();


//����ֽ���
#define MAX_PACKETSIZE 512
//��������ֵ30s
#define KEEPALIVEATIME 30*1000




//tcp�ͻ�����س�ʼ����Ĭ�ϼ���
WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;//����
unsigned long preTCPStartTick = 0;//����
bool preTCPConnected = false;



//��غ�����ʼ��
//����WIFI
void doWiFiTick();
void startSTA();

//TCP��ʼ������
void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);





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
            startTCPClient();
    } else
    {
        if ( TCPclient.available() )
        {//������
            char c = TCPclient.read();
            TcpClient_Buff += c;
            TcpClient_BuffIndex++;
            TcpClient_preTick = millis();

            if ( TcpClient_BuffIndex >= MAX_PACKETSIZE - 1 )
            {
                TcpClient_BuffIndex = MAX_PACKETSIZE - 2;
                TcpClient_preTick = TcpClient_preTick - 200;
            }
            preHeartTick = millis();
        }
        if ( millis() - preHeartTick >= KEEPALIVEATIME )
        {//��������
            preHeartTick = millis();
            Serial.println("--Keep alive:");
            sendtoTCPServer("cmd=0&msg=keep\r\n");
        }
    }
    if ( ( TcpClient_Buff.length() >= 1 ) && ( millis() - TcpClient_preTick >= 200 ) )
    {//data ready
        TCPclient.flush();
        Serial.println("Buff");
        Serial.println(TcpClient_Buff);
        if ( ( TcpClient_Buff.indexOf("&msg=on") > 0 ) )
        {
            turnOnLed();
        } else if ( ( TcpClient_Buff.indexOf("&msg=off") > 0 ) )
        {
            turnOffLed();
        }
        TcpClient_Buff = "";
        TcpClient_BuffIndex = 0;
    }
}

void startSTA()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(DEFAULT_STASSID , DEFAULT_STAPSW);

}



/**************************************************************************
                                 WIFI
***************************************************************************/
/*
  WiFiTick
  ����Ƿ���Ҫ��ʼ��WiFi
  ���WiFi�Ƿ������ϣ������ӳɹ�����TCP Client
  ����ָʾ��
*/
void doWiFiTick()
{
    static bool startSTAFlag = false;
    static bool taskStarted = false;
    static uint32_t lastWiFiCheckTick = 0;

    if ( !startSTAFlag )
    {
        startSTAFlag = true;
        startSTA();
        Serial.printf("Heap size:%d\r\n" , ESP.getFreeHeap());
    }

    //δ����1s����
    if ( WiFi.status() != WL_CONNECTED )
    {
        if ( millis() - lastWiFiCheckTick > 1000 )
        {
            lastWiFiCheckTick = millis();
        }
    }
    //���ӳɹ�����
    else
    {
        if ( taskStarted == false )
        {
            taskStarted = true;
            Serial.print("\r\nGet IP Address: ");
            Serial.println(WiFi.localIP());
            startTCPClient();
        }
    }
}
//�򿪵���
void turnOnLed()
{
    Serial.println("Turn ON");
    digitalWrite(LED_Pin , LOW);
}
//�رյ���
void turnOffLed()
{
    Serial.println("Turn OFF");
    digitalWrite(LED_Pin , HIGH);
}


// ��ʼ�����൱��main ����
void setup()
{
    Serial.begin(115200);
    Serial.println(ESP.getSketchMD5());
    pinMode(LED_Pin , OUTPUT);
    digitalWrite(LED_Pin , HIGH);
}

//ѭ��
void loop()
{
    doWiFiTick();
    doTCPClientTick();
}