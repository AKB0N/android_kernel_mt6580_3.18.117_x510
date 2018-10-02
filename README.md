This is 3.18.117 MT6580 kernel source ported to be used on INFINIX HOT2 x510.

## Known information
| Subsystem | Driver name | Availability | Working |
|-----------|-------------|--------------|---------|
| LCM driver #1 | `nt35521_dsi_vdo_yushun_cmi_hd720` | Yes | Yes |
| LCM driver #2 | `hx8394d_dsi_vdo_hlt_hsd_hd720` | Yes | Yes |
| Touch panel | `GT9XX (i2c 1-005D)` | Yes | Yes |
| Charge | `BQ24158 (i2c 1-006A)` | Yes | Yes |
| GPU | `Mali-400 MP` | Yes | Yes |
| Camera #1 | `ov8865_mipi_raw` | Yes | Yes |
| Camera #2 | `gc2755_mipi_raw` | Yes | No |
| Accelerometer | `BMA222 (i2c 2-0018)` | Yes | Yes |
| ALS/PS | `cm36283 (i2c 2-0060)` | Yes | No |
| Flash | `Samsung R821MB` | Yes | Yes |
| Lens #1 | `DW9714AF ` | Yes | Yes |
| Lens #2 | `FM50AF (i2c 0-000c)` | Yes | No |
| RAM | `2GB & 1GB  LPDDR3_1066` | - | Yes |
| Sound | `mtsndcard` | - | Yes |
| Accdet | `mt6580-accdet` | - | Yes |
| Other | `kd_camera_hw (i2c 0-0010)` | - | Yes |
| Other | `kd_camera_hw_bus2 (i2c 0-0036)` | - | Yes |


Bugs :

Alsps
Front camera
Upside Down back camera
LCM reset VOLT


## Acknowledgements

(in alphabetical order)

* Vasya Kovalev [Vgdn1942 (4pda.ru)](https://4pda.ru/forum/index.php?showuser=2214676) [(@Vgdn1942)](https://github.com/Vgdn1942)
* [aleha.druga (4pda.ru)](https://4pda.ru/forum/index.php?showuser=3708916) [(@aleha-druga)](https://github.com/aleha-druga)
* [kva3ar (4pda.ru)](https://4pda.ru/forum/index.php?showuser=6751930)
* [nik-kst (4pda.ru)](https://4pda.ru/forum/index.php?showuser=4052130) [(@nik124seleznev)](https://github.com/nik124seleznev)
* [Skyrimus (4pda.ru)](https://4pda.ru/forum/index.php?showuser=3927665) [(@Skyrimus)](https://github.com/Skyrimus)
* Ibrahim Fathelbab [ibrahim1973 (4pda.ru)](https://4pda.ru/forum/index.php?showuser=8068515) [(@ibrahim1973)](https://github.com/ibrahim1973)
* Yuvraj Saxena [Yuvraj (telegram)](https://t.me/imyuvraj)[(@rad0n)](https://github.com/rad0n)
