## 7-10
```bash
u-boot@LoongsonSoC# setenv dmfe_debug 1
u-boot@LoongsonSoC# ping $serverip

dc21x4x_init, 379 iobase:9ff00000
MDIO PHY0: ID=0000:0000 BMCR=0000 BMSR=0000 link-down ANAR=0000 LPA=0000 DSCSR=0000
rx ring 87ff5be0
tx ring 87ff5b60
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff5be0
DE4X5_TRBA= 87ff5b60
DE4X5_STS= f0660004
DE4X5_OMR= 32602242

buf:87fefb94, des1:90000c0, status:80000000
new:0 ,status:0
TX error status2 = 0x00000000
After setup
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff5be0
DE4X5_TRBA= 87ff5b60
DE4X5_STS= f0660004
DE4X5_OMR= 32602242
Using ethernet@0x9ff00000 device
dmfe_send len=42 tx_len=42 desc=1 status=00008400
dmfe_send len=42 tx_len=42 desc=2 status=00008400
dmfe_send len=42 tx_len=42 desc=3 status=00008400
dmfe_send len=42 tx_len=42 desc=4 status=00008400

ARP Retry count exceeded; starting again
ping failed; host 169.254.150.45 is not alive
```

```bash
# ping $serverip

dc21x4x_init, 398 iobase:9ff00000
MDIO: 候选 PHY1，ID1=0181
MDIO PHY1: ID=0181:b8a0 BMCR=3100 BMSR=786d link-up ANAR=01e1 LPA=5de1 DSCSR=8218
rx ring 87fefba0
tx ring 87fefb20
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87fefba0
DE4X5_TRBA= 87fefb20
DE4X5_STS= f0660004
DE4X5_OMR= 32602242

buf:87fe9b3c, des1:90000c0, status:80000000
new:0 ,status:0
TX error status2 = 0x00000000
After setup
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87fefba0
DE4X5_TRBA= 87fefb20
DE4X5_STS= f0660004
DE4X5_OMR= 32602242
Using ethernet@0x9ff00000 device
dmfe_send len=42 tx_len=42 desc=1 status=00000000
dmfe_send len=42 tx_len=42 desc=2 status=00000000
dmfe_send len=42 tx_len=42 desc=3 status=00000000
dmfe_send len=42 tx_len=42 desc=4 status=00000000

ARP Retry count exceeded; starting again
ping failed; host 169.254.150.45 is not alive
u-boot@LoongsonSoC# 
```

