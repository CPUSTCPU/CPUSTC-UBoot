# LA32R U-Boot 调试记录

## 变更项：增加无人值守 U-Boot 烧写工具

- 修改：2026-07-14 23:04 新增 `tools/cpustcpu_program_fpga.tcl` 和
  `tools/cpustcpu_flash_uboot.py`；工具要求唯一 XC7A200T 目标，在 programmer bit
  下载前打开候选串口，根据实际输出选择唯一活动端口，支持探测、XMODEM 传输和
  正常 SoC bit 切换，所有 Vivado 与串口输出写入指定日志目录。
- 修正：移除 FPGA 器件不存在的 `HW_TARGET` 属性读取，目标唯一性继续由器件型号和
  数量检查保证。
- 修正：XMODEM 启动流程改为先发送回车并保存按键提示，再发送 `x`；确认串口至少
  连续返回三个 `C` 后才调用 `sx`。增加 `--transfer-only`，用于首次烧写后保留
  programmer bit 并核对 Flash 完成提示，再切换正常 SoC bit。
- 修正：显式指定唯一串口时允许 programmer bit 下载后的启动输出为空，随后仍必须
  通过回车提示和 `CCC` 握手校验；自动选择串口时继续要求唯一活动端口。
- 修正：传输参数使用标准 128 字节块 XMODEM CRC，移除接收端不支持的 XMODEM-1K。
- 修正：XMODEM 数据传输改为直接使用同一个 pyserial 连接完成，逐块处理
  ACK/NAK/CAN 和 EOT，避免 `sx` 接管非阻塞串口文件描述符后首块无法收到 ACK。
- 修正：烧写串口按板上既有 Minicom 配置默认使用 `230400 5N1`，并提供
  `--data-bits 5/8` 进行可复现对照。
- 修改：新增 `tools/cpustcpu_nand_info.py`，在关闭 programmer 串口并下载正常 SoC
  bit 后，以 U-Boot 参数重新打开同一串口，保留 FTDI 缓存的启动输出并中断
  autoboot；仅在确认 U-Boot 提示符后执行只读 `nand info`，全程保存日志。
- 修正：显式指定共用串口时，重新打开后立即发送一个回车，用于中断 autoboot 或
  唤出已经运行但启动输出未被主机保留的 U-Boot 提示符。
- 修正：正常 SoC 的 U-Boot 控制台按板级配置使用 `115200 8N1`；保留
  `--data-bits 5/8` 仅用于串口参数判别实验。
- 修改：新增 `tools/cpustcpu_test_uboot_ram.py`，通过工作 U-Boot 的 `loadx` 将待测
  镜像加载到 `0xa0200000`，核对主机与板端 CRC32 后执行入口，并保存早期启动日志；
  用于区分镜像运行问题与 SPI Flash 烧写/取指问题。
- 修正：`loadx` 握手按收到至少一个 CRC 请求字符 `C` 判定，不沿用 programmer
  UART 快速连续返回 `CCC` 的专用判据。
- 修正：`arch/la32r/config.mk` 的 binary section 列表加入 `.got`。此前直接执行
  `make` 生成的 `u-boot.bin` 会把 `.got` 区域填成 `0xffffffff`，早期复制循环因此
  在 `st.w` 触发地址未对齐异常；`build_meta.sh` 末尾的完整 objcopy 会掩盖该问题。
- 建议：首次使用 `--probe-only` 只确认活动串口；完整烧写需同时提供 `--image`、
  `--programmer-bit`、`--soc-bit` 和 `--log-dir`。候选串口多于一个产生输出时工具
  会停止，避免向错误设备发送数据。

## 变更项：恢复 LS1A NAND 只识别测试

- 修改：2026-07-14 23:01 在 `configs/la32rsoc_defconfig` 重新启用
  `CONFIG_NAND`、`CONFIG_NAND_LS1A` 和 `CONFIG_CMD_NAND`；本轮保持现有
  LS1A 驱动不变，只验证启动、复位、READ ID 和状态路径。
- 修正：`CONFIG_SYS_NAND_BASE` 从缓存映射 `0xbfe78000` 改为非缓存映射
  `0x9fe78000`，与 Linux LS1A NAND 驱动及本平台其他 MMIO 地址属性一致。缓存
  地址不符合本平台 MMIO 访问约定；DDR 实测表明仅修正该地址仍会在 NAND 初始化
  阶段触发 CPU 异常，不能将缓存属性认定为本次异常的唯一根因。
- 诊断：在 `ls1a_nand_init()` 的内存分配、控制器寄存器配置和回调安装之间加入
  串口阶段标记，用于定位异常发生在驱动初始化还是后续 `nand_scan()`；定位完成前
  该版本仅允许 DDR 启动测试。
- 修正：注册由 LS1A 控制器管理片选的空操作 `select_chip`。此前 NAND 核心安装的
  传统默认实现会在取消片选时调用未注册的 `cmd_ctrl`，导致空函数指针异常；该处理
  与项目 Linux LS1A NAND 驱动一致。DDR 对照表明加入该回调后异常仍存在，因此
  继续在首次 `NAND_CMD_RESET` 的入口、发令和 DONE 等待之间增加诊断标记。
