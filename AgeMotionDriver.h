#ifndef AGEMOTIONDRIVER_H
#define AGEMOTIONDRIVER_H

#include <QLibrary>
#include <QString>
#include <QDebug>

// --- AgeCOM 类型定义 ---
typedef long BOOL32;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;

namespace AgeReg {

// --- 1. 控制与状态 (Control & Status) ---
static constexpr int ADDR_CONTROL         = 0x0000; // [WORD]  控制寄存器 (使能/暂停/急停等)
static constexpr int ADDR_ERROR_CODE      = 0x0002; // [WORD]  故障寄存器 (读取报警代码)
static constexpr int ADDR_INPUT_TYPE      = 0x0008; // [WORD]  输入类型 (脉冲方向/正反转等)

// --- 2. 电流设置 (Current) ---
// 注意：电流单位通常为 0.01A (例如 200 = 2.00A)
static constexpr int ADDR_CURRENT_MAX     = 0x0010; // [UINT16] 电流最大值 (只读)
static constexpr int ADDR_CURRENT_MIN     = 0x0011; // [UINT16] 电流最小值 (只读)
static constexpr int ADDR_CURRENT_SET     = 0x0012; // [UINT16] 电流设定值 (运行电流)
static constexpr int ADDR_CURRENT_LOW     = 0x0013; // [UINT16] 闲时降流百分比 (0-100)
static constexpr int ADDR_CURRENT_LOW_WT  = 0x0014; // [UINT16] 降流等待时间 (ms)
static constexpr int ADDR_CURRENT_REAL    = 0x0015; // [UINT16] 实时电流 (只读)
static constexpr int ADDR_VOLTAGE_BREAK   = 0x001E; // [UINT16] 刹车电压

// --- 3. 位置控制 (Position) ---
// Position 类型为 INT64 (4个WORD)，单位: MMS (微步)
static constexpr int ADDR_POS_REAL        = 0x0020; // [INT64] 电机实时位置 (0x0020-0x0023)
static constexpr int ADDR_POS_TARGET      = 0x0024; // [INT64] 电机目标位置 (0x0024-0x0027)

// 分辨率相关
static constexpr int ADDR_T_RESOLUTION    = 0x0028; // [UINT32] 单齿分辨率 (默认76800)
static constexpr int ADDR_PULSE_LENGTH    = 0x002A; // [UINT32] 脉冲步进长度

// 脉冲位置 (32位整数，方便通过脉冲数控制)
static constexpr int ADDR_PULSE_POS_REAL  = 0x002C; // [INT32] 脉冲实时位置
static constexpr int ADDR_PULSE_POS_SET   = 0x002E; // [INT32] 脉冲目标位置

// 误差控制 (伺服/闭环功能)
static constexpr int ADDR_POS_ERR_ALARM   = 0x0030; // [UINT32] 位差报错阈值
static constexpr int ADDR_POS_ERR_ALLOW   = 0x0032; // [UINT32] 到位允许误差
static constexpr int ADDR_TIME_ERR_ALLOW  = 0x0034; // [UINT16] 到位允许时间
static constexpr int ADDR_POS_ERROR       = 0x003A; // [INT64] 实时位置误差

// --- 4. 速度控制 (Velocity) ---
// 速度转换公式: RPM = (VelSet * KV * 60000) / (Resolution * T)
// 简化公式(常规): VelSet = (16 * RPM) / 5
static constexpr int ADDR_VEL_SET         = 0x0040; // [UINT16] 运行速度设定
static constexpr int ADDR_VEL_START       = 0x0041; // [UINT16] 启动速度
static constexpr int ADDR_VEL_FILTER      = 0x0042; // [UINT16] 速度滤波 (加减速平滑度)
static constexpr int ADDR_VEL_KV          = 0x0043; // [UINT16] 速度系数 (通常20)
static constexpr int ADDR_VEL_FILTER_COM  = 0x0044; // [UINT16] 通讯控制滤波
static constexpr int ADDR_VEL_REAL        = 0x0045; // [SHORT]  实时速度
static constexpr int ADDR_VEL_ZERO        = 0x0046; // [UINT16] 回零速度

// --- 5. 总线与端口 (Bus & Port) ---
static constexpr int ADDR_BUS_WDT         = 0x0060; // [UINT16] 看门狗定时
static constexpr int ADDR_BUS_ADDR        = 0x0061; // [UINT16] 总线站号 (1-247)
static constexpr int ADDR_BUS_BAUD        = 0x0063; // [UINT32] 波特率
static constexpr int ADDR_PORT_STATUS     = 0x0080; // [UINT16] IO端口状态
static constexpr int ADDR_INPUT_BAND      = 0x0091; // [UINT32] 输入带宽

// --- 6. 设备信息 (Info) ---
static constexpr int ADDR_CPU_TEMP        = 0x0300; // [INT16]  CPU温度
static constexpr int ADDR_MOTOR_SN0       = 0x2000; // [UINT64] 电机序列号0
static constexpr int ADDR_DRIVER_NAME     = 0x8000; // [String] 驱动器型号
}