```bash
# ping $serverip

dc21x4x_init, 506 iobase:9ff00000
MDIO: 候选 PHY1，ID1=0181
MDIO PHY1: ID=0181:b8a0 BMCR=3100 BMSR=786d link-up ANAR=01e1 LPA=5de1 DSCSR=8218
rx ring 87ff04e0
tx ring 87ff0460
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff04e0
DE4X5_TRBA= 87ff0460
DE4X5_STS= f0660004
DE4X5_OMR= 32602242

buf:87fea480, des1:90000c0, status:80000000
new:0 ,status:0
TX error status2 = 0x00000000
After setup
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff04e0
DE4X5_TRBA= 87ff0460
DE4X5_STS= f0660004
DE4X5_OMR= 32602242
dmfe after-setup: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660004 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=0 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
RX[0]: status=80000000 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=1
RX[1]: status=80000000 des1=000005f0 buf=87fedad0 next=87ff0500 own=1
RX[2]: status=80000000 des1=000005f0 buf=87fee0c0 next=87ff0510 own=1
RX[3]: status=80000000 des1=000005f0 buf=87fee6b0 next=87ff0520 own=1
RX[4]: status=80000000 des1=000005f0 buf=87feeca0 next=87ff0530 own=1
RX[5]: status=80000000 des1=000005f0 buf=87fef290 next=87ff0540 own=1
RX[6]: status=80000000 des1=000005f0 buf=87fef880 next=87ff0550 own=1
RX[7]: status=80000000 des1=020005f0 buf=87fefe70 next=87ff04e0 own=1
Using ethernet@0x9ff00000 device
dmfe_send len=42 tx_len=42 desc=1 status=00000000
TX[1]: status=00000000 des1=e100002a buf=87feab50 next=87ff0480 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660405 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
dmfe_recv: RX[0] 仍由 MAC 持有，尚未收到完整帧
dmfe rx-idle: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660405 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
MDIO PHY1: RECR=0 DISCR=0
RX[0]: status=80000000 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=1
RX[1]: status=80000000 des1=000005f0 buf=87fedad0 next=87ff0500 own=1
RX[2]: status=80000000 des1=000005f0 buf=87fee0c0 next=87ff0510 own=1
RX[3]: status=80000000 des1=000005f0 buf=87fee6b0 next=87ff0520 own=1
RX[4]: status=80000000 des1=000005f0 buf=87feeca0 next=87ff0530 own=1
RX[5]: status=80000000 des1=000005f0 buf=87fef290 next=87ff0540 own=1
RX[6]: status=80000000 des1=000005f0 buf=87fef880 next=87ff0550 own=1
RX[7]: status=80000000 des1=020005f0 buf=87fefe70 next=87ff04e0 own=1
RX[0]: status=00400720 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[1]: status=00400720 des1=000005f0 buf=87fedad0 next=87ff0500 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[2]: status=00400720 des1=000005f0 buf=87fee0c0 next=87ff0510 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[3]: status=00bb0720 des1=000005f0 buf=87fee6b0 next=87ff0520 own=0
  RX status: len=187 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status bb0720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[4]: status=00400720 des1=000005f0 buf=87feeca0 next=87ff0530 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[5]: status=00cf0720 des1=000005f0 buf=87fef290 next=87ff0540 own=0
  RX status: len=207 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status cf0720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
dmfe_send len=42 tx_len=42 desc=2 status=00000000
TX[2]: status=00000000 des1=e100002a buf=87feb140 next=87ff0490 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[6]: status=00400720 des1=000005f0 buf=87fef880 next=87ff0550 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[7]: status=00400720 des1=020005f0 buf=87fefe70 next=87ff04e0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[0]: status=00400720 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[1]: status=00400720 des1=000005f0 buf=87fedad0 next=87ff0500 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
dmfe_send len=42 tx_len=42 desc=3 status=00000000
TX[3]: status=00000000 des1=e100002a buf=87feb730 next=87ff04a0 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[2]: status=00400720 des1=000005f0 buf=87fee0c0 next=87ff0510 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[3]: status=00400720 des1=000005f0 buf=87fee6b0 next=87ff0520 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[4]: status=00400720 des1=000005f0 buf=87feeca0 next=87ff0530 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
dmfe_send len=42 tx_len=42 desc=4 status=00000000
TX[4]: status=00000000 des1=e100002a buf=87febd20 next=87ff04b0 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[5]: status=00400720 des1=000005f0 buf=87fef290 next=87ff0540 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[6]: status=01250720 des1=000005f0 buf=87fef880 next=87ff0550 own=0
  RX status: len=293 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 1250720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[7]: status=00400720 des1=020005f0 buf=87fefe70 next=87ff04e0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
RX[0]: status=004a0720 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=0
  RX status: len=74 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 4a0720
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0

ARP Retry count exceeded; starting again
ping failed; host 169.254.150.45 is not alive
```

