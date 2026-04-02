# arch/riscv/ 目录 - RISC-V 架构支持

**RISC-V 架构特定代码** — 支持 32/64-bit、多平台 SoC，含 Nuclei 处理器专项实现。

## 目录结构

```
arch/riscv/
├── cpu/                    # CPU 架构代码 (7个平台)
│   ├── start.S            # 统一入口点 (_start)
│   ├── u-boot.lds         # 主链接脚本
│   ├── u-boot-spl.lds     # SPL 链接脚本
│   ├── andesv5/           # Andes V5 核心
│   ├── fu540/             # SiFive FU540
│   ├── fu740/             # SiFive FU740
│   ├── generic/           # QEMU Virt
│   ├── jh7110/            # StarFive JH7110
│   └── nuclei/            # Nuclei 处理器 ⭐
├── lib/                    # 通用库
│   ├── spl.c              # SPL 入口
│   ├── sbi.c              # SBI 接口
│   ├── interrupts.c       # 中断处理
│   └── smp.c              # 多核支持
├── dts/                    # 设备树 (32个 .dts)
└── include/asm/            # 头文件
    ├── csr.h              # CSR 操作宏
    ├── encoding.h         # 模式前缀定义
    └── arch-nuclei/       # Nuclei 特定头文件
        └── csr.h          # Nuclei CSR 定义
```

## 查找指南

| 内容类型 | 位置 |
|---------|------|
| 统一入口点 | `cpu/start.S` (469行) |
| SPL 入口 | `lib/spl.c` |
| SBI 接口 | `lib/sbi.c` |
| CSR 操作宏 | `include/asm/csr.h` |
| 模式前缀宏 | `include/asm/encoding.h` |
| 链接脚本 | `cpu/u-boot.lds` / `cpu/u-boot-spl.lds` |
| 陷阱处理 | `cpu/mtrap.S` |

## 启动流程

```
_start (cpu/start.S)
    ├── 读取 CSR_MHARTID → tp (hart ID)
    ├── 保存 DTB 指针 → s1
    ├── 设置 trap_entry (MODE_PREFIX(tvec))
    ├── 屏蔽所有中断 (MODE_PREFIX(ie) = 0)
    ├── SMP: hart lottery 选择主核
    ├── 启用缓存 (icache_enable / dcache_enable)
    └── 跳转 board_init_f()
```

**模式前缀宏**: 自动适配 M-mode/S-mode
```c
#ifdef RISCV_SMODE
#define MODE_PREFIX(s) s##s    # stvec, sie (Supervisor)
#else
#define MODE_PREFIX(s) m##s    # mtvec, mie (Machine)
#endif
```

## Nuclei 处理器专项 ⭐

### Nuclei 文件位置
```
cpu/nuclei/
├── Kconfig          # NUCLEI_RISCV 配置
├── cpu.c            # cleanup_before_linux()
├── cache.c          # CCM 缓存控制
├── dram.c           # DRAM 初始化
├── spl.c            # spl_soc_init()
└── Makefile

include/asm/arch-nuclei/
└── csr.h            # Nuclei CSR 定义
```

### Nuclei 特有 CSR

| CSR | 地址 | 用途 |
|-----|------|------|
| `MCACHE_CTL` | 0x7ca | 缓存控制 |
| `MMISC_CTL` | 0x7d0 | 杂项控制 |
| `CCM_MBEGINADDR` | 0x7cb | M-mode CCM 起始地址 |
| `CCM_MCOMMAND` | 0x7cc | M-mode CCM 命令 |
| `CCM_SBEGINADDR` | 0x5cb | S-mode CCM 起始地址 |
| `CCM_SCOMMAND` | 0x5cc | S-mode CCM 命令 |
| `CCM_SUEN` | 0x7ce | S-mode CCM 启用 |

### CCM 缓存命令 (cache.c)

```c
CCM_DC_INVAL       = 0x0    // D-Cache 行失效
CCM_DC_WB          = 0x1    // D-Cache 行写回
CCM_DC_WBINVAL     = 0x2    // 写回+失效
CCM_DC_WBINVAL_ALL = 0x6    // 全部 D-Cache 写回+失效
CCM_IC_INVAL_ALL   = 0xd    // 全部 I-Cache 失效
```

### Nuclei 启动特点

1. **SPL 初始化** (`board/nuclei/generic/spl.c`):
```c
int spl_board_init_f(void) {
    // 启用 S-mode CCM 操作
    csr_write(CCM_SUEN, 0x2020202);
    return 0;
}
```

