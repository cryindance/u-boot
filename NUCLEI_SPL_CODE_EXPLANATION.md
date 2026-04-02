# U-Boot SPL 代码详解

## 概述

SPL（Secondary Program Loader）是 U-Boot 的"轻量级先锋队"。由于嵌入式系统上电后可用内存有限，Boot ROM 只能加载一个很小的镜像（通常几十KB），这个镜像就是 SPL。

**SPL 的核心使命：**
1. 初始化 DDR 内存 — 让系统有足够 RAM
2. 启用缓存 — 提升性能
3. 加载主 U-Boot — 从 Flash/SD 卡加载完整的 U-Boot
4. 跳转到下一阶段 — 将控制权交给主 U-Boot

---

## 一、启动入口：`_start` (arch/riscv/cpu/start.S)

这是 CPU 上电后执行的第一段代码。

```asm
.globl _start
_start:
#if CONFIG_IS_ENABLED(RISCV_MMODE)
	csrr	a0, CSR_MHARTID
#endif

	/* 保存 hart id 和 dtb 指针 */
	mv	tp, a0          // tp = hart ID (线程指针，C代码不会修改)
	mv	s1, a1          // s1 = DTB 指针 (设备树)

	/* 初始化全局数据指针为0，防止早期 trap 出错 */
	mv	gp, zero

	/* 设置异常处理入口 */
	la	t0, trap_entry
csrw	MODE_PREFIX(tvec), t0

	/* 屏蔽所有中断 */
	csrw	MODE_PREFIX(ie), zero
```

**关键操作：**
1. **读取 Hart ID** - 获取当前 CPU 核心编号
2. **保存 DTB 指针** - 设备树由 Boot ROM/OpenSBI 传入，保存在 s1 寄存器
3. **设置异常入口** - 配置 trap_entry，处理早期异常
4. **屏蔽中断** - 全局禁用中断，避免干扰初始化

---

## 二、C 语言入口：`board_init_f` (arch/riscv/lib/spl.c)

汇编代码设置好基本环境后，跳转到 C 语言代码。

```c
__weak void board_init_f(ulong dummy)
{
	int ret;

	/* 1. SPL 框架初始化 */
	ret = spl_early_init();
	if (ret)
		panic("spl_early_init() failed: %d\n", ret);

	/* 2. CPU 基础设置 */
	riscv_cpu_setup(NULL, NULL);

	/* 3. 串口控制台初始化 - 终于可以打印日志了 */
	preloader_console_init();

	/* 4. 板级初始化 - 关键！ */
	ret = spl_board_init_f();
	if (ret)
		panic("spl_board_init_f() failed: %d\n", ret);
}
```

**执行流程：**
1. **spl_early_init()** - 初始化 SPL 框架、全局数据 gd
2. **riscv_cpu_setup()** - 设置 CPU 特性（如定时器）
3. **preloader_console_init()** - 初始化串口，启用打印功能
4. **spl_board_init_f()** - 板级初始化（DDR、CCM 等关键操作）

**注意 `__weak` 关键字：**
- 这是一个"弱符号"，可以被板级代码覆盖
- Nuclei 平台在 `board/nuclei/generic/spl.c` 中提供了自定义实现

---

## 三、SoC 层初始化：`spl_soc_init` (arch/riscv/cpu/nuclei/spl.c)

这是 Nuclei 平台 SoC 层的 SPL 初始化代码。

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

**业务逻辑：**
1. **使用 Driver Model (DM)** - 调用 `uclass_get_device(UCLASS_RAM)`
2. **UCLASS_RAM** - U-Boot 的 RAM 设备类，统一管理 DDR 驱动
3. **DDR 初始化** - 配置 DDR 控制器、时序、训练等

**关键点：**
- 这是 SPL **最重要的工作** — 让系统有可用内存
- 如果 DDR 初始化失败，系统无法继续启动
- 使用 DM 框架，符合 U-Boot 驱动模型

---

## 四、板级初始化：`spl_board_init_f` (board/nuclei/generic/spl.c)

这是 Nuclei 平台板级 SPL 业务逻辑。