```bash
# ping $serverip

dc21x4x_init, 506 iobase:9ff00000
MDIO: 候选 PHY1，ID1=0181
MDIO PHY1: ID=0181:b8a0 BMCR=3100 BMSR=786d link-up ANAR=01e1 LPA=5de1 DSCSR=8218
rx ring 87ff04e0
tx ring 87ff0460
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff04e0
DE4X5_TRBA= 87ff0460
DE4X5_STS= f0660004
DE4X5_OMR= 32602242

buf:87fea480, des1:90000c0, status:80000000
new:0 ,status:0
TX error status2 = 0x00000000
After setup
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff04e0
DE4X5_TRBA= 87ff0460
DE4X5_STS= f0660004
DE4X5_OMR= 32602242
dmfe after-setup: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660004 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=0 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[0]: status=80000000 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=1
RX[1]: status=80000000 des1=000005f0 buf=87fedad0 next=87ff0500 own=1
RX[2]: status=80000000 des1=000005f0 buf=87fee0c0 next=87ff0510 own=1
RX[3]: status=80000000 des1=000005f0 buf=87fee6b0 next=87ff0520 own=1
RX[4]: status=80000000 des1=000005f0 buf=87feeca0 next=87ff0530 own=1
RX[5]: status=80000000 des1=000005f0 buf=87fef290 next=87ff0540 own=1
RX[6]: status=80000000 des1=000005f0 buf=87fef880 next=87ff0550 own=1
RX[7]: status=80000000 des1=020005f0 buf=87fefe70 next=87ff04e0 own=1
Using ethernet@0x9ff00000 device
TX frame: 前 42/42 字节
00000000: ff ff ff ff ff ff 00 98 76 64 32 19 08 06 00 01    ........vd2.....
00000010: 08 00 06 04 00 01 00 98 76 64 32 19 a9 fe 96 2e    ........vd2.....
00000020: 00 00 00 00 00 00 a9 fe 96 2d                      .........-
dmfe_send len=42 tx_len=42 desc=1 status=00000000
TX[1]: status=00000000 des1=e100002a buf=87feab50 next=87ff0480 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660405 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
dmfe_recv: RX[0] 仍由 MAC 持有，尚未收到完整帧
dmfe rx-idle: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660405 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
MDIO PHY1: RECR=0 DISCR=0
RX[0]: status=80000000 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=1
RX[1]: status=80000000 des1=000005f0 buf=87fedad0 next=87ff0500 own=1
RX[2]: status=80000000 des1=000005f0 buf=87fee0c0 next=87ff0510 own=1
RX[3]: status=80000000 des1=000005f0 buf=87fee6b0 next=87ff0520 own=1
RX[4]: status=80000000 des1=000005f0 buf=87feeca0 next=87ff0530 own=1
RX[5]: status=80000000 des1=000005f0 buf=87fef290 next=87ff0540 own=1
RX[6]: status=80000000 des1=000005f0 buf=87fef880 next=87ff0550 own=1
RX[7]: status=80000000 des1=020005f0 buf=87fefe70 next=87ff04e0 own=1
RX[0]: status=00400720 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[1]: status=00400720 des1=000005f0 buf=87fedad0 next=87ff0500 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 00 45 00    .......%..K...E.
00000010: 00 24 42 8d 40 00 40 11 0e 12 a9 fe 96 2d a9 fe    .$B.@.@......-..
00000020: ff ff a3 e8 05 fe 00 10 cd 47 54 43 46 32 04 00    .........GTCF2..
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 40 b7 65 6f    ............@.eo
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[2]: status=00400720 des1=000005f0 buf=87fee0c0 next=87ff0510 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
TX frame: 前 42/42 字节
00000000: ff ff ff ff ff ff 00 98 76 64 32 19 08 06 00 01    ........vd2.....
00000010: 08 00 06 04 00 01 00 98 76 64 32 19 a9 fe 96 2e    ........vd2.....
00000020: 00 00 00 00 00 00 a9 fe 96 2d                      .........-
dmfe_send len=42 tx_len=42 desc=2 status=00000000
TX[2]: status=00000000 des1=e100002a buf=87feb140 next=87ff0490 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[3]: status=00400720 des1=000005f0 buf=87fee6b0 next=87ff0520 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
TX frame: 前 42/42 字节
00000000: ff ff ff ff ff ff 00 98 76 64 32 19 08 06 00 01    ........vd2.....
00000010: 08 00 06 04 00 01 00 98 76 64 32 19 a9 fe 96 2e    ........vd2.....
00000020: 00 00 00 00 00 00 a9 fe 96 2d                      .........-
dmfe_send len=42 tx_len=42 desc=3 status=00000000
TX[3]: status=00000000 des1=e100002a buf=87feb730 next=87ff04a0 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[4]: status=00400720 des1=000005f0 buf=87feeca0 next=87ff0530 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[5]: status=00400720 des1=000005f0 buf=87fef290 next=87ff0540 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 00 45 00    .......%..K...E.
00000010: 00 24 54 b7 40 00 40 11 fb e7 a9 fe 96 2d a9 fe    .$T.@.@......-..
00000020: ff ff 96 23 05 fe 00 10 db 0c 54 43 46 32 04 00    ...#......TCF2..
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 c5 3b 8d c1    .............;..
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[6]: status=00400720 des1=000005f0 buf=87fef880 next=87ff0550 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[7]: status=00400720 des1=020005f0 buf=87fefe70 next=87ff04e0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
TX frame: 前 42/42 字节
00000000: ff ff ff ff ff ff 00 98 76 64 32 19 08 06 00 01    ........vd2.....
00000010: 08 00 06 04 00 01 00 98 76 64 32 19 a9 fe 96 2e    ........vd2.....
00000020: 00 00 00 00 00 00 a9 fe 96 2d                      .........-
dmfe_send len=42 tx_len=42 desc=4 status=00000000
TX[4]: status=00000000 des1=e100002a buf=87febd20 next=87ff04b0 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[0]: status=00400720 des1=000005f0 buf=87fed4e0 next=87ff04f0 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[1]: status=00400720 des1=000005f0 buf=87fedad0 next=87ff0500 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 00 45 00    .......%..K...E.
00000010: 00 24 57 54 40 00 40 11 f9 4a a9 fe 96 2d a9 fe    .$WT@.@..J...-..
00000020: ff ff 05 fe 05 fe 00 10 6b 32 54 43 46 32 04 00    ........k2TCF2..
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 64 eb 4b 52    ............d.KR
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[2]: status=00400720 des1=000005f0 buf=87fee0c0 next=87ff0510 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[3]: status=00400720 des1=000005f0 buf=87fee6b0 next=87ff0520 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=1 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400720
RX frame: 前 64/64 字节
00000000: ff ff ff ff ff ff b0 25 aa 90 4b d0 08 06 00 01    .......%..K.....
00000010: 08 00 06 04 00 01 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 00 00 00 00 00 a9 fe 96 ff 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 ca d6 d4 c4    ................
dmfe rx-ok: CSR0=fe000000 CSR3=87ff04e0 CSR4=87ff0460 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)

ARP Retry count exceeded; starting again
ping failed; host 169.254.150.45 is not alive
```

