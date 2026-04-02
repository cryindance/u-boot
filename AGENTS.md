# U-Boot 项目知识库

**版本:** 2023.10  
**项目类型:** 嵌入式 Bootloader  
**主要语言:** C (20,157+ 文件)

## 项目概述

U-Boot（Universal Boot Loader）是嵌入式系统最常用的开源引导加载程序，支持 ARM、RISC-V、x86、MIPS、PowerPC 等 13+ 架构，为 180+ 板卡提供支持。

## 目录结构

```
.
├── arch/           # 架构特定代码 (13 个架构)
├── board/          # 板级支持 (182 个板卡)
├── cmd/            # Shell 命令实现
├── common/         # 通用代码 (main.c 入口)
├── configs/        # 板级配置文件 (1,310 个)
├── drivers/        # 设备驱动 (74 个子系统)
├── dts/            # 设备树源文件
├── env/            # 环境变量处理
├── fs/             # 文件系统支持
├── include/        # 头文件
├── lib/            # 库代码
├── net/            # 网络协议栈
├── scripts/        # 构建脚本
├── test/           # 测试框架
└── tools/          # 构建工具 (Python)
```

## 关键入口点

| 类型 | 位置 | 说明 |
|------|------|------|
| 构建入口 | `./Makefile` | GNU Make + Kconfig |
| C 运行时入口 | `common/main.c` | `main_loop()` 主循环 |
| 硬件复位入口 | `arch/*/cpu/start.S` | 架构特定的汇编入口 |
| 链接脚本 | `arch/*/cpu/u-boot.lds` | 定义 ENTRY(_start) |

## 构建命令

```bash
# 配置特定板卡
make <board>_defconfig

# 并行构建
make -j$(nproc)

# 详细输出
make V=1

# 运行测试
make check           # 运行所有测试
make pcheck          # 并行测试
make qcheck          # 快速测试

# 仅构建工具
make tools

# 构建 SPL（二级引导程序）
make SPL
```

## 代码约定

### 风格规范
- **行尾:** LF（非 CRLF）— `.gitattributes`
- **代码检查:** Linux kernel checkpatch.pl + U-Boot 特定规则 — `.checkpatch.conf`
- **Python 命名:** snake_case — `scripts/style.py`

### 关键配置
- **优化级别:** 默认 `-Os`（大小优化）
- **编译器:** GCC 或 Clang 支持
- **配置系统:** Kconfig（Linux 内核风格）

## 反模式（禁止事项）

| 模式 | 位置 | 说明 |
|------|------|------|
| 字体安全 | `drivers/video/stb_truetype.h` | **DO NOT** 用于不受信任的字体文件 |
| 旧代码示例 | `drivers/gpio/lpc32xx_gpio.c` | **DO NOT** 作为新代码示例 |
| USB ID 复用 | `drivers/usb/gadget/ether.c` | **DO NOT** 用不兼容驱动复用 ID |
| 重映射时机 | `board/armltd/integrator/` | **DO NOT** 在代码重定位前重映射 |
| 时钟门控 | `arch/arm/mach-socfpga/` | **DO NOT** 关闭调试/桥接时钟 |

## 项目特定模式

### 三阶段启动
1. **TPL** - 三级程序加载器（可选验证启动）
2. **SPL** - 二级程序加载器（早期 DRAM 初始化）
3. **U-Boot** - 主引导程序（完整功能）

### 驱动模型（Driver Model）
- 新驱动必须使用 DM 框架
- 旧非 DM 代码标记为 DEPRECATED

### 多架构支持
- 每个架构在 `arch/<arch>/` 下有独立代码
- 共用代码通过宏和条件编译处理差异

## 测试框架

### C 单元测试
- 框架: 自定义 `test/test.h`
- 注册: `UNIT_TEST(name, flags, suite)` 宏
- 命名: `<suite>_test_<name>`
- 运行: `ut <suite> <test>`（U-Boot shell）

### Python 测试
- 框架: pytest
- 位置: `test/py/tests/`
- 标记: `@pytest.mark.boardspec`, `@pytest.mark.buildconfigspec`
- 运行: `./test/py/test.py --bd sandbox`

## 自定义工具

| 工具 | 用途 | 位置 |
|------|------|------|
| buildman | 多板并行构建 | `tools/buildman/` |
| binman | 二进制镜像创建 | `tools/binman/` |
| dtoc | 设备树转 C 代码 | `tools/dtoc/` |
| patman | 补丁管理 | `tools/patman/` |

## CI/CD

- **GitLab CI:** `.gitlab-ci.yml`（主要 CI）
- **Azure Pipelines:** `.azure-pipelines.yml`
- **Docker 镜像:** `trini/u-boot-gitlab-ci-runner`

## 注意事项

1. **板卡配置:** 1,310 个 defconfig 文件在 `configs/` 目录
2. **设备树:** ARM 架构重度依赖 DTB，源文件在 `arch/arm/dts/`
3. **文档:** Sphinx 构建，`make htmldocs`
4. **代码审查:** 提交前运行 `scripts/checkpatch.pl`