- 修正：DONE 轮询改用本平台 `get_timer()` 的毫秒计时。LA32R `get_tbclk()` 返回
  MHz 数值 `33`，通用 `timer_get_us()` 按 Hz 使用该返回值；此前
  `readl_poll_timeout()` 在 RESET 发令后进入错误的微秒计时路径。超时保持 200 ms。
- 验证：计时修复定位版在 DDR 中完成 RESET 和 READ ID，识别 128 MiB NAND；
  `nand info` 显示 128 KiB 擦除块、2048 字节页和 64 字节 OOB。验证后删除所有
  临时阶段标记，保留功能修复。
- 修改：按项目 Linux LS1A NAND 驱动和同平台 MMC DMA 约定补齐同步页读、页写、
  OOB 读、块擦除、`read_buf` 与 `write_buf`。DMA 使用 32 字节对齐的 4096 字节
  缓冲区和描述符、物理地址、order 寄存器启动位 3、写方向位 12，并在传输前后
  执行缓存维护；禁用子页写以匹配控制器一次页操作模型。
- 建议：构建归档后烧写，保存完整启动串口日志并执行 `nand info`；页读写与擦除
  仍未实现，本轮禁止执行 `nand erase`、`nand write` 和 `nand read`。

## 变更项：增加DM9161环回速率选择

- 修改：`drivers/net/dmfe.c`增加`dmfe_phy_loopback_speed=auto/100/10`；显式启用环回时，`100`写BMCR `0x6100`，`10`写`0x4100`，`auto`启用并重启自动协商，未设置时保持原BMCR位并只置Loopback。
- 建议：强制100M时执行`setenv dmfe_phy_loopback 1; setenv dmfe_phy_loopback_speed 100; ping $serverip`，强制10M时把速率值改为`10`；强制模式会关闭自动协商，测试完成后复位板卡恢复PHY默认状态。

## 变更项：等待DM9161数字环回稳定

- 修改：`drivers/net/dmfe.c`在显式启用`dmfe_phy_loopback=1`且BMCR回读确认后等待800 ms，并记录等待前后的RECR；用于覆盖DM9161在100 Mbps环回模式下最长720 ms的无有效MII RX输出时间。
- 建议：重新构建后执行`setenv dmfe_debug 2; setenv dmfe_sw_pad 0; setenv dmfe_phy_loopback 1; ping $serverip`，同时将RX ILA触发改为`probe2/rxer=1`；检查稳定等待期间和首帧发送后的RECR变化。

## 变更项：DMFE DMA 对象强制 64 字节对齐

- 修改：`drivers/net/dmfe.c` 将TX/RX描述符环、TX/RX数据缓冲区和Setup Frame统一放入`.bss.align64`，并强制64字节对齐，用于验证DMA对象基地址与通信成功率的相关性。
- 建议：重新构建后检查`System.map`中的`_tx_ring`、`_rx_ring`、`__NetTxPackets`、`__NetRxPackets`和`setup_frame`地址，确认低6位为0；上板保持其余条件不变执行`setenv dmfe_debug 2; ping $serverip`进行单变量对照。

## 变更项：修复 dmfe Setup Frame 缓存别名

- 修改：`drivers/net/dmfe.c` 的 `send_setup_frame()` 统一通过非缓存别名清零和填充192字节Setup Frame，并将同一非缓存地址写入TX描述符，避免CPU缓存内容未回写时MAC读取旧数据。
- 建议：使用LA32R交叉工具链重新构建；上板执行 `setenv dmfe_debug 2; ping $serverip`，确认Setup描述符完成且普通ARP收发行为无回退。

## 变更项：临时关闭 LS1A NAND

- 修改：`configs/la32rsoc_defconfig` 关闭 `CONFIG_NAND`、`CONFIG_NAND_LS1A` 和 `CONFIG_CMD_NAND`，启动阶段不再进入 NAND 初始化；驱动源码保留，便于后续继续排查异常。
- 建议：执行 `make clean && make la32rsoc_defconfig && ./build_meta.sh` 重新构建并烧写，确认启动日志跳过 `NAND:` 且不再触发异常；关闭期间不提供 `nand` 命令。

## 变更项：增加 LS1A NAND 芯片识别支持

- 修改：新增 LS1A APB NAND 控制器初始驱动，接入 U-Boot raw NAND 框架，并为 `la32rsoc_defconfig` 启用 NAND 与 `nand` 命令；首阶段支持复位、READ ID 和状态读取。驱动私有数据在运行时分配；通用 NAND 初始化在 `CONFIG_NAND_LS1A` 下直接调用强符号 `ls1a_nand_init()`，避免 LA32R 将弱符号入口错误重定位成 relocation offset。
- 建议：执行 `make clean && make la32rsoc_defconfig && ./build_meta.sh` 构建并烧写新的 `u-boot.bin`，进入 U-Boot 后使用 `nand info` 验证 K9F1G08U0C 是否识别为 Samsung `EC F1`、128 MiB。页读写与擦除尚未启用，当前不要执行 `nand erase` 或 `nand write`。