2. **缓存启用** (`cpu/nuclei/cache.c`):
```c
void icache_enable(void) {
    csr_set(MCACHE_CTL, ICACHE_EN | ICACHE_PF_EN | ICACHE_CANCLE_EN);
}

void dcache_enable(void) {
    csr_set(MCACHE_CTL, DCACHE_EN);
}
```

3. **Kconfig 依赖**:
```
NUCLEI_RISCV:
    select ARCH_EARLY_INIT_R
    select SYS_CACHE_SHIFT_6      # 64B cache line
    imply RISCV_ACLINT            # ACLINT 中断
    imply SPL_OPENSBI             # OpenSBI 支持
    imply SPL_LOAD_FIT            # FIT 镜像
```

## 关键配置

### RISC-V 基础
```bash
CONFIG_RISCV=y
CONFIG_RISCV_SMODE=y          # 或 RISCV_MMODE
CONFIG_ARCH_RV64I=y           # 或 RV32I
CONFIG_CMODEL_MEDLOW=y        # 代码模型
```

### Nuclei 专用
```bash
CONFIG_NUCLEI_RISCV=y
CONFIG_TARGET_NUCLEI_GENERIC_SOC=y
CONFIG_SYS_CACHE_SHIFT_6=y    # 64B cache line
```

### SPL 配置
```bash
CONFIG_SPL=y
CONFIG_SPL_RISCV_MMODE=y      # SPL 运行模式
CONFIG_SPL_OPENSBI=y          # OpenSBI 支持
CONFIG_SPL_LOAD_FIT=y         # FIT 镜像
```

## 支持平台 (Kconfig)

- **TARGET_AE350** - Andes Technology AE350
- **TARGET_NUCLEI_GENERIC_SOC** - Nuclei 通用 SoC
- **TARGET_MICROCHIP_ICICLE** - Microchip PolarFire-SoC
- **TARGET_QEMU_VIRT** - QEMU Virt 虚拟机
- **TARGET_SIFIVE_UNLEASHED** - SiFive HiFive Unleashed
- **TARGET_SIFIVE_UNMATCHED** - SiFive HiFive Unmatched
- **TARGET_STARFIVE_VISIONFIVE2** - StarFive VisionFive 2
- **TARGET_TH1520_LPI4A** - Sipeed TH1520 Lichee PI 4A
- **TARGET_SIPEED_MAIX** - Sipeed Maix (K210)
- **TARGET_OPENPITON_RISCV64** - OpenPiton RISC-V

## 注意事项

1. **统一入口**: 所有 RISC-V 平台共用 `cpu/start.S`，通过 `MODE_PREFIX()` 适配 M/S-mode
2. **Nuclei CCM**: 缓存操作通过专用 CSR 命令序列，非标准 RISC-V 指令
3. **启动流程**: SPL → OpenSBI → U-Boot Proper，DDR 初始化在 SPL 阶段完成
4. **多核支持**: hart lottery 机制选择主核，其他核等待 `gd` 初始化完成
5. **缓存行大小**: Nuclei 使用 64B 缓存行 (`SYS_CACHE_SHIFT_6`)

## SPL 业务逻辑详解 (Nuclei 实现) ⭐⭐⭐

### 完整 SPL 启动流程

```
_start (cpu/start.S)
    └── 判断 CONFIG_SPL_BUILD → 跳转到 board_init_f
        
board_init_f (lib/spl.c)
    ├── spl_early_init()          # SPL 框架初始化
    ├── riscv_cpu_setup()         # CPU 基础设置
    ├── preloader_console_init()  # 串口控制台
    └── spl_board_init_f()        # 板级初始化
        
spl_soc_init (cpu/nuclei/spl.c)   # ⭐ SoC 层
    └── uclass_get_device(UCLASS_RAM)  # DDR 初始化
        
spl_board_init_f (board/nuclei/generic/spl.c)  # ⭐ 板级层
    └── csr_write(CCM_SUEN, CCM_SEN)   # 启用 S-mode CCM
        
spl_board_init (board/nuclei/generic/spl.c)
    └── 板级后续初始化 (日志等)
        
spl_load_image
    └── 解析 FIT → image_uncipher() 解密 → jump_to_image_no_args()
        └── 跳转到 OpenSBI 或 U-Boot Proper
```

### SPL 文件清单

```
arch/riscv/
├── lib/spl.c              # 通用 SPL 入口 (board_init_f)
├── cpu/u-boot-spl.lds     # SPL 链接脚本
└── cpu/nuclei/
    ├── spl.c              # SoC 初始化 (spl_soc_init)
    ├── Kconfig            # SPL 配置
    └── Makefile           # SPL 构建规则

board/nuclei/generic/
└── spl.c                  # 板级 SPL 业务逻辑
```

