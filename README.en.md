# CPUSTC U-Boot

[中文](README.md) | English

LoongArch32 U-Boot for the CPUSTC-SoC FPGA board. It initializes the board,
loads Linux and its separate root filesystem from USB by default, and falls
back to the embedded-rootfs image in NAND. Manual SD and TFTP loading remain
available. The upstream U-Boot directory and licensing structure are retained.

## Pipeline

```text
CPUSTC-SoC / DDR / peripherals
        -> U-Boot LA32R initialization
        -> vmlinux from USB, with NAND fallback
        -> bootelf
        -> CPUSTC-Linux / Buildroot
```

## Requirements

| Tool | Purpose |
| --- | --- |
| LoongArch32 `ilp32s` toolchain | U-Boot build |
| GNU make, binutils, and `nproc` | Configuration and build |
| CPUSTC-SoC FPGA board | Boot testing |

## Quick Start

```sh
export ARCH=la32r
export CROSS_COMPILE=/path/to/loongarch-toolchain/bin/loongarch32r-linux-gnusf-
make O=out/l2 la32rsoc_l2_defconfig
make O=out/l2 -j"$(nproc)"
```

`out/l2` enables the unified L2 Cache profile. For the L1-only profile, use:

```sh
make O=out/l1 la32rsoc_l1_defconfig
make O=out/l1 -j"$(nproc)"
```

The Cache profile must match the FPGA bitstream. See [U-Boot documentation](docs/README.md)
for the profile differences, Linux boot commands, and serial measurement.

## Repository Structure

Source is under `arch/la32r/`, `configs/`, `drivers/`, and `board/`. The CPUSTC
build script is `build_meta.sh`; project notes are under `docs/`.

## Documentation

- [Chinese documentation index](docs/README.md)
- [Build and boot](docs/build.md)
- [Boot measurement tool](docs/tools.md)
- [Upstream U-Boot README](README)
- [Upstream U-Boot documentation](doc/)

## Validation

The current source has been validated on the CPUSTC-SoC FPGA board.

## License

U-Boot remains under the upstream GPLv2 terms, per-file SPDX declarations, and
exceptions. See [Licenses/README](Licenses/README). CPUSTC additions use the SPDX
declarations in their individual files.