```c
// This is called in board_init_f of arch/riscv/lib/spl.c
int spl_board_init_f(void)
{
    // TODO you can init your DDR memory here
	#define CCM_SUEN	0x7CE
	#define CCM_SEN		0x2020202

	/* enable ccm ops for smode */
	csr_write(CCM_SUEN, CCM_SEN);

	return 0;
}
```

**业务逻辑：**
1. **CCM_SUEN (0x7CE)** - Nuclei 特有的 CSR，控制 S-mode CCM 启用
2. **CCM_SEN (0x2020202)** - 魔法值，启用 S-mode 缓存操作权限
3. **执行时机** - 必须在 DDR 初始化之后

**为什么这么重要？**

Nuclei 处理器的 CCM (Cache Control and Management) 机制：
- M-mode (Machine) 默认可以操作缓存
- S-mode (Supervisor) **默认不能** 操作缓存
- 必须通过 `CCM_SUEN` CSR 显式启用

**启动流程顺序：**
```
spl_soc_init() [DDR 初始化] → spl_board_init_f() [CCM 启用]
```

**注意警告：**
```c
// board_init_f weak version is defined arch/riscv/lib/spl.c
// you should never select CONFIG_SPL_FRAMEWORK_BOARD_INIT_F in kconfig
```
- Nuclei 使用弱符号覆盖机制
- 不要启用 `CONFIG_SPL_FRAMEWORK_BOARD_INIT_F`

---

## 五、缓存管理：`cache.c` (arch/riscv/cpu/nuclei/cache.c)

Nuclei 使用自定义 CSR 控制缓存，非标准 RISC-V 指令。

### 5.1 CCM 命令枚举

```c
typedef enum CCM_CMD {
    CCM_DC_INVAL = 0x0,           // D-Cache 行失效
    CCM_DC_WB = 0x1,              // D-Cache 行写回
    CCM_DC_WBINVAL = 0x2,         // 写回+失效
    CCM_DC_WBINVAL_ALL = 0x6,     // 全部 D-Cache 写回+失效
    CCM_IC_INVAL_ALL = 0xd        // 全部 I-Cache 失效
} CCM_CMD_Type;
```

### 5.2 启用缓存

```c
void icache_enable(void)
{
#if CONFIG_IS_ENABLED(RISCV_MMODE)
	csr_set(CSR_MCACHE_CTL, CSR_MCACHE_ICACHE_EN 
			| CSR_MCACHE_ICACHE_PF_EN | CSR_MCACHE_ICACHE_CANCLE_EN);
#endif
}

void dcache_enable(void)
{
#if CONFIG_IS_ENABLED(RISCV_MMODE)
	csr_set(CSR_MCACHE_CTL, CSR_MCACHE_DCACHE_EN);
#endif
}
```

**关键 CSR：**
- `CSR_MCACHE_CTL` (0x7ca) - 缓存控制寄存器
- 在 M-mode 下启用 I/D Cache
- 预取 (`PF_EN`) 和取消 (`CANCLE_EN`) 功能

### 5.3 缓存操作

```c
static void ccm_cache_ops(size_t start_addr, size_t end_addr, CCM_CMD_Type type)
{
	size_t cache_aligned;

#if CONFIG_IS_ENABLED(RISCV_MMODE)
	cache_aligned = start_addr & CACHE_LINE_MASK;     // 64B 对齐
	csr_write(CSR_CCM_MBEGINADDR, cache_aligned);     // 设置起始地址
	for (; cache_aligned < end_addr; cache_aligned += CACHE_LINE_SIZE) {
		csr_write(CSR_CCM_MCOMMAND, type);             // 执行命令
	}
#elif CONFIG_IS_ENABLED(RISCV_SMODE)
	// S-mode 使用 CCM_SBEGINADDR / CCM_SCOMMAND
	cache_aligned = start_addr & CACHE_LINE_MASK;
	csr_write(CSR_CCM_SBEGINADDR, cache_aligned);
	for (; cache_aligned < end_addr; cache_aligned += CACHE_LINE_SIZE) {
		csr_write(CSR_CCM_SCOMMAND, type);
	}
#endif
}
```