### 核心函数业务逻辑

#### 1. 板级初始化 `spl_board_init_f()`

**文件**: `board/nuclei/generic/spl.c:15-25`

```c
int spl_board_init_f(void)
{
    // 注释: 这里可以初始化 DDR (如果需要)
    
    // Nuclei 特有: 启用 S-mode CCM 缓存操作
    #define CCM_SUEN    0x7CE       // CSR: CCM S-mode 启用
    #define CCM_SEN     0x2020202   // 魔法值
    
    csr_write(CCM_SUEN, CCM_SEN);   // 写 CSR 启用 CCM
    return 0;
}
```

**业务逻辑**:
- 启用 S-mode CCM (Cache Control and Management)
- 必须在 DDR 初始化之后执行
- 通过自定义 CSR `0x7CE` 启用

#### 2. SoC 初始化 `spl_soc_init()`

**文件**: `arch/riscv/cpu/nuclei/spl.c:7-20`

```c
int spl_soc_init(void)
{
    int ret;
    struct udevice *dev;
    
    /* DDR 初始化 - 通过 Driver Model */
    ret = uclass_get_device(UCLASS_RAM, 0, &dev);
    if (ret) {
        debug("DRAM init failed: %d\n", ret);
        return ret;
    }
    return 0;
}
```

**业务逻辑**:
- 使用 UCLASS_RAM 类初始化 DDR
- 符合 U-Boot Driver Model 框架
- 失败时返回错误码

#### 3. 启动设备选择 `spl_boot_device()`

**文件**: `board/nuclei/generic/spl.c:36-39`

```c
u32 spl_boot_device(void)
{
    return BOOT_DEVICE_RAM;  // 从 RAM 启动
}
```

**可选值**:
- `BOOT_DEVICE_RAM` - RAM (当前)
- `BOOT_DEVICE_MMC` - SD/eMMC
- `BOOT_DEVICE_SPI` - SPI Flash
- `BOOT_DEVICE_NAND` - NAND

#### 4. 设备树设置 `board_fdt_blob_setup()`

**文件**: `board/nuclei/generic/spl.c:41-47`

```c
void *board_fdt_blob_setup(int *err)
{
    void *fdt_blob = (ulong *)&_end;  // DTB 在 BSS 结束处
    *err = 0;
    return fdt_blob;
}
```

**业务逻辑**:
- 设备树放在 `_end` 符号处 (BSS 结束)
- 通常由 OpenSBI 或仿真器提供

#### 5. FIT 镜像解密 `image_uncipher()`

**文件**: `board/nuclei/generic/spl.c:74-106`

```c
static int image_uncipher(const void *fit, int image_noffset,
                         void **data, size_t *size)
{
    // 1. 查找 cipher 子节点
    cipher_noffset = fdt_subnode_offset(fit, image_noffset, 
                                        FIT_CIPHER_NODENAME);
    if (cipher_noffset < 0)
        return 0;  // 未加密
    
    // 2. 解密数据
    ret = fit_image_decrypt_data(fit, image_noffset, cipher_noffset,
                                 *data, *size, &dst, &size_dst);
    
    // 3. 返回解密后的数据
    *data = dst;
    *size = size_dst;
    return 0;
}

// FIT 镜像加载后回调
void board_fit_image_post_process(const void *fit, int node, 
                                  void **p_image, size_t *p_size)
{
    image_uncipher(fit, node, p_image, p_size);
}
```

**业务逻辑**:
- 支持 FIT 镜像自动解密 (安全启动)
- 自动检测是否包含 cipher 节点
- 透明解密，对上层逻辑无感知

### SPL 函数汇总表

| 函数 | 文件位置 | 业务逻辑 | 重要性 |
|------|---------|---------|--------|
| `spl_board_init_f()` | `board/spl.c` | 启用 S-mode CCM | ⭐⭐⭐ 必须 |
| `spl_soc_init()` | `cpu/nuclei/spl.c` | DDR 初始化 (DM) | ⭐⭐⭐ 必须 |
| `spl_boot_device()` | `board/spl.c` | 选择启动设备 | ⭐⭐ 配置 |
| `board_fdt_blob_setup()` | `board/spl.c` | 设置 DTB 地址 | ⭐⭐ 必须 |
| `spl_board_init()` | `board/spl.c` | 板级后续初始化 | ⭐ 可选 |
| `harts_early_init()` | `cpu/nuclei/spl.c` | 多核早期初始化 | ⭐ 预留 |
| `image_uncipher()` | `board/spl.c` | FIT 镜像解密 | ⭐⭐ 安全启动 |