class AgeMotionDriver
{
public:
    AgeMotionDriver();
    ~AgeMotionDriver();

    bool connectDevice();

    // 注意：这里现在的单位是 微米 (μm)
    bool getPosition(double &positionUm);
    
    // 新增功能函数
    bool getVelocity(double &rpm);
    bool setTargetVelocity(double rpm);
    bool moveToPosition(double positionUm);
    bool stopMotion();
    int checkError();

    QString getLastError() const;

private:
    // ==========================================
    //               驱动配置常量
    // ==========================================

    // 1. 物理参数
    // 定义：多少个脉冲等于 1 微米
    static constexpr double PULSES_PER_UM = 1600.0;

    // 2. 通信参数
    // 站号 (RTU Address)
    static constexpr int STATION_ID = 1;
    // 自动超时 (0 = Auto)
    static constexpr int TIMEOUT_MS = 0;

    // 3. 寄存器地址
    // 电机位置寄存器 (QWORD)
    static constexpr int REG_POSITION_ADDR = 0x0020;

    // 4. 路径与授权 (使用 constexpr char* 确保在头文件中定义且无链接错误)
    static constexpr const char* DLL_RELATIVE_PATH = "/AgeMotionForDriver/x64/AgeCOM.dll";
    static constexpr const char* DLL_ABS_PATH = "D:/Project/Git/Git/AutoFocus/AgeMotionForDriver/x64/AgeCOM.dll";
    static constexpr const char* LICENSE_KEY =
        "AgeMotion-20010203-00000000-0000-0000-0000-0000-0000-0000-0000-0000-"
        "0000-0000-0000-0000-0000-0000-0000-0000-0000-0000";

    // ==========================================

    // --- 内部成员 ---
    QLibrary m_lib;
    QString m_lastError;
    bool m_isConnected;

    // --- 函数指针定义 ---
    typedef BOOL32 (*AgeCOMIsValidFunc)(BOOL32);
    typedef BOOL32 (*AgeCOMGetUSBIDFunc)(BYTE*);
    typedef BOOL32 (*AgeCOMReadQWORDFunc)(BYTE, WORD, QWORD&, DWORD);
    typedef BOOL32 (*AgeCOMGetCOMIDFunc)(WORD&);
    typedef BOOL32 (*AgeCOMSerialFunc)(BYTE*, DWORD);
    // 定义函数指针类型
    typedef BOOL32 (*AgeCOMReadWORDFunc)(BYTE, WORD, WORD&, DWORD);
    typedef BOOL32 (*AgeCOMWriteWORDFunc)(BYTE, WORD, WORD, DWORD);
    typedef BOOL32 (*AgeCOMWriteQWORDFunc)(BYTE, WORD, QWORD, DWORD);

    // 成员变量
    AgeCOMReadWORDFunc  m_api_readWORD = nullptr;
    AgeCOMWriteWORDFunc m_api_writeWORD = nullptr;
    AgeCOMWriteQWORDFunc m_api_writeQWORD = nullptr;

    AgeCOMIsValidFunc   m_api_isValid = nullptr;
    AgeCOMGetUSBIDFunc  m_api_getUSBID = nullptr;
    AgeCOMReadQWORDFunc m_api_readQWORD = nullptr;
    AgeCOMGetCOMIDFunc  m_api_getCOMID = nullptr;
    AgeCOMSerialFunc    m_api_setSerial = nullptr;

    bool loadLibrary();
    bool authorize();
};

#endif // AGEMOTIONDRIVER_H
