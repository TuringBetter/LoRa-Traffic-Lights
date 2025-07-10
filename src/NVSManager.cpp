#include "NVSManager.h"

// 定义NVS命名空间和键名
#define NVS_NAMESPACE_LORA_MULTICAST "lora_multicast" // 用于存储LoRa多播相关配置的NVS命名空间
#define NVS_KEY_DEV_ADDR             "dev_addr"       // 存储DevAddr
#define NVS_KEY_APP_SKEY             "app_skey"       // 存储AppSKey
#define NVS_KEY_NWK_SKEY             "nwk_skey"       // 存储NwkSKey

// 定义ESP_LOG_TAG，用于esp_log输出系统，标识本模块的日志来源
static const char* TAG = "NVS_MANAGER";

// 始化NVS Flash分区，并检查NVS分区的完整性。
void NVS_init() {
    // 调用esp-idf的nvs_flash_init()函数初始化NVS Flash。
    esp_err_t ret = nvs_flash_init();
    
    // 检查初始化结果。
    // ESP_ERR_NVS_NO_FREE_PAGES: 表示NVS分区已满，没有更多空闲页面可供写入。
    // ESP_ERR_NVS_NEW_VERSION_FOUND: 表示Flash上的NVS数据格式与当前NVS库版本不兼容，需要更新。
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 当遇到上述错误时擦除整个NVS分区
        ESP_LOGW(TAG, "NVS partition is full or new version found, erasing...");
        // 调用nvs_flash_erase()擦除整个NVS分区
        ESP_ERROR_CHECK(nvs_flash_erase()); 
        // 擦除后再次尝试初始化NVS。
        ret = nvs_flash_init();
    }
    // 再次检查初始化结果。如果此时仍然失败，表示出现了严重的硬件或配置问题。
    // ESP_ERROR_CHECK宏确保如果NVS初始化最终失败，程序会停止，防止后续操作出现未定义行为。
    ESP_ERROR_CHECK(ret); 
    
    // NVS初始化成功，打印信息日志。
    ESP_LOGI(TAG, "NVS initialized.");
}

/**
    保存LoRa多播组的凭证到NVS。
    DevAddr, AppSKey, NwkSKey这三个字符串作为键值对保存到NVS的指定命名空间下。
    如果所有凭证都成功保存并提交到Flash，则 true否则返回 false。
 */
