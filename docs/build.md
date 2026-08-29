# CPUSTC U-Boot 构建与启动

[返回项目 README](../README.md)

## 配置

| 配置 | 用途 |
| --- | --- |
| `la32rsoc_l1_defconfig` | CPUSTC-SoC，L1 Cache |
| `la32rsoc_l2_defconfig` | CPUSTC-SoC，L1/L2 Cache |
| `la32rsoc_defconfig` | L2 配置兼容入口 |

## L1 与 L2 配置

两个配置都针对 CPUSTC-SoC，L1 Cache line 为 64 字节：

| Profile | U-Boot 配置 | Cache 维护 | 配置标识 | NAND 诊断 |
| --- | --- | --- | --- | --- |
| L1 | `la32rsoc_l1_defconfig` | L1 D-Cache writeback/invalidate | `-cpustc-l1` | 关闭 |
| L2 | `la32rsoc_l2_defconfig` | L1 维护后增加 selector-2 L2 writeback/invalidate | `-cpustc-l2` | 开启 |

L1 profile 用匹配 L1 Cache 的 FPGA bitstream；L2 profile 用匹配统一 L2 Cache 的
FPGA bitstream。`la32rsoc_defconfig` 与 `la32rsoc_l2_defconfig` 相同，作为兼容入口。

L1 构建：

```sh
make O=out/l1 la32rsoc_l1_defconfig
make O=out/l1 -j"$(nproc)"
```

L2 构建：

```sh
make O=out/l2 la32rsoc_l2_defconfig
make O=out/l2 -j"$(nproc)"
```

## 工具链

```sh
export ARCH=la32r
export CROSS_COMPILE=/path/to/loongarch-toolchain/bin/loongarch32r-linux-gnusf-
```

## 生成 U-Boot

下面以 L2 配置为例生成 U-Boot、反汇编和二进制文件；L1 配置使用上面的 L1 输出目录：

```sh
make O=out/l2 la32rsoc_l2_defconfig
make O=out/l2 -j"$(nproc)"
${CROSS_COMPILE}objdump -S out/l2/u-boot > out/l2/u-boot.S
${CROSS_COMPILE}objcopy -O binary out/l2/u-boot out/l2/u-boot.bin
```

自动构建：

1. 将 `CPUSTC_CACHE_PROFILE` 设为 `l1` 或 `l2`。
2. 执行 `./build_meta.sh`；脚本使用对应的 `la32rsoc_<profile>_defconfig` 和
   `out/<profile>` 输出目录。

```sh
CPUSTC_CACHE_PROFILE=l1 ./build_meta.sh
CPUSTC_CACHE_PROFILE=l2 ./build_meta.sh
```

脚本输出 `out/<profile>/u-boot.bin`、反汇编文件和带 Cache/频率标识的制品；发布
副本默认位于 `out/published`，可用 `CPUSTC_PUBLISH_DIR` 修改。

CPU 定时器频率默认 50 MHz，可在执行前设置：

```sh
CPUSTC_CPU_FREQ_HZ=60000000 ./build_meta.sh
```

## U-Boot 环境

源码默认环境定义在 `include/configs/la32rsoc_demo.h`。NAND 中存在有效环境时，
其值会覆盖源码默认值。

查看和临时设置变量：

```text
printenv bootcmd
setenv name value
```

将当前环境保存到 NAND：

```text
saveenv
```

仅用源码默认值恢复 `bootcmd`，或恢复全部默认环境：

```text
env default bootcmd
env default -a
```

恢复后执行 `saveenv` 才会写入 NAND。本板的 U-Boot `reset` 未实现；需要重新启动时
使用物理复位、重新上电或重新下载 FPGA bitstream。

## Linux 启动

USB：

将 `vmlinux` 放在 U 盘第一个 ext4 分区的根目录。可先检查分区和文件：

```text
usb start
part list usb 0
ext4ls usb 0:1 /
```

手动启动：

```text
ext4load usb 0:1 0xa3000000 /vmlinux
usb stop
bootelf 0xa3000000 console=ttyS0,115200 rdinit=/init
```

默认 `bootcmd` 会自动执行上述 USB 启动流程；USB 读取失败时回退到 NAND。

### USB 读取测试

测试环境：CPUSTC-SoC L2 配置、50 MHz 定时器、OHCI 1.0，U 盘为
Kingston DataTraveler 3.0（全速 USB）。测试文件位于第一个 ext4 分区，大小为
32,990,616 字节。

U-Boot 使用以下命令测试：

```text
usb start
ext4load usb 0:1 0xa3000000 /vmlinux
```

| 环境 | 用时 | 读取速度 |
| --- | ---: | ---: |
| U-Boot，默认 10 KiB READ10 | 118.386 s | 271.5 KiB/s |
| U-Boot，120 KiB READ10 | 38.665 s | 833.2 KiB/s |
| Linux 5.14，`dd bs=1M iflag=direct` 三次平均 | 36.097 s | 892.5 KiB/s |

120 KiB READ10 使 U-Boot 读取速度提升约 3.06 倍，达到 Linux 对照结果的
93.4%。已验证可启动到登录界面。当前已设置为最大 120 KiB READ10。

SD 卡：

```text
fatload mmc 0 0xa0e00000 vmlinux
bootelf 0xa0e00000 console=ttyS0,115200 rdinit=/init
```

网络：

```text
setenv serverip 192.0.2.1
setenv ipaddr 192.0.2.2
tftpboot 0xa3000000 vmlinux
bootelf 0xa3000000 console=ttyS0,115200 rdinit=/init
```

## 许可证

U-Boot 继续沿用上游 GPLv2、逐文件 SPDX 声明和例外条款，详见
[`Licenses/README`](../Licenses/README)。