**操作步骤：**
1. 地址按缓存行大小（64B）对齐
2. 写入 `CCM_MBEGINADDR` / `CCM_SBEGINADDR`
3. 写入命令到 `CCM_MCOMMAND` / `CCM_SCOMMAND`
4. 硬件自动执行缓存操作

---

## 六、FIT 镜像解密：`image_uncipher` (board/nuclei/generic/spl.c)

支持 FIT 镜像的安全启动（加密）。

```c
static int image_uncipher(const void *fit, int image_noffset,
                             void **data, size_t *size)
{
	int cipher_noffset, ret;
	void *dst;
	size_t size_dst;

	/* 1. 查找 cipher 子节点 */
	cipher_noffset = fdt_subnode_offset(fit, image_noffset,
									   FIT_CIPHER_NODENAME);
	if (cipher_noffset < 0)
		return 0;  // 未加密，直接返回

	/* 2. 解密数据 */
	log_info("decrypt %s...", fit_get_name(fit, image_noffset, NULL));
	ret = fit_image_decrypt_data(fit, image_noffset, cipher_noffset,
								 *data, *size, &dst, &size_dst);
	if (ret) {
		log_info("Failed,err:0x%x\n", ret);
		goto out;
	}

	/* 3. 返回解密后的数据 */
	*data = dst;
	*size = size_dst;
	log_info("OK\n");
out:
	return ret;
}

/* FIT 镜像加载后回调 */
void board_fit_image_post_process(const void *fit, int node, void **p_image,
								 size_t *p_size)
{
	image_uncipher(fit, node, p_image, p_size);
}
```

**业务逻辑：**
1. **检测加密** - 查找 FIT 镜像中的 `cipher` 节点
2. **自动解密** - 调用 `fit_image_decrypt_data()` 解密
3. **透明处理** - 解密后替换原数据，对上层无感知
4. **回调机制** - `board_fit_image_post_process` 是 U-Boot 定义的回调

**安全启动流程：**
```
FIT 镜像 (加密存储)
    ↓
spl_load_image() 加载
    ↓
board_fit_image_post_process() 回调
    ↓
image_uncipher() 解密
    ↓
跳转到解密后的镜像
```

---

## 七、跳转到下一阶段：`jump_to_image_no_args`

SPL 完成使命后，跳转到下一阶段（通常是 OpenSBI 或 U-Boot Proper）。

```c
void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_riscv_t)(ulong hart, void *dtb);
	void *fdt_blob;
	__maybe_unused int ret;

	/* 获取设备树地址 */
#if CONFIG_IS_ENABLED(LOAD_FIT) || CONFIG_IS_ENABLED(LOAD_FIT_FULL)
	fdt_blob = spl_image->fdt_addr;           // FIT 镜像中
#else
	fdt_blob = (void *)gd->fdt_blob;          // 全局数据
#endif

	/* 设置入口函数指针 */
	image_entry_riscv_t image_entry =
		(image_entry_riscv_t)spl_image->entry_point;

	/* 刷新指令缓存，确保跳转正确 */
	invalidate_icache_all();

	debug("image entry point: 0x%lX\n", spl_image->entry_point);

#ifdef CONFIG_SPL_SMP
	/* SMP: 通知其他核一起跳转 */
	ret = smp_call_function(spl_image->entry_point, (ulong)fdt_blob, 0, 0);
	if (ret)
		hang();
#endif

	/* 跳转！传递 hart ID 和 DTB 指针 */
	image_entry(gd->arch.boot_hart, fdt_blob);
}
```

**关键步骤：**
1. **获取 DTB** - 设备树地址（FIT 或全局数据）
2. **设置入口** - 目标镜像的入口点地址
3. **刷新缓存** - `invalidate_icache_all()` 确保指令最新
4. **SMP 处理** - 多核时通知其他核一起跳转
5. **执行跳转** - 调用入口函数，传递 hart ID 和 DTB