## 偶然正常了
```bash
u-boot@LoongsonSoC# ping $serverip

dc21x4x_init, 506 iobase:9ff00000
MDIO: 候选 PHY1，ID1=0181
MDIO PHY1: ID=0181:b8a0 BMCR=3100 BMSR=786d link-up ANAR=01e1 LPA=5de1 DSCSR=8218
rx ring 87ff0540
tx ring 87ff04c0
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff0540
DE4X5_TRBA= 87ff04c0
DE4X5_STS= f0660004
DE4X5_OMR= 32602242

buf:87fea4e0, des1:90000c0, status:80000000
new:0 ,status:0
TX error status2 = 0x00000000
After setup
DE4X5_BMR= fe000000
DE4X5_TPD= 0
DE4X5_RRBA= 87ff0540
DE4X5_TRBA= 87ff04c0
DE4X5_STS= f0660004
DE4X5_OMR= 32602242
dmfe after-setup: CSR0=fe000000 CSR3=87ff0540 CSR4=87ff04c0 CSR5=f0660004 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=0 TPS=0 TU=1 UNF=0 RI=0 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[0]: status=80000000 des1=000005f0 buf=87fed540 next=87ff0550 own=1
RX[1]: status=80000000 des1=000005f0 buf=87fedb30 next=87ff0560 own=1
RX[2]: status=80000000 des1=000005f0 buf=87fee120 next=87ff0570 own=1
RX[3]: status=80000000 des1=000005f0 buf=87fee710 next=87ff0580 own=1
RX[4]: status=80000000 des1=000005f0 buf=87feed00 next=87ff0590 own=1
RX[5]: status=80000000 des1=000005f0 buf=87fef2f0 next=87ff05a0 own=1
RX[6]: status=80000000 des1=000005f0 buf=87fef8e0 next=87ff05b0 own=1
RX[7]: status=80000000 des1=020005f0 buf=87fefed0 next=87ff0540 own=1
Using ethernet@0x9ff00000 device
TX frame: 前 42/42 字节
00000000: ff ff ff ff ff ff 00 98 76 64 32 19 08 06 00 01    ........vd2.....
00000010: 08 00 06 04 00 01 00 98 76 64 32 19 a9 fe 96 2e    ........vd2.....
00000020: 00 00 00 00 00 00 a9 fe 96 2d                      .........-
dmfe_send input_len=42 tx_len=42 sw_pad=0 pad=0 desc=1 status=00000000
TX[1]: status=00000000 des1=e100002a buf=87feabb0 next=87ff04e0 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff0540 CSR4=87ff04c0 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[0]: status=00400320 des1=000005f0 buf=87fed540 next=87ff0550 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=0 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400320
RX frame: 前 64/64 字节
00000000: 00 98 76 64 32 19 b0 25 aa 90 4b d0 08 06 00 01    ..vd2..%..K.....
00000010: 08 00 06 04 00 02 b0 25 aa 90 4b d0 a9 fe 96 2d    .......%..K....-
00000020: 00 98 76 64 32 19 a9 fe 96 2e 00 00 00 00 00 00    ..vd2...........
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 2d eb 66 84    ............-.f.
dmfe rx-ok: CSR0=fe000000 CSR3=87ff0540 CSR4=87ff04c0 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
TX frame: 前 42/42 字节
00000000: b0 25 aa 90 4b d0 00 98 76 64 32 19 08 00 45 00    .%..K...vd2...E.
00000010: 00 1c 00 00 40 00 ff 01 fb 87 a9 fe 96 2e a9 fe    ....@...........
00000020: 96 2d 08 00 f7 ff 00 00 00 00                      .-........
dmfe_send input_len=42 tx_len=42 sw_pad=0 pad=0 desc=2 status=00000000
TX[2]: status=00000000 des1=e100002a buf=87feb1a0 next=87ff04f0 own=0
dmfe after-tx: CSR0=fe000000 CSR3=87ff0540 CSR4=87ff04c0 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
RX[1]: status=00400320 des1=000005f0 buf=87fedb30 next=87ff0560 own=0
  RX status: len=64 ES=0 FF=0 DE=0 RF=0 MF=0 FS=1 LS=1 TL=0 CS=0 FT=1 RE=0 DB=0 CE=0 OF=0
received a packet status 400320
RX frame: 前 64/64 字节
00000000: 00 98 76 64 32 19 b0 25 aa 90 4b d0 08 00 45 00    ..vd2..%..K...E.
00000010: 00 1c b3 5c 00 00 40 01 47 2c a9 fe 96 2d a9 fe    ...\..@.G,...-..
00000020: 96 2e 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 00 00 00 00 78 35 30 4c    ............x50L
dmfe rx-ok: CSR0=fe000000 CSR3=87ff0540 CSR4=87ff04c0 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
dmfe_recv: RX[2] 仍由 MAC 持有，尚未收到完整帧
dmfe rx-idle: CSR0=fe000000 CSR3=87ff0540 CSR4=87ff04c0 CSR5=f0660445 CSR6=32602242 CSR7=f3fe0000
  CSR5: TS=6 RS=3 TI=1 TPS=0 TU=1 UNF=0 RI=1 RU=0 RPS=0
  CSR8=e0000000 (读取会清除 FIFO overflow/missed-frame 计数)
MDIO PHY1: RECR=0 DISCR=0
RX[0]: status=80000000 des1=000005f0 buf=87fed540 next=87ff0550 own=1
RX[1]: status=80000000 des1=000005f0 buf=87fedb30 next=87ff0560 own=1
RX[2]: status=80000000 des1=000005f0 buf=87fee120 next=87ff0570 own=1
RX[3]: status=80000000 des1=000005f0 buf=87fee710 next=87ff0580 own=1
RX[4]: status=80000000 des1=000005f0 buf=87feed00 next=87ff0590 own=1
RX[5]: status=80000000 des1=000005f0 buf=87fef2f0 next=87ff05a0 own=1
RX[6]: status=80000000 des1=000005f0 buf=87fef8e0 next=87ff05b0 own=1
RX[7]: status=80000000 des1=020005f0 buf=87fefed0 next=87ff0540 own=1
host 169.254.150.45 is alive
```