## 变更项：改用内部 framebuffer/simplefb 路线

- 修改：`configs/la32rsoc_defconfig` 撤销 PCI/VESA/BIOSEMU 相关配置，保留
  `CONFIG_DM_VIDEO`、常用像素格式和 ANSI 控制台支持，并启用
  `CONFIG_VIDEO_SIMPLE`、`CONFIG_CONSOLE_MUX`、`CONFIG_SYS_CONSOLE_IS_IN_ENV`；
  同时移除 `include/configs/la32rsoc_demo.h` 中的 `CONFIG_BIOSEMU`、
  `VIDEO_IO_OFFSET`，并将默认 `stdout/stderr` 设置为 `serial,vidconsole`。
- 建议：内部 VGA/framebuffer 控制器需要在设备树中按真实硬件补
  `compatible = "simple-framebuffer"` 节点，并正确填写 `reg`、`width`、
  `height`、`stride`、`format`。验证命令：
  `make la32rsoc_defconfig && make ARCH=la32r CROSS_COMPILE=/opt/loongarch-toolchain/bin/loongarch32r-linux-gnusf- -j4`。

## 变更项：修正 la32rsoc 网卡节点字段

- 修改：`arch/la32r/dts/la32rsoc_demo.dts` 中，修正 `gmac0` 节点的
  `evice_type` 为 `device_type`。
- 建议：这是设备树字段拼写修正，无需额外操作；重新编译并使用新的 DTB/U-Boot 即可。

## 变更项：为 dmfe 调试日志增加运行时开关

- 修改：`drivers/net/dmfe.c` 中，将 `pr_debug()` 改为由运行时变量控制，
  并在 `dmfe_start()` 中读取 U-Boot 环境变量 `dmfe_debug`。
- 建议：执行网络命令前开启或关闭调试：

```bash
setenv dmfe_debug 1
ping $serverip
```

```bash
setenv dmfe_debug 0
ping $serverip
```

`dmfe_debug` 在 `dmfe_start()` 中读取，因此应在执行 `ping`、`tftpboot`
等网络命令前设置；一次网络命令开始后再修改环境变量，通常不会影响当前这次传输。

## 变更项：增加 dmfe 发送路径日志

- 修改：`drivers/net/dmfe.c` 中，在 `dmfe_send()` 增加发送路径日志，输出
  `len`、`tx_len`、`desc` 和当前 TX descriptor `status`。
- 建议：排查 ARP/TX descriptor 问题时再开启 `dmfe_debug`。大规模传输时建议关闭：

```bash
setenv dmfe_debug 0
tftpboot ...
```

串口输出较慢，持续打印 `dmfe_send()` 日志会明显影响网卡传输时序和性能。

## 变更项：调整构建脚本

- 修改：`build_meta.sh` 中，构建前执行 `make clean` 和
  `make la32rsoc_defconfig`；交叉编译工具链路径改为
  `/opt/loongarch-toolchain/bin/`；使用 `${CROSS_COMPILE}` 调用
  `objdump/objcopy`；构建后备份 `u-boot.bin`。
- 建议：使用该脚本前确认 `/opt/loongarch-toolchain/bin/` 存在且包含
  `loongarch32r-linux-gnusf-` 工具链；同时确认备份目录可写。

## 变更项：增加 logo 转 RGB565 framebuffer 脚本

- 修改：`convert.py` 改为命令行工具，默认将 `logo.png` 转换为
  `640x480`、little-endian RGB565 裸数据 `vga565.raw`，支持指定输入、
  输出、缩放方式和背景色。
- 建议：建议在 Python 虚拟环境中安装 Pillow 后运行：
  `python3 -m venv .venv && source .venv/bin/activate && python -m pip install Pillow && python convert.py`。
  生成文件应为 `614400` 字节；在 U-Boot 中执行 `loadb 87e00000`
  后通过串口发送 `vga565.raw`。

## 变更项：U-Boot 启动时自动显示 framebuffer logo

- 修改：新增 `arch/la32r/cpu/boot_logo.c` 和
  `arch/la32r/cpu/boot_logo_data.S`，通过 `CONFIG_LA32R_BOOT_LOGO`
  将根目录 `vga565.raw` 编进 U-Boot，并在 `board_late_init()` 中拷贝到
  VGA framebuffer `0x87e00000`；logo 默认显示 `3000ms` 后清空 framebuffer，
  再切换到 `serial,vidconsole` 显示 U-Boot 控制台。
- 建议：构建前确认 `vga565.raw` 存在且大小为 `614400` 字节；推荐先执行
  `python convert.py` 重新生成，再执行
  `make la32rsoc_defconfig && make ARCH=la32r CROSS_COMPILE=/opt/loongarch-toolchain/bin/loongarch32r-linux-gnusf- -j4`。
  如需调整显示时间，可修改 `CONFIG_LA32R_BOOT_LOGO_DELAY_MS`。
  若需要关闭启动 logo，建议在构建配置中取消 `CONFIG_LA32R_BOOT_LOGO`；
  当前 `CONFIG_ENV_IS_NOWHERE` 下，U-Boot 里执行 `setenv bootlogo 0`
  不能持久保存到下次重启。

