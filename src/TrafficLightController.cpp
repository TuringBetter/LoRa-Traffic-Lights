#include "TrafficLightController.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// 构造函数
TrafficLightController::TrafficLightController(uint32_t lightId) 
    :   _lightId(lightId),
        _currentState(TrafficLightState::YELLOW_CONSTANT),
        _vehicleDetected(false),
        _collisionDetected(false),
        _lastVehicleDetectionTime(0),
        _lastCollisionDetectionTime(0){
    
    // 创建互斥锁
    _stateMutex = xSemaphoreCreateMutex();
}

// 初始化函数
void TrafficLightController::begin() {
    // 初始化串口
    Serial.println("Traffic Light Controller Initializing...");
    
    /*
    // 初始化LoRa模块
    _loraModule.begin();
    
    // 配置LoRa参数
    LoRaTransConfigStruct txConfig = {
        .freq = 915000000,  // 915MHz
        .dataRate = SF7,
        .bandwidth = BW_125KHz,
        .codeRate = CR_4_5,
        .iqConverted = IQ_OFF,
        .power = 20
    };
    
    _loraModule.setTxConfig(&txConfig);
    _loraModule.setRxConfig(&txConfig);
    _loraModule.setLocalAddress(_lightId);
    */
    // 创建任务
    /*
    xTaskCreatePinnedToCore(
        accelerometerTask,
        "AccelerometerTask",
        4096,
        this,
        1,
        &_accelerometerTaskHandle,
        1
    );
    */
    xTaskCreatePinnedToCore(
        laserTask,
        "LaserTask",
        4096,
        this,
        1,
        &_laserTaskHandle,
        1
    );
    /*
    xTaskCreatePinnedToCore(
        loraTask,
        "LoRaTask",
        8192,
        this,
        1,
        &_loraTaskHandle,
        1
    );
    */
    /*
    xTaskCreatePinnedToCore(
        ledTask,
        "LedTask",
        4096,
        this,
        1,
        &_ledTaskHandle,
        1
    );
    */
}

// 更新函数
void TrafficLightController::update() {
    // 主循环中不需要做任何事情，所有工作都在任务中完成
}

// 加速度计任务
void TrafficLightController::accelerometerTask(void* parameter) {
    TrafficLightController* controller = (TrafficLightController*) parameter;

    if (!controller->_accelerometer.begin())
        Serial.println("Failed to initialize accelerometer!");
    Serial.println("accelerometer module initialized");

    while (true) {
        int16_t x, y, z;
        controller->_accelerometer.readRaw(x, y, z);
        
        // 处理加速度数据
        controller->processAccelerometerData(x, y, z);
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms
    }
}

// 激光测距任务
void TrafficLightController::laserTask(void* parameter) {
    TrafficLightController* controller = (TrafficLightController*) parameter;

    controller->_laser.begin();
    // 初始化激光模块，发送一次读取命令使其进入测量状态
    controller->_laser.sendReadCommand();
    Serial.println("Laser module initialized and set to continuous measurement mode");
    
    
    while (true) {
        // 读取距离数据
        int16_t distance = controller->_laser.receiveReadResponse();
        
        // 处理激光数据
        if (distance >= 0) {
            controller->processLaserData(distance);
        }
        vTaskDelay(pdMS_TO_TICKS(5));  // 5ms延时，确保能捕获所有数据
    }
}

// LoRa任务
void TrafficLightController::loraTask(void* parameter) {
    TrafficLightController* controller = (TrafficLightController*) parameter;
    
    while (true) {
        // 接收LoRa消息
        RecvInfo recvInfo;
        uint64_t result = controller->_loraModule.receiveData(recvInfo);
        
        if (result > 0) {
            // 处理接收到的消息
            controller->processLoRaMessage(recvInfo);
        }
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms
    }
}

// LED任务
void TrafficLightController::ledTask(void* parameter) {
    TrafficLightController* controller = (TrafficLightController*) parameter;

    while (true) {
        // 更新LED状态
        controller->updateLedState();
        
        // 更新LED模块
        controller->_ledModule.update();
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms
    }
}

// 处理激光数据
void TrafficLightController::processLaserData(int16_t distance) {
    // 检查是否有车辆接近
    if (distance < VEHICLE_DETECTION_THRESHOLD) {
        if (!_vehicleDetected) {
            _vehicleDetected = true;
            _lastVehicleDetectionTime = millis();
            
            // 发送车辆接近消息
            sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 1);
            sendLoRaMessage(MessageType::VEHICLE_APPROACHING, _lightId + 2);
            
            // 更新LED状态
            if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
                _currentState = TrafficLightState::RED_BLINKING;
                xSemaphoreGive(_stateMutex);
            }
        }
    } else {
        // 检查车辆是否已经通过
        if (_vehicleDetected && (millis() - _lastVehicleDetectionTime > VEHICLE_TIMEOUT)) {
            _vehicleDetected = false;
            
            // 发送车辆已通过消息
            sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 1);
            sendLoRaMessage(MessageType::VEHICLE_PASSED, _lightId + 2);
            
            // 更新LED状态
            if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
                if (_currentState == TrafficLightState::RED_BLINKING) {
                    _currentState = TrafficLightState::YELLOW_CONSTANT;
                }
                xSemaphoreGive(_stateMutex);
            }
        }
    }
}