**参数传递（RISC-V 惯例）：**
- `a0` = hart ID（当前 CPU 核心编号）
- `a1` = DTB 指针（设备树）

---

## 八、其他重要函数

### 8.1 启动设备选择

```c
u32 spl_boot_device(void)
{
	return BOOT_DEVICE_RAM;  // 从 RAM 启动
}
```

**可选值：**
- `BOOT_DEVICE_RAM` — RAM（当前配置）
- `BOOT_DEVICE_MMC` — SD/eMMC
- `BOOT_DEVICE_SPI` — SPI Flash
- `BOOT_DEVICE_NAND` — NAND Flash

### 8.2 设备树设置

```c
void *board_fdt_blob_setup(int *err)
{
	void *fdt_blob = NULL;
	fdt_blob = (ulong *)&_end;  // DTB 在 BSS 结束处
	*err = 0;
	return fdt_blob;
}
```

**业务逻辑：**
- DTB 通常由 OpenSBI 或仿真器提供
- 放在 `_end` 符号处（BSS 段结束）
- 通过 `a1` 寄存器传递给下一阶段

### 8.3 板级后续初始化

```c
void spl_board_init(void)
{
	log_info("Do initialization for spl board, sizeof(struct global_data)=%lu!\n", 
			sizeof(struct global_data));
}
```

**执行时机：** 在 `spl_board_init_f()` 之后，镜像加载之前。

---

## 九、启动流程总结

```
┌─────────────────────────────────────────────────────────────┐
│ 阶段 1: Boot ROM / CPU 上电                                  │
│ └── 从固定地址加载 u-boot-spl.bin                           │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ 阶段 2: _start (arch/riscv/cpu/start.S)                      │
│ ├── 读取 CSR_MHARTID → tp (hart ID)                         │
│ ├── 保存 DTB 指针 → s1                                      │
│ ├── 设置 trap_entry (异常入口)                              │
│ ├── 屏蔽所有中断                                            │
│ └── 启用缓存 (icache_enable / dcache_enable)                │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ 阶段 3: board_init_f (arch/riscv/lib/spl.c)                  │
│ ├── spl_early_init()          # SPL 框架初始化              │
│ ├── riscv_cpu_setup()         # CPU 基础设置                │
│ ├── preloader_console_init()  # 串口控制台初始化            │
│ └── spl_board_init_f()        # 板级初始化 (重点！)         │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ 阶段 4: spl_soc_init (arch/riscv/cpu/nuclei/spl.c)           │
│ └── uclass_get_device(UCLASS_RAM, 0, &dev)                  │
│     └── DDR 初始化 (通过 Driver Model 框架)                 │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ 阶段 5: spl_board_init_f (board/nuclei/generic/spl.c)        │
│ └── csr_write(CCM_SUEN, CCM_SEN)                            │
│     └── 启用 S-mode CCM 缓存操作 (Nuclei 特有！)            │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ 阶段 6: spl_load_image                                       │
│ ├── 从 BOOT_DEVICE_RAM 读取 FIT 镜像                        │
│ ├── board_fit_image_post_process()                          │
│ │   └── image_uncipher()  # FIT 镜像解密 (安全启动)          │
│ └── 将镜像加载到内存 (如 0xc3000000)                         │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ 阶段 7: jump_to_image_no_args                                │
│ ├── invalidate_icache_all()  # 刷新指令缓存                 │
│ └── image_entry(gd->arch.boot_hart, fdt_blob)               │
│     └── 跳转到下一阶段 (OpenSBI 或 U-Boot Proper)           │
└─────────────────────────────────────────────────────────────┘
```

---

## 十、关键注意事项

### 10.1 CCM 启用顺序

```
正确: spl_soc_init() [DDR] → spl_board_init_f() [CCM]
错误: CCM 在 DDR 前启用 → 系统崩溃
```

### 10.2 不要启用 CONFIG_SPL_FRAMEWORK_BOARD_INIT_F

```c
// board/nuclei/generic/spl.c 明确警告:
// you should never select CONFIG_SPL_FRAMEWORK_BOARD_INIT_F in kconfig
```

Nuclei 使用弱符号覆盖，而非框架回调。

