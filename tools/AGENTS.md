# tools/ 目录

U-Boot 构建与维护工具集

## 结构

```
tools/
├── buildman/        # 多板并行构建系统
├── binman/          # 二进制镜像组合工具
├── dtoc/            # 设备树转 C 代码
├── patman/          # 补丁管理（发送邮件/Patchwork）
├── u_boot_pylib/    # Python 共享库
├── scripts/         # 辅助脚本
├── docker/          # Docker 镜像定义
└── *.c, *.py        # 独立工具（mkimage 等）
```

## 主要工具

| 工具 | 入口 | 说明 |
|------|------|------|
| buildman | `buildman/buildman` | 并行构建多个板卡配置 |
| binman | `binman/binman` | 创建复杂的固件镜像布局 |
| dtoc | `dtoc/dtoc` | 将设备树数据转换为 C 结构体 |
| patman | `patman/patman` | 补丁格式化、发送、跟踪 |

## 快速定位

| 需求 | 位置 |
|------|------|
| 批量构建多个板卡 | `buildman/` |
| 创建/修改固件镜像 | `binman/` |
| 设备树处理 | `dtoc/` |
| 补丁提交工作流 | `patman/` |
| 共享 Python 工具函数 | `u_boot_pylib/` |
| 镜像格式处理（C） | `mkimage.c`, `imagetool.c` |

## 约定

- **Python 版本:** 3.7+
- **命名:** snake_case（见 `scripts/style.py`）
- **配置:** 每个子目录有 `pyproject.toml`
- **测试:** `test/` 子目录或 `*_test.py`

## 运行方式

```bash
# 直接运行（无需安装）
./tools/buildman/buildman --help
./tools/binman/binman --help

# 或使用 Python 模块方式
cd tools && python3 -m buildman
```