## 变更项：默认关闭 VGA 文本控制台输出

- 修改：`include/configs/la32rsoc_demo.h` 中默认 `stdout/stderr` 改为
  `serial`，并增加默认环境变量 `bootlogo=1`。
- 建议：这样启动日志、倒计时和命令提示符不会覆盖 logo。若需要把 U-Boot
  文本重新输出到 VGA，可临时执行 `setenv stdout serial,vidconsole` 和
  `setenv stderr serial,vidconsole`，但这会在 framebuffer 上叠加文字。

## 变更项：VGA 控制台改为黑底浅色字

- 修改：`configs/la32rsoc_defconfig` 启用 `CONFIG_SYS_WHITE_ON_BLACK`，
  让 DM video/vidconsole 初始化时使用黑色背景和浅色前景。
- 建议：用于避免 logo 结束后 vidconsole 默认清成白底，导致误判为白屏。
  验证时可在 U-Boot 串口执行 `echo VGA_TEST`，应同时在 VGA 上看到文字。

## 变更项：启用 console 设备诊断命令

- 修改：`configs/la32rsoc_defconfig` 启用 `CONFIG_CMD_CONSOLE`，恢复
  U-Boot 的 `coninfo` 命令。
- 建议：重编译烧录后执行 `coninfo`，确认 `vidconsole` 是否已注册为输出设备；
  该命令用于区分“stdout 环境变量正确”与“实际 stdio/console 设备绑定正确”。

## 变更项：切换 VGA 控制台后强制刷新 iomux

- 修改：`arch/la32r/cpu/boot_logo.c` 中，logo 清屏后设置
  `stdout/stderr=serial,vidconsole`，并额外调用 `iomux_doenv()` 立即刷新
  实际 console mux 设备列表。
- 建议：用于排除环境变量已经变更、但 `console_devices[stdout/stderr]`
  未同步更新的问题。验证时可执行 `printenv stdout stderr`、`coninfo`、
  `echo VGA_TEST` 对照串口和 VGA 输出。

## 变更项：dmfe 调试宏改名

- 修改：`drivers/net/dmfe.c` 中将本地 `pr_debug(...)` 调试宏改名为
  `dmfe_dbg(...)`，避免覆盖 U-Boot 公共 `pr_debug` 宏并消除重定义告警。
- 建议：使用方式不变，仍通过 `setenv dmfe_debug 1` 开启 dmfe 网卡调试，
  通过 `setenv dmfe_debug 0` 关闭。

## 变更项：修复 DM 串口 ready 标志

- 修改：`drivers/serial/serial-uclass.c` 中，`serial_init()` 找到 DM 串口后
  正确设置 `GD_FLG_SERIAL_READY`。
- 建议：该标志会影响 `puts()/putc()` 是否进入标准 console mux 路径。未设置时，
  启用 `CONFIG_DEBUG_UART` 后输出会直接走 debug UART，导致
  `stdout=serial,vidconsole` 无法分发到 VGA。重编译烧录后可执行
  `setenv stdout serial,vidconsole`、`setenv stderr serial,vidconsole`、
  `echo VGA_TEST`，对照串口和 VGA 输出。

## 变更项：显式关闭 pre-console buffer

- 修改：`configs/la32rsoc_defconfig` 将 `CONFIG_PRE_CONSOLE_BUFFER` 显式设为
  未启用，并移除依赖该开关的 `CONFIG_PRE_CON_BUF_SZ` 和
  `CONFIG_PRE_CON_BUF_ADDR=0x9fe001e0`。
- 建议：`0x9fe001e0` 是 UART 寄存器地址，不是 RAM。启用正式 console 后，
  `pre_console_putc()` 会把早期字符串写到该地址，可能改写 UART 寄存器并导致
  串口乱码。重新执行 `make la32rsoc_defconfig` 后构建；串口仍按
  `115200 8N1` 使用。

## 变更项：忽略 AGENTS.md

- 修改：`.gitignore` 新增 `/AGENTS.md`，用于忽略仓库根目录的 agent 本地说明文件。
- 建议：已执行 `git rm --cached AGENTS.md`，本地文件保留但会在下次提交中从版本库移除；
  提交前可用 `git status --short AGENTS.md .gitignore note.md` 确认状态。

## 变更项：增加 dmfe PHY MDIO 只读诊断

- 修改：`drivers/net/dmfe.c` 在开启 `dmfe_debug` 时，通过 CSR9 执行 Clause 22
  MDIO 只读查询，输出 PHY ID、BMCR、BMSR、ANAR、LPA 和 DSCSR。
- 建议：执行 `setenv dmfe_debug 1` 后运行 `ping $serverip`。该诊断不写入 PHY
  寄存器，排查结束后建议
  `setenv dmfe_debug 0` 关闭串口日志。

## 变更项：扫描 dmfe PHY 地址

