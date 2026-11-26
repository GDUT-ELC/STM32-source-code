# 12-gd25qdemo：GD25Q80C 外部 Flash 工程

## 工程说明

使用 STM32F407VET6 的 SPI1 读写 GD25Q80C SPI NOR Flash，提供初始化、保存、读取、擦除及状态查询接口。当前程序上电后在 `0x001000` 写入 `11 22 33 44`，读回比较，并在 OLED 上显示 `API TEST`、状态和 `DATA PASS/FAIL`。

## 使用方法

1. Flash 接线：SCK→PB3、MISO/SO→PB4、MOSI/SI→PB5、CS#→PA15；VCC 接 3.3V、VSS 接 GND，WP# 和 HOLD# 拉高。建议芯片电源旁放置 100nF 去耦电容。
2. OLED 的 SCK/MOSI/RES/DC/CS 分别接 PE9/PE10/PE11/PE12/PE13。
3. 用 Keil MDK 打开 `MDK-ARM/12-gd25qdemo.uvprojx`，编译、下载。正常应显示 `OK` 和 `DATA PASS`，Keil Watch 可查看 `g_flash_info`，期望芯片 ID 为 `0xC84014`。
4. 自己使用时包含 `app_flash.h`，在 GPIO、SPI 初始化后调用一次 `app_flash_init()`。成功后用 `app_flash_save(地址, 数据, 长度)` 保存、`app_flash_load(地址, 缓冲区, 长度)` 读取，逐次检查返回值是否为 `GD25Q_OK`。
5. 需要擦除时用 `app_flash_erase(地址)`，参数为目标扇区内的地址；用 `GD25Q_StatusString(返回值)` 查看错误短码。修改接线时同步修改 `Bsp/flash_config.h` 和底层 GPIO/SPI 配置。

## 使用注意

- **每次复位都会重新写入 0x001000 起的 4 字节。** 使用自己的存储布局前，应先修改或移除 `main.c` 的测试代码，测试扇区内不要放重要数据。`app_flash_self_test()` 另会改写 0x000000 起的 8 字节。
- GD25Q80C 容量为 8Mbit，即 1MiB；地址范围为 0x000000–0x0FFFFF。页大小为 256 字节，最小擦除单位为 4KiB，擦除后为 0xFF。
- NOR Flash 直接编程只能把 1 改成 0。`app_flash_save()` 会在需要时擦除并回写同扇区其他数据；直接页编程接口要求不跨页，并满足已擦除等条件。擦除函数会清空整个对应区域。
- 当前 SPI 为模式 0，时钟约 42MHz；飞线较长或通信不稳定时应降低速率。PA15、PB3、PB4 与 JTAG 引脚复用，下载调试使用 SWD，避免重新启用完整 JTAG 占用这些引脚。
- **当前独立驱动在 SR2.QE=1 时会返回 `QE MODE`。** 这是此版本的软件检查行为，QE=1 本身并不表示普通 SPI 指令失效；遇到此错误应核对驱动和芯片状态，可参考 11-Demo 的对应处理，不要直接清除未知保护位。
- 擦写过程断电可能损坏整个正在更新的扇区；重要参数应自行增加双份备份、序号和 CRC。避免在循环中反复擦写，以减少磨损。
- 驱动采用阻塞等待和静态缓冲，不要在中断中调用；多任务访问时加互斥保护。出现 `ID`、`SPI`、`PROTECTED`、`TIMEOUT` 时，分别检查型号、接线、电源及保护状态。
