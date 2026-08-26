# CPUSTC 工具

[返回项目 README](../README.md)

## Linux 启动测量

工具：`tools/cpustcpu_measure_linux_boot.py`

依赖：`python3-serial`。

```sh
CPUSTC_BOOT_PASSWORD='<target-password>' \
python3 tools/cpustcpu_measure_linux_boot.py \
  --serial /dev/ttyUSB0 \
  --output-dir /tmp/cpustc-linux-boot-measurement
```

输出 JSON 报告、原始串口日志和分块索引。