- 修改：`drivers/net/dmfe.c` 的 MDIO 诊断改为扫描地址 0 至 31，并选择第一个返回有效
  PHY ID 的地址读取状态寄存器；扫描到候选地址或读取后续 ID 失败时输出对应提示。
- 建议：若输出“PHY0-31 均未返回有效 ID”，地址配置不是唯一问题，应重点检查 FPGA
  的 MDC/MDIO 引脚约束、MDIO 双向缓冲和 PHY 复位信号。

## 变更项：修正 dmfe MDIO 读回转时序

- 修改：`drivers/net/dmfe.c` 移除 Clause 22 读事务中多发的一个 MDIO 高阻时钟；回转
  位改为直接采样 PHY 驱动的 TA=0，避免寄存器数据整体错移一位。
- 建议：重新构建烧写后运行 `setenv dmfe_debug 1; ping $serverip`。当前板卡的 DM9161
  位于 PHY 地址 1，正常 ID 为 `0181:b8a0`（修订号可能不同）。

## 变更项：扩展 dmfe 收发路径诊断

- 修改：`drivers/net/dmfe.c` 的 `dmfe_debug` 增加 CSR0/3-7、收发描述符、接收错误位、
  PHY RECR/DISCR 和首次无接收快照；`dmfe_debug=2` 额外输出收发帧前 64 字节及 CSR8
  计数器。
- 建议：优先执行 `setenv dmfe_debug 1; ping $serverip` 并保留完整日志；需要核对 ARP
  帧目的 MAC、源 MAC 或接收错误帧时使用 `setenv dmfe_debug 2`。CSR8 的读取会清除
  FIFO overflow/missed-frame 统计，仅在级别 2 启用。

## 变更项：增加 dmfe 软件最小帧填充对照

- 修改：`drivers/net/dmfe.c` 的 `dmfe_send()` 增加 `dmfe_sw_pad` 运行时开关。开关为 `1` 时，长度小于 60 字节的发送帧在 DMA 缓冲区尾部补零至 60 字节，并把描述符长度设为 60；默认关闭，保持原发送长度。调试日志额外输出原始长度、提交长度和填充长度。
- 建议：使用 `setenv dmfe_debug 2; setenv dmfe_sw_pad 0; ping $serverip` 保存基线日志，再执行 `setenv dmfe_sw_pad 1; ping $serverip` 进行对照。ARP 帧预期从 `input_len=42 tx_len=42 pad=0` 变为 `input_len=42 tx_len=60 pad=18`；验证结束后执行 `setenv dmfe_sw_pad 0`。

## 变更项：增加 dmfe PHY 数字环回诊断

- 修改：`drivers/net/dmfe.c` 增加 Clause 22 MDIO 写事务和 `dmfe_phy_loopback` 运行时开关；仅在该环境变量设置为 `0` 或 `1` 时清除或设置 PHY BMCR bit14，并回读确认。环境变量未设置时不写 PHY，正常初始化路径保持原行为。
- 建议：执行 `setenv dmfe_debug 2; setenv dmfe_phy_loopback 1; ping $serverip`，检查 RX 日志是否收到源 MAC 为 `00:98:76:64:32:19` 的本机 ARP 请求。该测试按本机发送帧能否从 PHY 环回接收判断，`ping` 本身无需成功；结束后执行 `setenv dmfe_phy_loopback 0; ping $serverip` 清除环回，或复位板卡。

## 变更项：增加 U-Boot DDR 文件加载与 CRC 校验工具

- 修改：新增 `tools/cpustcpu_load_file.py`，通过 U-Boot `loadx` 使用
  XMODEM-CRC 将任意文件加载到指定 DDR 地址，并用 U-Boot `crc32` 校验；支持按
  指定字节边界和填充值生成传输镜像，同时保存源文件及传输镜像的大小、SHA256 和
  CRC32。
- 建议：NAND 页写入测试使用 `--pad-to 2048 --pad-byte ff`，避免文件末尾不足一页
  时把 DDR 未初始化内容写入 NAND。所有握手、XMODEM 和 CRC 原始日志由
  `--log-dir` 指定目录保存。

## 变更项：从 NAND 自动加载并启动 vmlinux

- 修改：`configs/la32rsoc_defconfig` 的默认 `bootcmd` 改为从 NAND 偏移 0 读取
  `0xd0b000` 字节到 `0xa2000000`，再执行 `bootelf`；
  `include/configs/la32rsoc_demo.h` 删除覆盖正式 `CONFIG_BOOTCOMMAND` 的重复
  `bootcmd` 环境项。
- 原因：当前 vmlinux 的 ELF 目标段约为 `0xa0300000-0xa108c903`。旧加载地址
  `0xa0e00000` 位于目标段内，会在 `bootelf` 复制期间覆盖尚未读取的 ELF 内容；
  `0xa2000000` 与目标段不重叠。
- 当前固化镜像：源大小 `0xd0afe4`，按 2048 字节页使用 `0xff` 补齐到
  `0xd0b000`，NAND 擦除长度 `0xd20000`，页对齐镜像 CRC32 为 `9734dab3`。

