#include "AgeMotionDriver.h"
#include <QCoreApplication>
#include <QDir>

AgeMotionDriver::AgeMotionDriver() : m_isConnected(false)
{
}

AgeMotionDriver::~AgeMotionDriver()
{
    if (m_lib.isLoaded()) {
        m_lib.unload();
    }
}

QString AgeMotionDriver::getLastError() const
{
    return m_lastError;
}

bool AgeMotionDriver::connectDevice()
{
    // 1. 加载 DLL
    if (!loadLibrary()) {
        return false;
    }

    // 2. 执行授权
    if (!authorize()) {
        m_lastError = "License authorization failed.";
        return false;
    }

    // 3. 建立连接 (AutoConnect = 1)
    if (m_api_isValid && !m_api_isValid(1)) {
        m_lastError = "Failed to connect to USB Device (AgeCOMIsValid returned FALSE).";
        return false;
    }

    m_isConnected = true;
    qDebug() << "✅ AgeMotionDriver: Device connected successfully.";
    return true;
}

bool AgeMotionDriver::loadLibrary()
{
    // 使用头文件中定义的 DLL_RELATIVE_PATH
    // QString dllPath = QCoreApplication::applicationDirPath() + DLL_RELATIVE_PATH;
    // m_lib.setFileName(dllPath);

    QString dllPath = DLL_ABS_PATH;
    m_lib.setFileName(dllPath);

    if (!m_lib.load()) {
        m_lastError = "Failed to load DLL at: " + dllPath + "\nError: " + m_lib.errorString();
        qCritical() << m_lastError;
        return false;
    }

    // 解析函数指针
    m_api_isValid   = (AgeCOMIsValidFunc)m_lib.resolve("AgeCOMIsValid");
    m_api_getUSBID  = (AgeCOMGetUSBIDFunc)m_lib.resolve("AgeCOMGetUSBID");
    m_api_readQWORD = (AgeCOMReadQWORDFunc)m_lib.resolve("AgeCOMReadQWORD");
    m_api_getCOMID  = (AgeCOMGetCOMIDFunc)m_lib.resolve("AgeCOMGetCOMID");
    m_api_setSerial = (AgeCOMSerialFunc)m_lib.resolve("AgeCOMSerial");
    
    // 新增函数指针解析
    m_api_readWORD   = (AgeCOMReadWORDFunc)m_lib.resolve("AgeCOMReadWORD");
    m_api_writeWORD  = (AgeCOMWriteWORDFunc)m_lib.resolve("AgeCOMWriteWORD");
    m_api_writeQWORD = (AgeCOMWriteQWORDFunc)m_lib.resolve("AgeCOMWriteQWORD");

    // 校验 (注意：根据实际情况，有些函数可能不是必须的，但为了完整性建议都校验)
    if (!m_api_isValid || !m_api_readQWORD || !m_api_setSerial || 
        !m_api_readWORD || !m_api_writeWORD || !m_api_writeQWORD) {
        m_lastError = "Failed to resolve one or more functions from DLL.";
        qCritical() << m_lastError;
        return false;
    }

    return true;
}

bool AgeMotionDriver::authorize()
{
    if (!m_api_setSerial) return false;

    // 使用头文件中定义的 LICENSE_KEY
    DWORD keyLength = (DWORD)strlen(LICENSE_KEY);

    // 调用授权接口
    return m_api_setSerial((BYTE*)LICENSE_KEY, keyLength);
}

bool AgeMotionDriver::getPosition(double &positionUm)
{
    if (!m_isConnected || !m_api_readQWORD) {
        m_lastError = "Driver not connected or function pointer invalid.";
        return false;
    }

    QWORD rawPos = 0;

    // 使用头文件定义的常量: STATION_ID, REG_POSITION_ADDR, TIMEOUT_MS
    if (m_api_readQWORD(STATION_ID, AgeReg::ADDR_POS_REAL, rawPos, TIMEOUT_MS)) {

        // 1. 转为有符号数 (处理负方向)
        long long signedPulses = (long long)rawPos;

        // 2. 转换为微米
        positionUm = (double)signedPulses / (PULSES_PER_UM);

        return true;
    } else {
        m_lastError = "Failed to read QWORD from device.";
        return false;
    }
}

// --- 获取实时速度 (RPM) ---
bool AgeMotionDriver::getVelocity(double &rpm)
{
    if (!m_isConnected || !m_api_readWORD) return false;

    WORD rawVel = 0;
    // 读取实时速度寄存器 0x0045
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_VEL_REAL, rawVel, TIMEOUT_MS)) {
        // 转换公式: RPM = (5 * raw) / 16 (根据手册 4.4.26)
        short signedVel = (short)rawVel; // 转为有符号
        rpm = (signedVel * 5.0) / 16.0;
        return true;
    }
    return false;
}

// --- 设置目标运行速度 (RPM) ---
bool AgeMotionDriver::setTargetVelocity(double rpm)
{
    if (!m_isConnected || !m_api_writeWORD) return false;

    // 转换公式: VelSet = (16 * RPM) / 5 (根据手册 4.4.21)
    // 注意: VelSet 值域 1~38400
    unsigned short val = (unsigned short)((rpm * 16.0) / 5.0);

    // 写入速度设定寄存器 0x0040
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_VEL_SET, val, TIMEOUT_MS);
}

// --- 绝对运动到指定位置 (微米) ---
bool AgeMotionDriver::moveToPosition(double positionUm)
{
    if (!m_isConnected || !m_api_writeQWORD) return false;

    // 1. 将微米转换为脉冲/微步 (MMS)
    // 1 um = PULSES_PER_UM pulses
    long long mms = (long long)(positionUm * PULSES_PER_UM);

    // 2. 写入目标位置寄存器 0x0024
    // 注意: 类型是 INT64 (QWORD)
    return m_api_writeQWORD(STATION_ID, AgeReg::ADDR_POS_TARGET, (QWORD)mms, TIMEOUT_MS);
}

// --- 停止运动 ---
bool AgeMotionDriver::stopMotion()
{
    if (!m_isConnected || !m_api_writeWORD) return false;

    // 写入控制寄存器 0x0000
    // 根据手册 4.4.1 [cite: 2430]，Bit 12 是 Stop (停止)
    // 0x1000 = 0001 0000 0000 0000 (二进制)
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_CONTROL, 0x1000, TIMEOUT_MS);
}

// --- 获取故障码 ---
int AgeMotionDriver::checkError()
{
    if (!m_isConnected || !m_api_readWORD) return -1;

    WORD errCode = 0;
    // 读取故障寄存器 0x0002 [cite: 2504]
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_ERROR_CODE, errCode, TIMEOUT_MS)) {
        return (int)errCode; // 0 表示无故障
    }
    return -1; // 通讯失败
}
