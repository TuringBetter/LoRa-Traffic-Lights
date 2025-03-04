#pragma once

enum DataRate {
    SF12 = 0, 
    SF11, 
    SF10, 
    SF9, 
    SF8, 
    SF7, 
    SF6, 
    SF5
};

enum Bandwidth {
    BW_125KHz = 0, BW_250KHz, BW_500KHz, BW_62_5KHz, BW_41_67KHz,
    BW_31_25KHz, BW_20_83KHz, BW_15_63KHz, BW_10_42KHz, BW_7_81KHz
};

enum CodeRate {
    CR_4_5 = 1, CR_4_6, CR_4_7, CR_4_8
};

enum IqConverted
{
    IQ_OFF = 0,IQ_ON
};

struct LoRaRxConfigStruct {
    long freq; // 频率
    DataRate dataRate; // 速率
    Bandwidth bandwidth; // 带宽
    CodeRate codeRate; // 编码率
    IqConverted iqConverted; // IQ转换功能
};

class LoRaModule {
public:
    void begin();   //初始化LoRa模块
    void setTxConfig(long freq, int dataRate, int bandwidth, int codeRate, int power, int iqConverted); //设置发射参数
    bool setRxConfig(const LoRaRxConfigStruct* pConfig);    //设置接受参数
    bool setLocalAddress(int localAddr);    //设置本地地址
    bool setTargetAddress(int targetAddr);  //设置目标地址
    void setSleepMode(int sleepMode);       //休眠模式
};