## 变更项：增加以太网更新 vmlinux 的内置命令

- 修改（2026-07-15 13:52）：新增 U-Boot 命令 `vmlinux update [filename]`，默认通过
  TFTP 从 `serverip` 下载文件 `vmlinux` 到 `0xa2000000`；擦除前检查 ELF32
  LoongArch 格式、文件边界、加载地址和 16 MiB 容量上限，随后擦除 NAND 偏移 0
  的 16 MiB 逻辑区域、跳过坏块写入，并在 `0xa3000000` 独立读回后核对 CRC32。
  默认 `bootcmd` 同步改为读取 16 MiB 后执行 `bootelf`。
- 建议：TFTP 服务端的 `vmlinux` 必须先执行
  `/opt/loongarch-toolchain/bin/loongarch32r-linux-gnusf-strip`，保留 ELF 格式且大小不
  超过 16 MiB；确认 `serverip`、`ipaddr` 和 `netmask` 后执行 `vmlinux update`。
  命令会擦除 NAND 前部区域，下载或校验失败时不会开始擦除；坏块扩展范围限制为
  NAND 前 20 MiB。实机验证（2026-07-15 15:24）已使用 SPI 中的最终 U-Boot 完成
  TFTP 下载、16 MiB 逻辑区域擦除、写入和独立读回，结果为
  `source=0xd0afe4 written=0xd0b000 crc32=9734dab3`；冷复位后默认 `bootcmd`
  读取 `0x1000000` 字节成功，并进入 Linux shell。

## 变更项：临时关闭 VGA framebuffer 与 vidconsole

- 修改（2026-07-15 13:54）：在 `configs/la32rsoc_defconfig` 关闭
  `CONFIG_DM_VIDEO`、相关像素格式与 framebuffer console 支持以及
  `CONFIG_VIDEO_SIMPLE`；将 `arch/la32r/dts/la32rsoc_demo.dts` 的
  `simple-framebuffer` 节点移入现有 `#if 0` 不编译区段。
- 建议：执行 `make la32rsoc_defconfig` 后重新构建；关闭期间 U-Boot 仅使用串口
  控制台，不注册 `vidconsole`，也不写入 VGA framebuffer。

## 变更项：修正 LS1A NAND DMA 描述符对齐

- 修改（2026-07-15 14:52）：`drivers/mtd/nand/raw/ls1a_nand.c` 将 28 字节 DMA
  描述符的分配对齐从 16 字节提高到 32 字节，避免关闭 DM video 后堆分配顺序变化
  使描述符落在非 32 字节边界，导致所有 NAND 页 DMA 读取超时 `-110`。
- 建议：构建后先从 DDR 运行 U-Boot 并执行单页只读测试
  `nand read.raw 0xa4000000 0 1`；确认无超时后再使用 `vmlinux update`。32 字节对齐
  诊断镜像已完成 16 MiB NAND 读取并进入 Linux。

## 变更项：关闭未使用的 MMC/SD 支持

- 修改（2026-07-15 14:31）：在 `configs/la32rsoc_defconfig` 关闭 MMC 核心、
  DM MMC、LSMMC 控制器驱动、`mmc`/`mmc swrite` 命令及其 sparse image 库；
  FAT、EXT4 和通用块设备支持保持不变。
- 建议：执行 `make la32rsoc_defconfig` 后重新构建；当前设备树没有 MMC 节点，
  关闭后不再编译 `drivers/mmc/ls_mmc.c`，U-Boot 中也不再提供 `mmc` 命令。

## 变更项：修改 git 上游仓库地址

- 修改（2026-07-21 13:04）：将 `origin` 远程地址从
  `git@github.com:shaabby/myuboot.git` 改为
  `git@github.com:CPUSTCPU/CPUSTCPUboot.git`（仅本地 `.git/config`，不影响源码）。
- 建议：推送前确认本机 SSH key 对 `CPUSTCPU/CPUSTCPUboot` 有写权限，可用
  `ssh -T git@github.com` 验证。

## 变更项：增加 vmlinux 更新等待提示

- 修改（2026-07-26 12:33）：`cmd/vmlinux.c` 在 NAND 擦除成功后提示正在执行写入和
  回读校验，并明确出现 `vmlinux: update complete` 前不要复位。
- 建议：执行 `vmlinux update [filename]` 后等待最终成功提示和 U-Boot 命令提示符，
  再执行复位或断电。

## 变更项：扩充 vmlinux NAND 容量

- 修改（2026-07-30 19:41）：`cmd/vmlinux.c` 将 `vmlinux update` 的镜像容量和擦除
  长度从 16 MiB 扩充到 32 MiB，将坏块扩展范围限制为 NAND 前 40 MiB，并将独立
  回读缓冲区移至 `0xa4000000`；`configs/la32rsoc_defconfig` 的默认 `bootcmd` 同步
  改为从 NAND 读取 32 MiB。
