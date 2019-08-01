# UIoT-Core设备端C-SDK
UIoT-Core设备端C-SDK主要用于提供设备调用的API，实现设备与UIoT平台通信以及一些其它的辅助功能。

C语言版SDK可在不同的操作系统上运行，如Linux、FreeRTOS等。SDK需要OS或硬件支持的操作被定义为一系列HAL函数。目前，C-SDK已经实现了linux环境的HAL，其他嵌入式RTOS及特定硬件平台需用户自行适配。

## SDK架构图
![](https://uiot.cn-sh2.ufileos.com/sdk%E6%9E%B6%E6%9E%84%E5%9B%BE.png)
## 目录结构
* `external_libs` - 第三方软件包组件，包括mbedTLS库、gtest库等
* `platform` - 平台相关代码
* `samples` - 应用示例
* `src` - SDK核心通信协议及功能模块
* `tests` - 测试用例
* `tools` - 配置及脚本文件

## 编译支持
* `GNU Make`
* `CMake`

## 文档
<https://github.com/UCloudDocs/uiot-core>