### 10.3 启动设备可配置

```c
// 当前从 RAM 启动，可改为:
return BOOT_DEVICE_MMC;   // SD 卡
return BOOT_DEVICE_SPI;   // SPI Flash
return BOOT_DEVICE_NAND;  // NAND Flash
```

### 10.4 双镜像机制

- `spl/u-boot-spl.bin` — SPL 镜像 (小，64-256KB)
- `u-boot.itb` — FIT 组合镜像 (大，包含 kernel + ramdisk)

---

## 参考文件

| 文件 | 功能 |
|------|------|
| `arch/riscv/cpu/start.S` | 统一入口点 `_start` |
| `arch/riscv/lib/spl.c` | RISC-V 通用 SPL 入口 `board_init_f` |
| `arch/riscv/cpu/nuclei/spl.c` | SoC 层 SPL 初始化 `spl_soc_init` |
| `board/nuclei/generic/spl.c` | 板级 SPL 业务逻辑 |
| `arch/riscv/cpu/nuclei/cache.c` | CCM 缓存控制 |
| `arch/riscv/cpu/u-boot-spl.lds` | SPL 链接脚本 |

---

## 十一、异构多核系统启动

在实际项目中，SoC可能包含多颗异构芯片。以三芯片系统为例：

### 11.1 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                     SoC 芯片架构                             │
├─────────────────────────────────────────────────────────────┤
│  N300 芯片          N600 芯片              UX900 芯片        │
│  (HSM/安全)         (RTOS/实时)            (Linux+TEE)       │
│                                                             │
│  ├─ 安全启动         ├─ 实时控制            ├─ 应用处理      │
│  ├─ 密钥管理         ├─ 外设驱动            ├─ Rich OS       │
│  ├─ 加密加速         ├─ 硬实时              ├─ TEE 安全环境   │
│  └─ 可信根           └─ 中断响应            └─ 虚拟化        │
└─────────────────────────────────────────────────────────────┘
```

### 11.2 启动流程（安全启动链）

**关键概念**：每个芯片都有自己的 Boot ROM

```
阶段 1: N300 执行（安全主导）
─────────────────────────────────────────────────────────
N300 Boot ROM (芯片内部)
    │
    ▼
N300 安全固件
    ├── 初始化 HSM 环境
    ├── 验证 N600 RTOS 固件签名
    ├── 验证 UX900 U-Boot 固件签名
    │
    ├── 唤醒 N600 ────────▶ N600 Boot ROM ──▶ N600 RTOS
    │
    └── 唤醒 UX900 ───────▶ UX900 Boot ROM
                              │
                              ▼
阶段 2: UX900 执行（U-Boot 所在芯片）
─────────────────────────────────────────────────────────
UX900 Boot ROM (芯片内部，独立执行)
    │
    ├── 基础初始化 (寄存器、时钟)
    └── 跳转到 SPL (加载到 UX900 SRAM/DRAM)
            │
            ▼
    UX900 SPL (u-boot-spl.bin)
            │
            ├── 初始化 UX900 DDR (如果 N300 未初始化)
            ├── 启用 CCM
            └── 加载 U-Boot Proper
                    │
                    ▼
            UX900 U-Boot ──▶ Linux (都在 UX900 上运行)
```

### 11.3 重要澄清

**误区**：以为整个系统只有一个 Boot ROM

**实际**：
- **N300** 有 N300 的 Boot ROM（安全启动用）
- **N600** 有 N600 的 Boot ROM（启动 RTOS）
- **UX900** 有 UX900 的 Boot ROM（启动 U-Boot/Linux）

**关系**：
- N300 的 Boot ROM **不执行** UX900 的代码
- N300 只是**验证** UX900 的代码，然后**释放复位信号**让 UX900 自己启动
- UX900 自己的 Boot ROM 执行，然后加载 SPL

### 11.4 对 U-Boot 的影响

在这种架构下，U-Boot 运行于 UX900，但可能需要：

```c
// 与 N300 HSM 通信请求安全服务
int spl_board_init_f(void) {
    // 标准 Nuclei CCM 启用
    csr_write(CCM_SUEN, CCM_SEN);
    
    // 【可选】检查 N300 是否已初始化 DDR
    if (!is_ddr_initialized_by_hsm()) {
        ddr_init();
    }
    
    // 【可选】向 N300 报告启动状态
    report_boot_status_to_hsm(BOOT_STAGE_SPL_INIT);
    
    return 0;
}