### 关键注意事项 ⚠️

1. **不要启用 `CONFIG_SPL_FRAMEWORK_BOARD_INIT_F`**
   ```c
   // spl.c 注释明确警告:
   // you should never select CONFIG_SPL_FRAMEWORK_BOARD_INIT_F in kconfig
   ```
   Nuclei 使用弱符号覆盖，而非框架回调。

2. **CCM 启用顺序**
   ```
   正确顺序: spl_soc_init() [DDR] → spl_board_init_f() [CCM]
   ```
   CCM 必须在 DDR 初始化之后启用。

3. **启动设备可配置**
   ```c
   // 当前: BOOT_DEVICE_RAM
   // 可改为: BOOT_DEVICE_MMC, BOOT_DEVICE_SPI, BOOT_DEVICE_NAND
   ```

### SPL 配置模板

```bash
# 启用 SPL
CONFIG_SPL=y
CONFIG_SPL_RISCV_MMODE=y
CONFIG_SPL_FRAMEWORK=y

# Nuclei 特有
CONFIG_SPL_OPENSBI=y
CONFIG_SPL_LOAD_FIT=y
CONFIG_SPL_RAM=y

# 可选: 安全启动
CONFIG_SPL_FIT_CIPHER=y

# 可选: 显示信息
CONFIG_SPL_DISPLAY_PRINT=y
```

### 典型启动序列

```bash
# 编译 SPL
make nuclei_generic_defconfig
make -j$(nproc)

# SPL 执行流程:
_start → board_init_f → spl_soc_init (DDR) 
     → spl_board_init_f (CCM) → spl_load_image
     → image_uncipher (解密) → jump_to_image_no_args
     → OpenSBI → U-Boot Proper
```

## SPL 与 U-Boot Proper 双镜像机制 ⭐⭐⭐

### 是的，编译生成两个独立镜像

```
arch/riscv/cpu/
├── u-boot-spl.lds          # SPL 链接脚本 (小镜像)
└── u-boot.lds              # U-Boot Proper 链接脚本 (大镜像)
```

### 链接脚本对比

#### SPL 链接脚本 (`u-boot-spl.lds`)

```lds
MEMORY { 
    .spl_mem : ORIGIN = IMAGE_TEXT_BASE,      /* 固定加载地址 */
               LENGTH = IMAGE_MAX_SIZE          /* 128KB-256KB */
    .bss_mem : ORIGIN = CONFIG_SPL_BSS_START_ADDR,
               LENGTH = CONFIG_SPL_BSS_MAX_SIZE
}

ENTRY(_start)

SECTIONS {
    .text : {
        arch/riscv/cpu/start.o (.text)
        *(.text*)
    } > .spl_mem
    
    /* 简化段: 只有 .text .rodata .data .bss */
    /* 无重定位段 - 不需要运行时重定位 */
}
```

**特点**: 体积小 (64KB-256KB)、独立内存区、简化段结构

#### U-Boot Proper 链接脚本 (`u-boot.lds`)

```lds
SECTIONS {
    .text : { arch/riscv/cpu/start.o (.text) }
    .efi_runtime : { ... }                      /* EFI 支持 */
    .got : { ... }                              /* 全局偏移表 */
    .rela.dyn : { ... }                         /* 动态重定位 */
    __u_boot_list : { ... }                     /* 驱动模型 */
}
```

**特点**: 体积大 (512KB-2MB)、完整功能、支持 PIE 重定位

### 编译构建

```bash
# 构建生成两个镜像
make nuclei_generic_defconfig
make -j$(nproc)

# 输出文件
spl/u-boot-spl.bin              # ⭐ SPL 镜像 (小)
u-boot.bin                      # ⭐ U-Boot Proper 镜像 (大)
u-boot.itb                      # FIT 组合镜像 (可选)
```

**条件编译** (`arch/riscv/cpu/nuclei/Makefile`):
```makefile
ifeq ($(CONFIG_SPL_BUILD),y)
    obj-y += spl.o              # SPL 专用代码
else
    obj-y += cpu.o dram.o       # U-Boot Proper 代码
endif
obj-y += cache.o                # 两者共有
```

### 加载时机与启动流程

