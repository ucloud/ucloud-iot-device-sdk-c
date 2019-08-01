/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef C_SDK_UIOT_EXPORT_SHADOW_H_
#define C_SDK_UIOT_EXPORT_SHADOW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uiot_export_mqtt.h"
#include "mqtt_client.h"

/**
 * @brief 请求响应返回的类型
 */
typedef enum {
    ACK_NONE = -3,      // 请求超时
    ACK_TIMEOUT = -2,   // 请求超时
    ACK_REJECTED = -1,  // 请求拒绝
    ACK_ACCEPTED = 0    // 请求接受
} RequestAck;

/**
 * @brief 操作云端设备文档可以使用的三种方式
 */
typedef enum {
    GET,                    // 获取云端设备文档
    UPDATE,                 // 更新或创建云端设备文档
    UPDATE_AND_RESET_VER,   // 更新同时重置版本号
    DELETE,                 // 删除部分云端设备文档
    DELETE_ALL,             // 删除全部云端设备文档中的属性,不需要一个个添加需要删除的属性
    REPLY_CONTROL_UPDATE,   // 设备处理完服务端的control消息后回应的update消息
    REPLY_CONTROL_DELETE,   // 设备处理完服务端的control消息后回应的delete消息
} Method;

/**
 * @brief JSON文档中支持的数据类型
 */
typedef enum {
    JINT32,     // 32位有符号整型
    JINT16,     // 16位有符号整型
    JINT8,      // 8位有符号整型
    JUINT32,    // 32位无符号整型
    JUINT16,    // 16位无符号整型
    JUINT8,     // 8位无符号整型
    JFLOAT,     // 单精度浮点型
    JDOUBLE,    // 双精度浮点型
    JBOOL,      // 布尔型
    JSTRING,    // 字符串
    JOBJECT     // JSON对象
} JsonDataType;

/**
 * @brief 定义设备的某个属性, 实际就是一个JSON文档节点
 */
typedef struct _JSONNode {
    char   		 *key;    // 该JSON节点的Key
    void         *data;   // 该JSON节点的Value
    JsonDataType type;    // 该JSON节点的数据类型
} DeviceProperty;

/**
 * @brief 每次文档请求响应的回调函数
 *
 * @param pClient        ShadowClient对象
 * @param method         文档操作方式
 * @param requestAck     请求响应类型
 * @param pJsonDocument  云端响应返回的文档
 * @param userContext    用户数据
 *
 */
typedef void (*OnRequestCallback)(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *userContext);

/**
 * @brief 文档操作请求的参数结构体定义
 */
typedef struct _RequestParam {
    Method               	method;              	// 文档请求方式: GET, UPDATE, DELETE等

    List                    *property_delta_list;   // 该请求需要修改的属性

    uint32_t             	timeout_sec;         	// 请求超时时间, 单位:s

    OnRequestCallback    	request_callback;    	// 请求回调方法

    void                 	*user_context;          // 用户数据, 会通过回调方法OnRequestCallback返回

} RequestParams;

/**
 * @brief 设备属性处理回调函数
 *
 * @param pClient          ShadowClient对象
 * @param pParams          设备影子文档修改请求
 * @param pJsonValueBuffer 设备属性值
 * @param valueLength      设备属性值长度
 * @param DeviceProperty   设备属性结构体
 */
typedef void (*OnPropRegCallback)(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty);

/**
 * @brief 构造ShadowClient
 *
 * @param product_sn 产品序列号
 * @param device_sn 设备序列号
 * @param ch_signal 与MQTT服务器连接的句柄
 *
 * @return 返回NULL: 构造失败
 */
void* IOT_Shadow_Construct(const char *product_sn, const char *device_sn, void *ch_signal);

/**
 * @brief 销毁ShadowClient 关闭设备影子连接
 *
 * @param handle ShadowClient对象
 *
 * @return 返回SUCCESS, 表示成功
 */
int IOT_Shadow_Destroy(void *handle);

/**
 * @brief            消息接收, 心跳包管理, 超时请求处理
 *
 * @param handle     ShadowClient对象
 * @param timeout_ms 超时时间, 单位:ms
 * @return           返回SUCCESS, 表示调用成功
 */
int IOT_Shadow_Yield(void *handle, uint32_t timeout_ms);

/**
 * @brief 注册当前设备的设备属性
 *
 * @param pClient    ShadowClient对象
 * @param pProperty  设备属性
 * @param callback   设备属性更新回调处理函数
 * @return           返回SUCCESS, 表示请求成功
 */
int IOT_Shadow_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback);

/**
 * @brief 注销当前设备的设备属性
 *
 * @param pClient    ShadowClient对象
 * @param pProperty  设备属性
 * @return           SUCCESS 请求成功
 */
int IOT_Shadow_UnRegister_Property(void *handle, DeviceProperty *pProperty);

/**
 * @brief 获取设备影子文档并同步设备离线期间设备影子更新的属性值和版本号
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Get_Sync(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context); 

/**
 * @brief 设备更新设备影子的属性，变长入参的个数要和property_count的个数保持一致
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @param property_count    变长入参的个数      
 * @param ...               变长入参设备的属性
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Update(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...);

/**
 * @brief 更新属性并清零设备影子的版本号，变长入参的个数要和property_count的个数保持一致
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @param property_count    变长入参的个数      
 * @param ...               变长入参设备的属性
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Update_And_Reset_Version(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...); 

/**
 * @brief 设备删除设备影子的属性，变长入参的个数要和property_count的个数保持一致
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @param property_count    变长入参的个数      
 * @param ...               变长入参设备的属性
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Delete(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context, int property_count, ...); 

/**
 * @brief 设备删除全部设备影子的属性
 *
 * @param pClient           ShadowClient对象
 * @param request_callback  请求回调函数
 * @param timeout_sec       请求超时时间, 单位:s
 * @param user_context      请求回调函数的用户数据
 * @return                  SUCCESS 请求成功
 */
int IOT_Shadow_Delete_All(void *handle, OnRequestCallback request_callback, uint32_t timeout_sec, void *user_context);

/**
 * @brief 在请求中增加需要修改的属性
 *
 * @param pParams    设备影子文档修改请求
 * @param pProperty  设备属性
 * @return           返回SUCCESS, 表示请求成功
 */
int IOT_Shadow_Request_Add_Delta_Property(void *handle, RequestParams *pParams, DeviceProperty *pProperty);

/**
 * @brief 使用字符串的属性值直接更新属性值
 *
 * @param value         从云服务器设备影子文档Desired字段中解析出的字符串
 * @param pProperty     设备属性
 * @return              返回SUCCESS, 表示请求成功
 */
int IOT_Shadow_Direct_Update_Value(char *value, DeviceProperty *pProperty);

#ifdef __cplusplus
}
#endif

#endif /* C_SDK_UIOT_EXPORT_SHADOW_H_ */