// 使用 N300 验证 FIT 镜像签名
void board_fit_image_post_process(const void *fit, int node, 
                                  void **p_image, size_t *p_size) {
    // 软件解密
    image_uncipher(fit, node, p_image, p_size);
    
    // 【可选】请求 N300 硬件验证签名
    if (fit_image_is_signed(fit, node)) {
        n300_hsm_verify_image(fit, node, *p_image, *p_size);
    }
}
```

---

## 十二、boot_from_devices 详解

`boot_from_devices` 是 SPL 框架的核心加载函数，负责从多种启动设备中尝试加载下一阶段镜像。

### 12.1 函数调用链

```
board_init_r() [common/spl/spl.c:741]
    │
    ├── board_boot_order(spl_boot_list)    ← 获取启动设备列表
    │       └── 返回: [BOOT_DEVICE_RAM, NONE, NONE, NONE, NONE]
    │
    └── boot_from_devices(&spl_image, spl_boot_list, 5)
            │
            ├── 遍历所有启动设备
            ├── 找到匹配的 image loader
            ├── 调用 spl_load_image() 加载镜像
            └── 成功则返回，失败继续尝试
```

### 12.2 核心源码分析

```c
static int boot_from_devices(struct spl_image_info *spl_image,
                             u32 spl_boot_list[], int count)
{
    // 1. 获取所有已注册的 image loader（链接器生成的数组）
    struct spl_image_loader *drv =
        ll_entry_start(struct spl_image_loader, spl_image_loader);
    const int n_ents =
        ll_entry_count(struct spl_image_loader, spl_image_loader);
    int ret = -ENODEV;
    int i;

    // 2. 遍历启动设备列表（最多5个）
    for (i = 0; i < count && spl_boot_list[i] != BOOT_DEVICE_NONE; i++) {
        struct spl_image_loader *loader;
        int bootdev = spl_boot_list[i];    // 当前尝试的设备

        // 3. 在所有注册的 loader 中查找匹配当前设备的
        for (loader = drv; loader != drv + n_ents; loader++) {
            if (bootdev != loader->boot_device)
                continue;                  // 设备类型不匹配，跳过

            // 4. 打印日志
            printf("Trying to boot from %s\n", spl_loader_name(loader));

            // 5. 调用 loader 加载镜像！
            if (loader && !spl_load_image(spl_image, loader)) {
                // 加载成功
                spl_image->boot_device = bootdev;
                return 0;                  // 成功返回
            }
        }
    }

    return ret;    // 所有设备都失败
}
```

### 12.3 Image Loader 注册机制

#### 数据结构

```c
struct spl_image_loader {
    const char *name;           // 名称，如 "RAM"
    uint boot_device;           // 设备类型，如 BOOT_DEVICE_RAM
    int (*load_image)(struct spl_image_info *spl_image,
                      struct spl_boot_device *bootdev);
};
```

#### 注册宏

```c
#define SPL_LOAD_IMAGE_METHOD(_name, _priority, _boot_device, _method) \
    SPL_LOAD_IMAGE(_boot_device ## _priority ## _method) = { \
        .name = _name, \
        .boot_device = _boot_device, \
        .load_image = _method, \
    }
```

#### 实际注册示例（common/spl/spl_ram.c）

```c
SPL_LOAD_IMAGE_METHOD("RAM", 0, BOOT_DEVICE_RAM, spl_ram_load_image);

