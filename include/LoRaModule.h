#pragma once
#include <Arduino.h>
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

enum TransferMode {
    RX_MODE = 0, 
    TX_MODE
};


struct LoRaTransConfigStruct {
    long freq;              // 频率
    DataRate dataRate;      // 速率
    Bandwidth bandwidth;    // 带宽
    CodeRate codeRate;      // 编码率
    IqConverted iqConverted;// IQ转换功能
    uint8_t power;          // 发射功率
};

class LoRaModule {
public:
    void begin();   //初始化LoRa模块
    bool setTxConfig(const LoRaTransConfigStruct* pConfig); //设置发射参数
    bool setRxConfig(const LoRaTransConfigStruct* pConfig);    //设置接受参数
    bool setLocalAddress(uint32_t localAddr);    //设置本地地址
    bool setTargetAddress(uint32_t targetAddr);  //设置目标地址
    void setSleepMode(int sleepMode);       //休眠模式
    bool sendData(const String &sendData);  //发射数据

private:
    TransferMode _currentTransferMode;

    long _currentFreq;               // 当前频率
    DataRate _currentDataRate;       // 当前速率
    Bandwidth _currentBandwidth;     // 当前带宽
    CodeRate _currentCodeRate;       // 当前编码率
    IqConverted _currentIqConverted; // IQ转换功能
    uint8_t _power;                  // 发射功率

    uint32_t _localAddr;
    uint32_t _targetAddr;
};
