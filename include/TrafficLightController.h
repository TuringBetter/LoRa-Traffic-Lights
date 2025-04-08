#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "LedModule.h"
#include "AccelerometerModule.h"
#include "LaserModule.h"
#include "LoRaModule.h"

// 交通灯状态枚举
enum class TrafficLightState {
    YELLOW_CONSTANT,  // 黄灯常亮
    RED_BLINKING,     // 红灯闪烁
    REMOTE_CONTROLLED // 远程控制状态
};

// 交通灯消息类型枚举
enum class MessageType {
    VEHICLE_APPROACHING,  // 车辆接近
    VEHICLE_PASSED,       // 车辆已通过
    REMOTE_CONTROL        // 远程控制命令
};

// 交通灯消息结构体
struct TrafficLightMessage {
    MessageType type;
    uint32_t senderId;
    uint32_t targetId;
    String payload;  // 额外数据，如远程控制命令
};

// 交通灯控制器类
class TrafficLightController {
public:
    TrafficLightController(uint32_t lightId);
    void begin();
    void update();

private:
    // 任务函数
    static void accelerometerTask(void* parameter);
    static void laserTask(void* parameter);
    static void loraTask(void* parameter);
    static void ledTask(void* parameter);

    // 辅助函数
    void processLaserData(int16_t distance);
    void processAccelerometerData(int16_t x, int16_t y, int16_t z);
    void processLoRaMessage(const RecvInfo& recvInfo);
    void sendLoRaMessage(MessageType type, uint32_t targetId, const String& payload = "");
    void handleRemoteControl(const String& command);
    void updateLedState();

    // 成员变量
    uint32_t            _lightId;
    TrafficLightState   _currentState;
    bool                _vehicleDetected;
    bool                _collisionDetected;
    uint32_t            _lastVehicleDetectionTime;
    uint32_t            _lastCollisionDetectionTime;
    
    // 模块实例
    Led             _ledModule;
    Accelerometer   _accelerometer;
    Laser           _laser;
    LoRaModule      _loraModule;
    
    // 配置参数
    static const int        VEHICLE_DETECTION_THRESHOLD = 100;  // 车辆检测阈值（厘米）
    static const int        COLLISION_THRESHOLD = 2000;         // 碰撞检测阈值
    static const uint32_t   VEHICLE_TIMEOUT = 10000;       // 车辆超时时间（毫秒）
    static const uint32_t   COLLISION_TIMEOUT = 5000;      // 碰撞超时时间（毫秒）
    
    // 任务句柄
    TaskHandle_t _accelerometerTaskHandle;
    TaskHandle_t _laserTaskHandle;
    TaskHandle_t _loraTaskHandle;
    TaskHandle_t _ledTaskHandle;
    
    // 互斥锁
    SemaphoreHandle_t _stateMutex;
}; 