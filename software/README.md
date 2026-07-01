# SuperCap
超电软件子项目，源于 港科ENTERPRIZE 24年开源

## 通讯

本模块通过 **FDCAN1 以 Classic CAN 方式** 与外部控制板通讯，当前代码中使用 **标准帧、8 字节数据段**。

- **发送 ID**：`0x051`
- **接收 ID**：`0x061`
- **CAN 外设**：`FDCAN1`
- **接收 FIFO**：`RX FIFO0`
- **过滤策略**：仅接收 `0x061`，其余标准帧/扩展帧均拒收
- **发送周期**： `5 ms` 一次（在 `TIM2_IRQHandler` 中以 1 kHz 基准 5 分频发送）

### 通讯背景与功率定义

本模块是一个**双向三相四开关 Buck-Boost 超级电容充放电模块**，并联在**底盘**与**裁判系统供电口**之间。

- 靠近底盘/裁判系统的一侧定义为 **A 侧**
  - 电压：`Vaside`
  - 电流：`Iaside`
- 超级电容一侧定义为 **B 侧**
  - 电压：`Vbside`
- 三相电感电流分别为：`Ialpha`、`Ibeta`、`Igamma`
- 系统总输入电流既裁判chassis输出口电流定义为： `IRefree`
- 底盘实际使用电流定义为：`IChassis`
- 系统总输入电流既裁判chassis输出口功率定义为： `pRefree` & `refreePower`
- 底盘实际使用功率定义为：`pChassis` & `chassisPower`

按当前控制实现中的约定：

- `refereePower ≈ Vaside × Iref`
- `paside ≈ Vaside × Iaside`
- `chassisPower = refereePower - paside`

可理解为：

- `refereePower`：裁判系统供电口输出的总功率
- `paside`：超级电容模块在 A 侧实际吸收/释放的功率
- `chassisPower`：底盘本体消耗功率

因此有：

```text
refereePower = chassisPower + paside
```
### 发送帧：模块 -> 外部控制板（ID = 0x051）

发送数据结构定义如下：

```c
typedef struct TxData
{
    uint8_t PowerLimit;
    uint16_t chassisPower;
    uint16_t refereePower;
    uint16_t SuperCapOutputMx;
    uint8_t OutPutCapability;
} __attribute__((packed)) Txdata;
```

总长度为 **8 字节**，按当前 STM32 平台实现，直接以内存打包发送。

#### 字段说明

| 字段 | 字节数 | 含义 | 来源 |
|---|---:|---|---|
| `PowerLimit` | 1 | 超电认为的功率限制 | `conn_->PowerLimit_` |
| `chassisPower` | 2 | 底盘功率 | `conn_->chassisPower_` |
| `refereePower` | 2 | 裁判系统输出总功率 | `conn_->refereePower_` |
| `SuperCapOutputMx` | 2 | 超级电容可向 A 侧输出的最大功率 | `conn_->SuperCapOutputMx_` |
| `OutPutCapability` | 1 | 输出能力百分比 | `当前允许最大功率/理论最大功率上限` |

#### 字节布局

按 `packed` 结构体顺序，发送 8 字节布局为：

| Byte0 | Byte1-2 | Byte3-4 | Byte5-6 | Byte7 |
|---|---|---|---|---|
| `errcode` | `chassisPower` | `refereePower` | `SuperCapOutputMx` | `OutPutCapability` |

> 注意：当前固件通过 `uint8_t*` 直接发送结构体内存，因此多字节字段字节序与 MCU 本地存储一致。

#### 发送字段的物理意义

1. **powerlimit**

   超电认为的当前底盘（refree `chassis` output）限制功率功率
  
2. **chassisPower**

   由控制器实时计算：

   ```text
   chassisPower = refereePower - Vaside × Iaside
   ```

   即估计的底盘实际功率。

3. **refereePower**

   由采样量计算：

   ```text
   refereePower = Vaside × Iref
   ```

4. **SuperCapOutputMx**

   表示当前超级电容侧在约束条件下，**最大可向 A 侧释放的功率上限**，即底盘可用最大功率等于**SuperCapOutputMx + refreepowerMX(裁判系统限制的底盘功率)**。

5. **OutPutCapability**

   表示当前可输出功率占总理论最大可输出功率的百分比（paside），**非电容容量**

> 注意：因为超级电容功率控制逻辑原因，超级电容容量不可作为超级电容输出能力的衡量因素，目前因为理论上限在实际使用中不可能达到，此百分比最大值约为65%左右，正在寻找方案使此数值归一化至0-100%（0-255）

### 接收帧：外部控制板 -> 模块（ID = 0x061）

接收数据结构定义如下：

```c
typedef struct RxData
{
    uint8_t enableCONV : 1;
    uint8_t resv : 1;
    uint8_t resv0 : 1;
    uint8_t resv1 : 1;
    uint8_t resv2 : 1;
    uint8_t resv3 : 1;
    uint16_t refereePowerLimit;
    uint16_t resv4;
    uint8_t resv5;
    int16_t resv6;
} __attribute__((packed)) RxData;
```

同样总长度为 **8 字节**。

#### 当前实际使用字段

虽然结构体中定义了多个保留位/保留字段，但当前固件仅使用以下两项：

| 字段 | 含义 | 写入目标 |
|---|---|---|
| `enableCONV` | 变换器使能标志 | `supercap.status_.enableCONV_` |
| `refereePowerLimit` | 外部下发的裁判功率限制 | `supercap.status_.PowerLimit_` |

#### 接收字段说明

1. **enableCONV**

   位于第 1 字节最低位，用于表示是否允许功率变换器工作。

   - `0`：关闭/禁止
   - `1`：开启/允许

### 典型通讯时序

1. CAN过滤器仅放行 `0x061`。
2. 模块周期性发送 `0x051`，向外报告功率、输出能力和错误状态。
