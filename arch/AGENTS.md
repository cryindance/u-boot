# arch/ 目录

**架构特定代码** — 支持 13+ 处理器架构的底层实现。

## 目录结构

```
arch/
├── arm/          # ARM 32/64-bit (armv7, armv8, arm9xxx)
├── riscv/        # RISC-V (andesv5, fu540/740, nuclei)
├── x86/          # x86/x86_64 (Intel/AMD)
├── mips/         # MIPS (mt7621, ath79, etc.)
├── powerpc/      # PowerPC (mpc8xxx, 85xx, etc.)
├── arc/          # ARC 处理器
├── microblaze/   # Xilinx MicroBlaze
├── m68k/         # Motorola 68000
├── nios2/        # Altera Nios II
├── sh/           # SuperH
├── sparc/        # SPARC (LEON)
├── xtensa/       # Xtensa
└── sandbox/      # 沙盒架构 (host testing)
```

## 查找指南

| 内容类型 | 位置 |
|---------|------|
| 架构入口点 | `arch/<arch>/cpu/<variant>/start.S` |
| 链接脚本 | `arch/<arch>/cpu/u-boot.lds` |
| SPL 链接脚本 | `arch/<arch>/cpu/u-boot-spl.lds` |
| CPU 初始化 | `arch/<arch>/cpu/` |
| 板级库函数 | `arch/<arch>/lib/` |
| 板级头文件 | `arch/<arch>/include/` |

## 约定

- **入口点**: 所有 start.S 定义 `_start` 符号
- **SPL/TPL**: 小内存引导阶段，代码独立
- **多变体**: ARM 按内核版本分目录 (armv7/, armv8/)
- **设备树**: ARM/RISC-V 重度依赖，源文件在 `arch/<arch>/dts/`

## 注意事项

- **DO NOT** 在代码重定位前重映射内存
- **DO NOT** 关闭调试/桥接时钟 (SocFPGA)
- 链接脚本定义入口 `ENTRY(_start)`