bool NVS_saveLoRaMulticast(const String& devAddr, const String& appSKey, const String& nwkSKey) {
    nvs_handle_t nvs_handle; // 声明一个NVS句柄，用于后续的NVS操作
    
    // NVS_READWRITE模式打开NVS命名空间进行读写操作
    esp_err_t err = nvs_open(NVS_NAMESPACE_LORA_MULTICAST, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        // 如果打开NVS句柄失败，打印错误日志并返回false。
        ESP_LOGE(TAG, "Error (%s) opening NVS handle for writing!", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "NVS handle opened for writing. Saving new multicast data...");

    // 写入DevAddr字符串。
    // nvs_set_str()将字符串与指定的键名关联起来。
    // devAddr.c_str()将Arduino String对象转换为C风格的char*字符串，NVS API需要这种类型。
    err = nvs_set_str(nvs_handle, NVS_KEY_DEV_ADDR, devAddr.c_str());
    if (err != ESP_OK) { 
        // 如果写入失败，打印错误日志，关闭句柄，并返回false。
        ESP_LOGE(TAG, "Failed to write DevAddr (%s)", esp_err_to_name(err)); 
        nvs_close(nvs_handle); 
        return false; 
    }

    // 写入AppSKey字符串
    err = nvs_set_str(nvs_handle, NVS_KEY_APP_SKEY, appSKey.c_str());
    if (err != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to write AppSKey (%s)", esp_err_to_name(err)); 
        nvs_close(nvs_handle); 
        return false; 
    }

    // 写入NwkSKey字符串
    err = nvs_set_str(nvs_handle, NVS_KEY_NWK_SKEY, nwkSKey.c_str());
    if (err != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to write NwkSKey (%s)", esp_err_to_name(err)); 
        nvs_close(nvs_handle); 
        return false; 
    }

    // nvs_commit()提交更改到Flash。
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        // 如果提交失败，打印错误日志，关闭句柄，并返回false。
        ESP_LOGE(TAG, "Failed to commit NVS changes (%s)", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }

    nvs_close(nvs_handle); // 关闭NVS句柄，释放资源。
    ESP_LOGI(TAG, "New multicast data saved to NVS."); 
    return true;
}

/**
    从NVS加载LoRa多播组的凭证。
    尝试从NVS的指定命名空间读取DevAddr, AppSKey, NwkSKey。
    如果成功加载了所有三个凭证，则 true否则返回 false（例如，命名空间不存在、键不存在或读取失败）。
 */
bool NVS_loadLoRaMulticast(String& devAddr, String& appSKey, String& nwkSKey) {
    nvs_handle_t nvs_handle; // 声明NVS句柄
    
    // NVS_READONLY打开NVS命名空间进行只读操作
    esp_err_t err = nvs_open(NVS_NAMESPACE_LORA_MULTICAST, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        // 如果打开失败，通常意味着指定的命名空间不存在（可能是第一次运行，还没有保存过数据）。
        // 打印警告日志，并返回false。
        ESP_LOGW(TAG, "Error (%s) opening NVS handle for reading. Probably no data yet.", esp_err_to_name(err));
        return false; 
    }

    ESP_LOGI(TAG, "NVS handle opened for reading. Loading multicast data...");
    size_t required_size; // 用于存储读取字符串所需的缓冲区大小
    bool all_found = true; // 标志，用于判断是否所有必要的凭证都成功找到并读取

    // --- 读取 DevAddr ---
    // 第一次调用nvs_get_str()，将value参数设为NULL，只获取字符串的实际长度。
    // required_size会被填充为字符串的长度（包括空终止符）。
    err = nvs_get_str(nvs_handle, NVS_KEY_DEV_ADDR, NULL, &required_size);
    // 检查获取长度的结果。ESP_OK表示键存在且成功获取长度，required_size > 0表示字符串非空。
    if (err == ESP_OK && required_size > 0) {
        // 根据required_size在栈上分配一个缓冲区。
        char devAddr_buf[required_size];
        // 第二次调用nvs_get_str()，将数据读取到缓冲区中。
        nvs_get_str(nvs_handle, NVS_KEY_DEV_ADDR, devAddr_buf, &required_size);
        devAddr = String(devAddr_buf); // 将C风格字符串转换为Arduino String对象
    } else {
        // 如果DevAddr键不存在或读取失败，打印警告，并将all_found设为false。
        ESP_LOGW(TAG, "DevAddr not found or error (%s)", esp_err_to_name(err));
        all_found = false;
    }

    // --- 读取 AppSKey ---
    err = nvs_get_str(nvs_handle, NVS_KEY_APP_SKEY, NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
        char appSKey_buf[required_size];
        nvs_get_str(nvs_handle, NVS_KEY_APP_SKEY, appSKey_buf, &required_size);
        appSKey = String(appSKey_buf);
    } else {
        ESP_LOGW(TAG, "AppSKey not found or error (%s)", esp_err_to_name(err));
        all_found = false;
    }

    // --- 读取 NwkSKey ---
    err = nvs_get_str(nvs_handle, NVS_KEY_NWK_SKEY, NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
        char nwkSKey_buf[required_size];
        nvs_get_str(nvs_handle, NVS_KEY_NWK_SKEY, nwkSKey_buf, &required_size);
        nwkSKey = String(nwkSKey_buf);
    } else {
        ESP_LOGW(TAG, "NwkSKey not found or error (%s)", esp_err_to_name(err));
        all_found = false;
    }

    nvs_close(nvs_handle); // 关闭NVS句柄。

    if (all_found) {
        // 如果所有凭证都成功加载，打印信息日志。
        ESP_LOGI(TAG, "Loaded complete multicast data from NVS.");
        // 使用ESP_LOGD（调试级别日志）打印详细信息，这些信息在默认日志级别下不会显示。
        ESP_LOGD(TAG, "DevAddr: %s", devAddr.c_str()); 
        ESP_LOGD(TAG, "AppSKey: %s", appSKey.c_str());
        ESP_LOGD(TAG, "NwkSKey: %s", nwkSKey.c_str());
    } else {
        // 如果有任何凭证缺失或读取失败，打印警告日志。
        ESP_LOGW(TAG, "Incomplete or missing multicast data in NVS.");
    }
    
    return all_found;
}