// 处理加速度计数据
void TrafficLightController::processAccelerometerData(int16_t x, int16_t y, int16_t z) {
    static float scale{0.061f / 1000};
    // 计算加速度的绝对值
    double acceleration = abs(x * scale)*abs(x * scale) + abs(y * scale)*abs(y * scale) + abs(z * scale)*abs(z * scale);

    // Serial.println(acceleration);

    // 检查是否检测到碰撞
    if (acceleration > COLLISION_THRESHOLD) {
        if (!_collisionDetected) {
            _collisionDetected = true;
            _lastCollisionDetectionTime = millis();
            
            // 打印碰撞信息
            Serial.printf("Collision detected! Acceleration: %.3lf\n", acceleration);
        }
    } else {
        // 检查碰撞状态是否已经恢复
        if (_collisionDetected && (millis() - _lastCollisionDetectionTime > COLLISION_TIMEOUT)) {
            _collisionDetected = false;
        }
    }
}

// 处理LoRa消息
void TrafficLightController::processLoRaMessage(const RecvInfo& recvInfo) {
    // 解析消息
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, recvInfo.message);
    
    if (error) {
        Serial.println("Failed to parse LoRa message");
        return;
    }
    
    // 获取消息类型
    int type = doc["type"];
    uint32_t senderId = doc["senderId"];
    uint32_t targetId = doc["targetId"];
    String payload = doc["payload"] | "";
    
    // 检查消息是否针对当前交通灯
    if (targetId != _lightId && targetId != 0) {
        return;
    }
    
    // 处理不同类型的消息
    switch ((MessageType)type) {
        case MessageType::VEHICLE_APPROACHING:
            // 车辆接近消息
            if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
                _currentState = TrafficLightState::RED_BLINKING;
                xSemaphoreGive(_stateMutex);
            }
            break;
            
        case MessageType::VEHICLE_PASSED:
            // 车辆已通过消息
            if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
                if (_currentState == TrafficLightState::RED_BLINKING) {
                    _currentState = TrafficLightState::YELLOW_CONSTANT;
                }
                xSemaphoreGive(_stateMutex);
            }
            break;
            
        case MessageType::REMOTE_CONTROL:
            // 远程控制消息
            handleRemoteControl(payload);
            break;
            
        default:
            Serial.println("Unknown message type");
            break;
    }
}

// 发送LoRa消息
void TrafficLightController::sendLoRaMessage(MessageType type, uint32_t targetId, const String& payload) {
    /*
    // 设置目标地址
    _loraModule.setTargetAddress(targetId);
    
    // 创建JSON消息
    StaticJsonDocument<200> doc;
    doc["type"] = (int)type;
    doc["senderId"] = _lightId;
    doc["targetId"] = targetId;
    doc["payload"] = payload;
    
    String message;
    serializeJson(doc, message);
    
    // 发送消息
    _loraModule.sendData(message);
    */
    Serial.println("Send message to "+targetId);
}

// 处理远程控制命令
void TrafficLightController::handleRemoteControl(const String& command) {
    if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
        if (command == "yellow") {
            _currentState = TrafficLightState::YELLOW_CONSTANT;
        } else if (command == "red") {
            _currentState = TrafficLightState::RED_BLINKING;
        }
        xSemaphoreGive(_stateMutex);
    }
}

// 更新LED状态
void TrafficLightController::updateLedState() {
    if (xSemaphoreTake(_stateMutex, portMAX_DELAY) == pdTRUE) {
        switch (_currentState) {
            case TrafficLightState::YELLOW_CONSTANT:
                _ledModule.setColor(LedColor::YELLOW);
                _ledModule.setBrightness(500);
                _ledModule.setFrequency(0);  // 常亮模式
                break;
                
            case TrafficLightState::RED_BLINKING:
                _ledModule.setColor(LedColor::RED);
                _ledModule.setBrightness(7000);
                _ledModule.setFrequency(60);  // 60Hz闪烁
                break;
                
            case TrafficLightState::REMOTE_CONTROLLED:
                // 远程控制状态，LED状态由远程控制命令设置
                break;
                
            default:
                break;
        }
        xSemaphoreGive(_stateMutex);
    }
} 