- 建议：重新构建并烧写 U-Boot 后执行 `vmlinux update [filename]`；镜像需保持为
  ELF32 LoongArch 格式且不超过 32 MiB，等待回读 CRC32 校验完成后再复位。默认启动
  会固定读取 32 MiB，NAND 前 40 MiB 内可用于跳过坏块。

## 变更项：修正 SDRAM 容量注释

- 修改（2026-07-30 19:44）：`include/configs/la32rsoc_demo.h` 将
  `CONFIG_SYS_SDRAM_SIZE=0x08000000` 的容量注释从 256 MiB 修正为 128 MiB，配置数值
  保持不变。
- 建议：无需额外操作。

## 变更项：增加 LS1A NAND DMA 与缓存只读诊断

- 修改（2026-08-03 16:18）：`drivers/mtd/nand/raw/ls1a_nand.c` 增加 `nanddiag`
  命令和超时阶段快照，区分 `DMA_START_TIMEOUT` 与 `NAND_DONE_TIMEOUT`；运行时可对照
  `baseline`、`dbar`、显式 L1+L2 `cacop` 和 uncached alias，并一次测试 OOB/full 读取
  及页 0、1、63、64、65。`configs/la32rsoc_defconfig` 默认启用该只读命令；
  `tools/cpustcpu_test_uboot_ram.py` 增加可重复的 `--command` 参数，自动中断候选镜像的
  autoboot、连续执行诊断并分别保存串口日志。
- 建议：通过 `tools/cpustcpu_test_uboot_ram.py --image u-boot.bin --serial <串口> \
  --log-dir <目录> --command "nanddiag all 1" --command "nanddiag mode l2" \
  --command "nand read.raw 0xa4000000 0 1"` 完成一次 DDR 临时启动、诊断矩阵和通用
  NAND 路径对照。若
  `baseline`/`dbar` 失败而 `l2` 成功，说明
  CPUSTCore 的 L1 写回只到 L2，NAND DMA 读取 DDR 旧描述符。全部诊断只发
  `READ`/`RESET`，不会擦除或写入 NAND；该诊断提交当时保持默认 `baseline`，实测
  确认 L2 路径后再由后续变更切换正常默认模式。

## 变更项：对齐 CPUSTCore 两级 Cache 维护语义

- 修改（2026-08-03 18:18）：`arch/la32r/lib/cache.c` 将数据写回路径改为 L1
  `cacop 0x11`、L2 `cacop 0x12`、`dbar 0`，并在通用 `flush_cache()` 末尾显式保留
  `ibar 0`；DMA 回收的 `invalidate_dcache_range()` 仅维护 L1 后执行 `dbar 0`，避免
  L2 写回覆盖设备写入的新数据。`arch/la32r/include/asm/io.h` 同步将
  `sync()`/`mmiowb()` 修正为数据屏障 `dbar 0`。
- 建议：构建后检查 `flush_cache()` 指令顺序为 L1、L2、`dbar`、`ibar`；该公共修改
  会影响 `bootelf`、重定位和所有使用 LA32R Cache API 的非一致 DMA 驱动，仍需在
  CPUSTCore 板上验证完整启动与外设 DMA。

## 变更项：统一 CPUSTCore Cache line 与 NAND DMA 对齐

- 修改（2026-08-03 18:18）：`arch/la32r/Kconfig` 和 `configs/la32rsoc_defconfig`
  将 la32rsoc 的 I/D Cache line 改为 64 字节、`CONFIG_LA32R_L1_CACHE_SHIFT` 改为
  6，使 `ARCH_DMA_MINALIGN` 为 64；
  `drivers/mtd/nand/raw/ls1a_nand.c` 的 buffer 和描述符均按 64 字节分配，正常 NAND
  DMA 默认使用 L1+L2 模式。诊断命令继续保留独立的
  `baseline`、`dbar`、`l2`、`uncached` 分级维护路径，DMA 读完成后只维护 L1。
- 建议：使用 DDR 临时加载候选 U-Boot，一次执行 `nanddiag cache`、
  `nanddiag all 4`、`nanddiag status`、完整 32 MiB `nand read` 与 CRC 对照，再执行
  `bootelf`；确认上板通过前不要烧写持久存储。

## 变更项：避免异常打印读取未实现的 CSR3

- 修改（2026-08-03 20:29）：`arch/la32r/cpu/start.S` 从异常打印列表移除 CPUSTCore
  未实现的 CSR `0x3`，并将 `ESTAT`、`ERA`、`BADV` 提前打印，避免诊断路径触发二次
  `INE` 异常而覆盖最初异常现场。
- 建议：重新构建后检查反汇编中没有 `csrrd` 访问 `0x3`；上板时优先记录 CSR
  `0x5`、`0x6`、`0x7`，再依据重定位偏移将 `ERA` 映射回 `u-boot` ELF。

## 变更项：同步 NAND 8 KiB APB 窗口

- 修改（2026-08-06 18:20）：`arch/la32r/dts/la32rsoc_demo.dts` 的禁用 NAND 节点基址改为 `0x1fe0c000`，窗口缩小为 `0x2000`。
- 建议：该节点当前位于 `#if 0`，启用时必须与采用新地址表的 SoC bitstream 配套验证。

