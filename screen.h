#pragma once
#include <WString.h>


//display=1��ʾ0����ʾ��displayType=1ȫ��ʾ0��ʾ��߻�����
void sendData(char *data,int dataLen);
void displayOriginalWater(bool display = true ,bool displayType=true);
void displayOriginalWaterNum(int WaterNum, bool display = true , bool displayZero = true);
void displayCleanWater(bool display = true , bool displayType = true);
void displayCleanWaterNum(int WaterNum , bool display = true , bool displayZero = true);
void displayWaterDrop(int FPS);//Ҫ�����n�붯��
void displayWashing(bool display = true , bool displayType = true);
void displayWaterFull(bool display = true , bool displayType = true);
void displayWaterLittle(bool display = true , bool displayType = true);
void displayWaterFix(bool display = true , bool displayType = true);
void displaySignalIco(bool display = true , bool displayType = true);
void displayFacilityId(String ID,bool display = true , bool displayType = true);