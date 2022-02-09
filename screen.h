#pragma once
#include <WString.h>

//display=1显示0不显示，displayType=1全显示0显示左边或上面
void sendData(char *data,int dataLen=3);//发送数据到屏幕驱动
void displayWhichScreen(int ScreenNum,int displayNum,bool display = true);//在第几个数码管显示（0-5）
void displayOriginalWater(bool display = true ,bool displayType=true);//显示原水两个字及PPM
void displayOriginalWaterNum(String WaterNum, bool display = true , bool displayZero = true);//显示原水的数值，WaterNum为adc转水质的读数，displayZero=1低位补0
void displayCleanWater(bool display = true , bool displayType = true);//显示净水两个字及PPM
void displayCleanWaterNum(String WaterNum , bool display = true , bool displayZero = true);//显示纯净水的数值，WaterNum为adc转水质的读数，displayZero=1低位补0
void displayWaterDrop(int FPS);//显示水滴动画，要求传入第n帧动画,0帧为不显示
void displayWashing(bool display = true , bool displayType = true);//显示清洗区域
void displayWaterFull(bool display = true , bool displayType = true);//显示水满区域
void displayWaterLittle(bool display = true , bool displayType = true);//显示缺水区域
void displayWaterFix(bool display = true , bool displayType = true);//显示维修区域
void displaySignalIco(bool display = true);//显示网络图标
void displayFacilityId(bool display = true);//显示设备的ID