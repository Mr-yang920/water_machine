#pragma once
#include <WString.h>


//display=1显示0不显示，displayType=1全显示0显示左边或上面
void sendData(char *data,int dataLen);
void displayOriginalWater(bool display = true ,bool displayType=true);
void displayOriginalWaterNum(int WaterNum, bool display = true , bool displayZero = true);
void displayCleanWater(bool display = true , bool displayType = true);
void displayCleanWaterNum(int WaterNum , bool display = true , bool displayZero = true);
void displayWaterDrop(int FPS);//要求传入第n针动画
void displayWashing(bool display = true , bool displayType = true);
void displayWaterFull(bool display = true , bool displayType = true);
void displayWaterLittle(bool display = true , bool displayType = true);
void displayWaterFix(bool display = true , bool displayType = true);
void displaySignalIco(bool display = true , bool displayType = true);
void displayFacilityId(String ID,bool display = true , bool displayType = true);