## 变更项：LS1A NAND 改用设备树资源

- 修改（2026-08-06 23:52）：LS1A NAND 改为 `UCLASS_MTD` 设备，控制器窗口和
  DMA order 寄存器通过 DTS 的 `nand`、`dma-order` 资源获取；启用物理地址
  `0x1fe0c000` 的 NAND 节点，删除 `CONFIG_SYS_NAND_BASE` 地址配置，并由控制器
  资源派生 DMA 数据端口 `0x1fe0c040`。
- 建议：使用 `make la32rsoc_defconfig` 和完整 LA32R 交叉构建验证；板测必须配套采用
  新 APB 地址表的 SoC bitstream，先确认启动识别 128 MiB NAND，再执行只读页测试。
- 验证（2026-08-06 23:58）：`make la32rsoc_defconfig`、LS1A NAND 单文件编译和完整
  LA32R 交叉构建通过，`git diff --check` 与 checkpatch 通过；生成 DTB 中两个资源为
  `0x1fe0c000/0x2000`、`0x1fd01160/0x4`，`u-boot.bin` SHA-256 为
  `013274b71d572621a2a95df35d1e7f2475cbbc075e6e24552d1a041d8744f41a`。本轮未板测。

## 变更项：删除 CORETEST 与 CPUSTRESS

- 修改（2026-08-07 00:34）：删除内建 `coretest`、`cpustress` 命令及 LA32R 汇编辅助，
  删除 standalone `coretest_app`、运行脚本和构建入口；同步移除 Kconfig、Makefile、
  defconfig、发布脚本及 XMODEM 分包中的相关内容，并清理旧调试记录与本地生成物。
- 建议：使用 `make la32rsoc_defconfig` 和完整 LA32R 交叉构建验证；源码、最终配置和
  U-Boot ELF 中不应再出现 CORETEST/CPUSTRESS 功能或符号。
- 验证（2026-08-07 00:36）：defconfig 与完整 LA32R 交叉构建通过，`git diff --check`
  通过；源码树、`.config`、U-Boot ELF、`u-boot.bin` 和文件名检索均无功能残留。
  `u-boot.bin` 为 401016 字节，SHA-256 为
  `56b5ce784cc3a337c8826d5e9e7497a47eded541f0a25c7fe89579bbdf580a71`。

## 变更项：CPU 周期计时频率构建参数

- 修改（2026-08-08）：LA32R 使用统一的 `CPUSTC_CPU_FREQ_HZ` 构建参数；当前
  `la32rsoc` 默认 50000000 Hz，历史 `la32rmegasoc` 默认 33000000 Hz。
  `get_tbclk()` 恢复返回 Hz，延时和毫秒计时按 64 位 CPU 周期换算，并避免读取
  计数器低 32 位翻转时组合出错误值；通用 `timer_get_us()` 也通过强 `get_ticks()`
  读取同一个 64 位周期计数器。
- 建议：构建与目标 bitstream 不同频率的镜像时显式传入参数，例如
  `make -j8 CPUSTC_CPU_FREQ_HZ=60000000`；`CONFIG_SYS_NS16550_CLK` 继续描述
  33 MHz 外设时钟，不随 CPU 参数修改。
- 验证（2026-08-08）：50 MHz 完整交叉构建通过，60 MHz 覆盖值可触发
  `time.o` 重新编译并已恢复为 50 MHz；最终 `u-boot.bin` 为 401092 字节，
  SHA-256 为 `fbcfff437974fea0a09119bc63a35a5c20549f43711a2f354d39c1ea6e76c5ea`。

## 变更项：扩充 vmlinux NAND 容量至 40 MiB

- 修改（2026-08-09）：`cmd/vmlinux.c` 将 `vmlinux update` 的镜像容量和擦除长度
  扩充至 40 MiB，坏块扩展范围限制为 NAND 前 48 MiB；TFTP/启动缓冲区移至
  `0xa3000000..0xa5800000`，避开当前 vmlinux 的
  `0xa0300000..0xa203ba14` 加载区。写入完成后复用 TFTP 缓冲区回读并校验 CRC32，
  避免保留额外的全尺寸回读缓冲区；默认 `bootcmd` 同步读取 40 MiB。
- 建议：重新构建并先以 DDR 临时方式验证 U-Boot；确认 `vmlinux update` 接受当前
  31,843,996 字节镜像、NAND 写入和回读 CRC32 均成功且出现
  `vmlinux: update complete` 后，才能复位验证默认 NAND 启动。
- 验证（2026-08-09）：未清理输出目录的 LA32R 增量交叉构建通过，最终
  `u-boot.bin` 为 401124 字节，SHA-256 为
  `5fc102d40ce90a48c2c9e50fc4a34fb9cf2701a839843d3bd8be48f52fbb742e`；当前
  vmlinux 的 `PT_LOAD` 内存终点为 `0xa203ba14`，与新缓冲区无重叠，镜像距
  40 MiB 上限还有 10,099,044 字节。本轮尚未执行 U-Boot DDR 临时启动或 NAND
  持久写入。
