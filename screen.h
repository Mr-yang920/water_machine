#pragma once
#include <WString.h>

//display=1��ʾ0����ʾ��displayType=1ȫ��ʾ0��ʾ��߻�����
void sendData(char *data,int dataLen=3);//�������ݵ���Ļ����
void displayWhichScreen(int ScreenNum,int displayNum,bool display = true);//�ڵڼ����������ʾ��0-5��
void displayOriginalWater(bool display = true ,bool displayType=true);//��ʾԭˮ�����ּ�PPM
void displayOriginalWaterNum(String WaterNum, bool display = true , bool displayZero = true);//��ʾԭˮ����ֵ��WaterNumΪadcתˮ�ʵĶ�����displayZero=1��λ��0
void displayCleanWater(bool display = true , bool displayType = true);//��ʾ��ˮ�����ּ�PPM
void displayCleanWaterNum(String WaterNum , bool display = true , bool displayZero = true);//��ʾ����ˮ����ֵ��WaterNumΪadcתˮ�ʵĶ�����displayZero=1��λ��0
void displayWaterDrop(int FPS);//��ʾˮ�ζ�����Ҫ�����n֡����,0֡Ϊ����ʾ
void displayWashing(bool display = true , bool displayType = true);//��ʾ��ϴ����
void displayWaterFull(bool display = true , bool displayType = true);//��ʾˮ������
void displayWaterLittle(bool display = true , bool displayType = true);//��ʾȱˮ����
void displayWaterFix(bool display = true , bool displayType = true);//��ʾά������
void displaySignalIco(bool display = true);//��ʾ����ͼ��
void displayFacilityId(bool display = true);//��ʾ�豸��ID