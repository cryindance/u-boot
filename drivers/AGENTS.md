# drivers/ 目录

**用途:** 设备驱动子系统，包含 74 个类别，约 2,589 个 C/H 文件。

## 结构

| 类别 | 目录 | 说明 |
|------|------|------|
| 存储 | `block/`, `mmc/`, `mtd/`, `nvme/`, `scsi/` | 块设备、闪存、NVMe |
| 网络 | `net/`, `usb/gadget/` | 网卡、USB 网络 gadget |
| 总线 | `i2c/`, `spi/`, `pci/`, `bus/` | I2C/SPI/PCI 控制器 |
| 外设 | `gpio/`, `video/`, `serial/`, `input/` | GPIO、显示、串口、输入 |
| 系统 | `clk/`, `reset/`, `power/`, `thermal/` | 时钟、复位、电源、温度 |
| 核心 | `core/` | Driver Model 核心实现 |

## 查找指南

| 内容 | 位置 |
|------|------|
| 驱动模型定义 | `include/dm/` |
| UCLASS 定义 | `include/dm/uclass-id.h` |
| 新驱动模板 | `drivers/core/` |
| 设备树绑定 | `include/dt-bindings/` |
| Kconfig 定义 | `drivers/Kconfig` |

## 约定

- **新驱动必须使用 Driver Model (DM)** 框架
- 驱动注册使用 `U_BOOT_DRIVER()` 宏
- 设备树兼容使用 `of_match_ptr()`

## 反模式

| 模式 | 说明 |
|------|------|
| 旧非 DM 驱动 | 标记 DEPRECATED，不要参考 |
| `lpc32xx_gpio.c` | 旧代码风格，不要作为示例 |
| 直接操作寄存器 | 应使用 DM 提供的 API |
