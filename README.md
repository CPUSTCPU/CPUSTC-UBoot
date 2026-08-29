# CPUSTC U-Boot

[中文](README.md) | [English](README.en.md)

CPUSTC LoongArch32 U-Boot，面向 CPUSTC-SoC FPGA 板，默认从 USB 加载 Linux
ELF 和独立根文件系统，并提供 NAND 回退及 SD 卡、网络手动加载支持。仓库保留
U-Boot 上游目录和许可证结构。

## Pipeline

```text
CPUSTC-SoC / DDR / 外设
        -> U-Boot LA32R 初始化
        -> USB 加载 vmlinux，NAND 回退
        -> bootelf
        -> CPUSTC-Linux / Buildroot
```

## Requirements

| 工具 | 用途 |
| --- | --- |
| LoongArch32 `ilp32s` 工具链 | 编译 U-Boot |
| GNU make、binutils、`nproc` | 配置和构建 |
| CPUSTC-SoC FPGA 板 | 启动测试 |

## Quick Start

```sh
export ARCH=la32r
export CROSS_COMPILE=/path/to/loongarch-toolchain/bin/loongarch32r-linux-gnusf-
make O=out/l2 la32rsoc_l2_defconfig
make O=out/l2 -j"$(nproc)"
```

`out/l2` 是启用统一 L2 Cache 的配置；L1-only 配置使用：

```sh
make O=out/l1 la32rsoc_l1_defconfig
make O=out/l1 -j"$(nproc)"
```

两种配置的 Cache 级别必须与 FPGA bitstream 对应。详细差异、Linux 启动
和串口测量见 [U-Boot 文档](docs/README.md)。

## Repository Structure

源码位于 `arch/la32r/`、`configs/`、`drivers/` 和 `board/`；构建脚本为
`build_meta.sh`，自有说明位于 `docs/`。

## Documentation

- [文档索引](docs/README.md)
- [构建与启动](docs/build.md)
- [启动测量工具](docs/tools.md)
- [U-Boot 上游 README](README)
- [U-Boot 上游文档](doc/)

## Validation

当前源码已在 CPUSTC-SoC FPGA 板上完成上板验证。

## License

U-Boot 沿用上游 GPLv2、逐文件 SPDX 声明和例外条款，详见
[Licenses/README](Licenses/README)。CPUSTC 新增文件按文件头 SPDX 声明处理。