// 展开后相当于：
static struct spl_image_loader BOOT_DEVICE_RAM0spl_ram_load_image = {
    .name = "RAM",
    .boot_device = BOOT_DEVICE_RAM,
    .load_image = spl_ram_load_image,
};
```

### 12.4 链接器数组机制

U-Boot 使用链接器脚本将所有的 loader 收集到一个数组中：

```
.u_boot_list_2_spl_image_loader_2_* 段
├── spl_ram_load_image     ← BOOT_DEVICE_RAM
├── spl_mmc_load_image     ← BOOT_DEVICE_MMC1/2
├── spl_spi_load_image     ← BOOT_DEVICE_SPI
├── spl_nand_load_image    ← BOOT_DEVICE_NAND
└── ... 其他 loader
```

### 12.5 RAM Loader 的具体实现

```c
static int spl_ram_load_image(struct spl_image_info *spl_image,
                              struct spl_boot_device *bootdev)
{
    struct legacy_img_hdr *header;
    
    // 1. FIT 镜像的固定加载地址
    header = (struct legacy_img_hdr *)CONFIG_SPL_LOAD_FIT_ADDRESS;
    // 例如：0xC300_0000

    // 2. 检查是否是 FIT 格式（魔数检查）
    if (image_get_magic(header) == FDT_MAGIC) {
        struct spl_load_info load;
        
        load.bl_len = 1;                           // 块大小
        load.read = spl_ram_load_read;             // 读函数
        
        // 3. 解析并加载 FIT 镜像
        ret = spl_load_simple_fit(spl_image, &load, 0, header);
        // 这会解析 FIT 中的 kernel.itb，提取 OpenSBI + U-Boot
    } else {
        // 兼容旧格式（Legacy image）
        ret = spl_parse_image_header(spl_image, bootdev, header);
    }

    return ret;
}
```

### 12.6 加载后的 spl_image 结构

```c
struct spl_image_info {
    const char *name;              // "OpenSBI"
    u8 os;                         // IH_OS_OPENSBI (0x13)
    u32 load_addr;                 // 0xC000_0000
    u32 entry_point;               // 0xC000_0000
    void *fdt_addr;                // 0xC800_0000 (DTB 地址)
    u32 boot_device;               // BOOT_DEVICE_RAM
    // ... 其他字段
};
```

这个结构体后续被 `jump_to_image_no_args()` 使用来完成最终跳转。

---

## 十三、汇编到 C 语言的跳转机制

### 13.1 跳转代码（arch/riscv/cpu/start.S）

```asm
/* 准备跳转到 board_init_f */
	mv	a0, zero		/* a0 <-- boot_flags = 0 */
	la	t5, board_init_f      /* t5 = board_init_f 地址 */
	jalr	t5			/* 跳转！ */
```

### 13.2 详细解析

| 指令 | 作用 |
|------|------|
| `mv a0, zero` | 设置函数参数 a0 = 0（boot_flags） |
| `la t5, board_init_f` | 加载函数地址到 t5 寄存器 |
| `jalr t5` | 跳转并链接：ra = PC+4, PC = t5 |

**jalr 指令**：
- **保存返回地址**：`ra = PC + 4`（下一条指令地址）
- **跳转到目标**：`PC = t5`（board_init_f 的地址）
- 相当于 C 语言的函数调用

### 13.3 为什么用 jalr 而不是 jal？

| 指令 | 适用场景 |
|------|---------|
| `jal label` | 相对跳转，目标在当前代码附近（±1MB） |
| `jalr reg` | 绝对跳转，目标地址在运行时确定 |

**使用 jalr 的原因**：
1. **地址不确定** - `board_init_f` 可能在链接后的任意位置
2. **位置无关代码 (PIC)** - U-Boot 支持 PIE
3. **灵活性** - 支持重定位后跳转

### 13.4 参数传递（RISC-V ABI）

```
寄存器    用途
─────────────────────────────────
a0-a7     函数参数 (0-7个)
a0-a1     函数返回值
ra        返回地址
sp        栈指针
```

**实际执行**：
```c
// 汇编设置 a0 = 0，然后调用
void board_init_f(ulong dummy)  // dummy = 0
{
    // 现在运行在 C 语言世界
    spl_early_init();
    ...
}
```

---

*基于 U-Boot 2023.10 版本 nuclei 平台代码*
*更新日期：2026-03-26*