```
┌──────────────────────────────────────────────────────────────┐
│ Stage 1: Boot ROM / CPU 内部                                  │
│ ├── 上电复位                                                  │
│ ├── 从固定地址加载 SPL (Flash/SPI)                           │
│ └── 跳转到 SPL _start                                         │
└──────────────────────────┬───────────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────────┐
│ Stage 2: SPL 执行 (u-boot-spl.bin)                           │
│ ├── _start → board_init_f()                                  │
│ ├── spl_soc_init() → DDR 初始化                              │
│ ├── spl_board_init_f() → CCM 启用                            │
│ ├── spl_load_image() → 加载 U-Boot Proper                   │
│ │       ├── 从 SPI/MMC/NAND 读取                             │
│ │       ├── image_uncipher() 解密 (可选)                     │
│ │       └── 加载到内存 (如 0x80200000)                       │
│ └── jump_to_image_no_args() → 跳转                           │
└──────────────────────────┬───────────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────────┐
│ Stage 3: U-Boot Proper (u-boot.bin)                          │
│ ├── _start → 完整初始化                                      │
│ ├── relocate_code() → 重定位到 RAM 顶端                      │
│ ├── board_init_r() → 运行时初始化                            │
│ └── main_loop() → 命令行                                     │
└──────────────────────────────────────────────────────────────┘
```

### Nuclei 内存布局示例

```c
// 典型 128MB DDR 布局

0x8000_0000  ┌──────────────────────┐  ← SPL 加载地址
             │   u-boot-spl.bin     │     (64KB-256KB)
             │   (IMAGE_TEXT_BASE)  │
0x8004_0000  ├──────────────────────┤
             │        ...           │
0x8020_0000  ┌──────────────────────┐  ← U-Boot 加载地址
             │   u-boot.bin/.itb    │     (SPL 决定)
             │   (FIT 镜像)         │     (512KB-1MB)
0x8080_0000  ├──────────────────────┤
             │        ...           │
0x8F00_0000  ┌──────────────────────┐  ← U-Boot 运行地址
             │   重定位后运行       │     (RAM 顶端)
             │   (gd->relocaddr)    │
             └──────────────────────┘
```

### 双镜像关键区别

| 特性 | SPL | U-Boot Proper |
|------|-----|---------------|
| **体积** | 64KB-256KB | 512KB-2MB |
| **功能** | DDR 初始化 + 加载 | 完整 Bootloader |
| **驱动** | 仅基本 (RAM/UART) | 完整驱动模型 |
| **命令** | 无 | 完整命令集 |
| **网络** | 通常无 | 有 |
| **重定位** | 无 (XIP/静态) | 有 (PIE) |
| **链接脚本** | `u-boot-spl.lds` | `u-boot.lds` |
| **构建标志** | `CONFIG_SPL_BUILD=y` | 正常 |

### 镜像组合方式

#### 方式 1: 独立镜像 (常见)

```bash
# 烧录到不同位置
spl/u-boot-spl.bin    → Flash 偏移 0x00000 (0x80000000)
u-boot.bin            → Flash 偏移 0x20000 (0x80200000)

# Boot ROM → SPL → U-Boot
```

#### 方式 2: FIT 组合镜像 (推荐)

```bash
u-boot.itb
├── u-boot-spl.bin      # SPL 段
├── u-boot.bin          # U-Boot Proper 段
└── opensbi.bin         # OpenSBI (RISC-V)

# SPL → 解析 FIT → 加载 u-boot.bin → 跳转
```

### Nuclei 特殊注意事项 ⚠️

1. **CCM 必须在 SPL 启用**
   ```c
   // spl_board_init_f() 中
   csr_write(CCM_SUEN, CCM_SEN);  // 必须！
   ```

2. **U-Boot 重新初始化缓存**
   ```c
   // U-Boot Proper 的 start.S
   icache_enable();   // 重新启用
   dcache_enable();
   ```

3. **设备树传递链**
   ```c
   // Boot ROM → SPL (a1 寄存器)
   // SPL → U-Boot Proper (a1 寄存器)
   // 通过 board_fdt_blob_setup() 传递
   ```

4. **加载地址可配置**
   ```c
   // board/nuclei/generic/spl.c
   u32 spl_boot_device(void) {
       return BOOT_DEVICE_RAM;  // 可改 MMC/SPI
   }
   ```

### 验证构建输出

```bash
# 检查镜像大小
$ ls -lh spl/u-boot-spl.bin u-boot.bin
-rw-r--r-- 68K spl/u-boot-spl.bin    # 小镜像
-rw-r--r-- 512K u-boot.bin           # 大镜像

# 查看入口点
$ riscv64-unknown-elf-readelf -h spl/u-boot-spl.bin | grep Entry
Entry point address: 0x80000000
```

**核心要点**: SPL "铺路" (DDR+CCM), U-Boot "开车" (完整功能)。两个独立镜像，分阶段加载。
