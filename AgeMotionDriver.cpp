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

    // 读取并保存默认目标速度
    double vel = 0.0;
    if (getTargetVelocity(vel)) {
        m_defaultTargetVelocity = vel;
        qDebug() << "Default Target Velocity saved:" << m_defaultTargetVelocity << "um/s";
    }

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
    // 解析 32位读写函数
    m_api_readDWORD  = (AgeCOMReadDWORDFunc)m_lib.resolve("AgeCOMReadDWORD");
    m_api_writeDWORD = (AgeCOMWriteDWORDFunc)m_lib.resolve("AgeCOMWriteDWORD");

    // 校验 (注意：根据实际情况，有些函数可能不是必须的，但为了完整性建议都校验)
    if (!m_api_isValid || !m_api_readQWORD || !m_api_setSerial ||
        !m_api_readWORD || !m_api_writeWORD || !m_api_writeQWORD ||
        !m_api_readDWORD || !m_api_writeDWORD) {
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
        positionUm = (double)signedPulses / (MMS_PER_UM);

        return true;
    } else {
        m_lastError = "Failed to read QWORD from device.";
        return false;
    }
}

bool AgeMotionDriver::getTargetPosition(double &positionUm)
{
    if (!m_isConnected || !m_api_readQWORD) {
        m_lastError = "Driver not connected or function pointer invalid.";
        return false;
    }

    QWORD rawPos = 0;

    if (m_api_readQWORD(STATION_ID, AgeReg::ADDR_POS_TARGET, rawPos, TIMEOUT_MS)) {
        long long signedPulses = (long long)rawPos;
        positionUm = (double)signedPulses / (MMS_PER_UM);
        return true;
    } else {
        m_lastError = "Failed to read QWORD from device.";
        return false;
    }
}

// --- 获取目标速度 (RPM) ---
bool AgeMotionDriver::getTargetRPM(double &rpm)
{
    if (!m_isConnected || !m_api_readWORD) return false;

    WORD rawVel = 0;
    // 读取速度设定寄存器 0x0040
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_VEL_SET, rawVel, TIMEOUT_MS)) {
        // VelSet is UINT16
        rpm = (rawVel * KV_DEFAULT * 60000) / MMS_PER_R;
        return true;
    }
    return false;
}

// --- 设置目标运行速度 (RPM) ---
bool AgeMotionDriver::setTargetRPM(double rpm)
{
    if (!m_isConnected || !m_api_writeWORD) return false;

    // 转换公式: VelSet = (16 * RPM) / 5 (根据手册 4.4.21)
    // 注意: VelSet 值域 1~38400
    // unsigned short val = (unsigned short)((rpm * 16.0) / 5.0);
    WORD val = (WORD)((rpm * MMS_PER_R) / (KV_DEFAULT * 60000));

    // 写入速度设定寄存器 0x0040
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_VEL_SET, val, TIMEOUT_MS);
}

bool AgeMotionDriver::getTargetVelocity(double &velocityUmPerSec)
{
    double rpm = 0.0;
    if (getTargetRPM(rpm)) {
        // RPM = r/min = r/60s
        // velocity = (rpm / 60.0) * POSITION_PER_R
        velocityUmPerSec = (rpm / 60.0) * POSITION_PER_R;
        return true;
    }
    return false;
}

bool AgeMotionDriver::setTargetVelocity(double velocityUmPerSec)
{
    // velocity = (rpm / 60.0) * POSITION_PER_R
    // rpm = (velocity * 60.0) / POSITION_PER_R
    double rpm = (velocityUmPerSec * 60.0) / POSITION_PER_R;
    return setTargetRPM(rpm);
}

// --- 获取实时速度 (um/s) ---
bool AgeMotionDriver::getVelocity(double &velocityUmPerSec)
{
    if (!m_isConnected || !m_api_readWORD) return false;

    WORD rawVel = 0;
    // 读取实时速度寄存器 0x0045 (SHORT)
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_VEL_REAL, rawVel, TIMEOUT_MS)) {
        // 转换为有符号 short
        short signedVel = (short)rawVel;
        
        // 转换为 RPM
        // 公式: RPM = (VelReal * KV * 60000) / MMS_PER_R
        double rpm = (signedVel * KV_DEFAULT * 60000.0) / MMS_PER_R;
        
        // 转换为 um/s
        velocityUmPerSec = (rpm / 60.0) * POSITION_PER_R;
        return true;
    }
    return false;
}

// --- 设置恒定运行速度 (Jog模式) ---
bool AgeMotionDriver::setVelocity(double velocityUmPerSec)
{
    // 速度为0则停止
    if (qAbs(velocityUmPerSec) < 0.001) {
        return stopMotion();
    }

    // 1. 设置运行速度 (取绝对值)
    if (!setTargetVelocity(qAbs(velocityUmPerSec))) return false;

    // 2. 设置目标位置为极大值 (模拟恒定运行)
    // 正速度 -> 正无穷, 负速度 -> 负无穷
    // 假设 100米 (100,000,000 um) 足够远
    double targetPos = (velocityUmPerSec > 0) ? 100000000.0 : -100000000.0;
    
    // 手动写入位置寄存器，避免调用 setTargetPosition (因为它会恢复默认速度)
    if (!m_isConnected || !m_api_writeQWORD) return false;
    
    long long mms = (long long)(targetPos * MMS_PER_UM);
    return m_api_writeQWORD(STATION_ID, AgeReg::ADDR_POS_TARGET, (QWORD)mms, TIMEOUT_MS);
}

// --- 绝对运动到指定位置 (微米) ---
bool AgeMotionDriver::setTargetPosition(double positionUm)
{
    if (!m_isConnected || !m_api_writeQWORD) return false;

    // 恢复默认速度 (防止之前调用 setVelocity 修改了速度)
    if (m_defaultTargetVelocity > 0.001) {
        setTargetVelocity(m_defaultTargetVelocity);
    }

    // 1. 将微米转换为脉冲/微步 (MMS)
    // 1 um = MMS_PER_UM 
    long long mms = (long long)(positionUm * MMS_PER_UM);

    // 2. 写入目标位置寄存器 0x0024
    // 注意: 类型是 INT64 (QWORD)
    return m_api_writeQWORD(STATION_ID, AgeReg::ADDR_POS_TARGET, (QWORD)mms, TIMEOUT_MS);
}

// --- 相对运动 (微米) ---
bool AgeMotionDriver::setRelativePosition(double deltaUm)
{
    double targetPos = 0.0;
    // 1. 获取当前目标位置 (基于上一次的目标位置进行增量，避免多次累积误差或运动中修改)
    if (!getTargetPosition(targetPos)) return false;
    
    // 2. 计算目标位置并执行绝对运动
    return setTargetPosition(targetPos + deltaUm);
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

// ==========================================
//          新增：控制寄存器功能
// ==========================================

bool AgeMotionDriver::setEnable(bool enable)
{
    if (!m_isConnected || !m_api_readWORD || !m_api_writeWORD) return false;

    WORD ctrl = 0;
    // 1. 读取当前控制字
    if (!m_api_readWORD(STATION_ID, AgeReg::ADDR_CONTROL, ctrl, TIMEOUT_MS)) return false;

    // 2. 修改 Bit 2 使能位
    // 0x0004 = 0000 0000 0000 0100 (二进制)
    // 通常: 1 = Enable, 0 = Disable
    if (enable) {
        ctrl |= 0x0004;
    } else {
        ctrl &= ~0x0004;
    }

    // 3. 写回
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_CONTROL, ctrl, TIMEOUT_MS);
}

bool AgeMotionDriver::emergencyStop()
{
    if (!m_isConnected || !m_api_writeWORD) return false;

    // 假设 Bit 13 为急停 (Stop 是 Bit 12)
    // 0x2000 = 0010 0000 0000 0000 (二进制)
    // 这里直接发送急停指令，不读取旧值以保证速度
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_CONTROL, 0x2000, TIMEOUT_MS);
}

bool AgeMotionDriver::moveToLimit(bool toUpper)
{
    if (!m_isConnected || !m_api_writeWORD) return false;
    //  Bit 4 = 向上限位运动, Bit 5 = 向下限位运动
    // 0x0010 = 0000 0000 0001 0000 (二进制)
    // 0x0020 = 0000 0000 0010 0000 (二进制)
    WORD cmd = toUpper ? 0x0010 : 0x0020;
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_CONTROL, cmd, TIMEOUT_MS);
}

bool AgeMotionDriver::setCurrPositionToZero()
{
    if (!m_isConnected || !m_api_writeWORD) return false;
    //  Bit 8 = 位置偏移清零
    // 0x0100 = 0000 0001 0000 0000 (二进制)
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_CONTROL, 0x0100, TIMEOUT_MS);
}

bool AgeMotionDriver::findReference(bool toHigh)
{
    if (!m_isConnected || !m_api_writeWORD) return false;
    // Bit 11 = 向高位回零, Bit 10 = 向低位回零
    // 0x0800 = 0000 1000 0000 0000 (二进制)
    // 0x0400 = 0000 0100 0000 0000 (二进制)
    WORD cmd = toHigh ? 0x0800 : 0x0400;
    return m_api_writeWORD(STATION_ID, AgeReg::ADDR_CONTROL, cmd, TIMEOUT_MS);
}

bool AgeMotionDriver::isMotionComplete(bool &isDone)
{
    if (!m_isConnected || !m_api_readQWORD) return false;

    QWORD realPos = 0;
    QWORD targetPos = 0;

    // 读取实时位置和目标位置
    if (!m_api_readQWORD(STATION_ID, AgeReg::ADDR_POS_REAL, realPos, TIMEOUT_MS)) return false;
    if (!m_api_readQWORD(STATION_ID, AgeReg::ADDR_POS_TARGET, targetPos, TIMEOUT_MS)) return false;

    long long diff = (long long)realPos - (long long)targetPos;
    if (diff < 0) diff = -diff;

    // 阈值设定
    isDone = (diff < MMS_PER_UM/2); // 允许半微米误差
    return true;
}

bool AgeMotionDriver::isHomingComplete(bool &isDone)
{
    if (!m_isConnected || !m_api_readWORD) return false;
    WORD ctrl = 0;
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_CONTROL, ctrl, TIMEOUT_MS)) {
        // 如果 Bit 10 和 Bit 11 都是 0，则动作完成
        // 0x0C00 = 0000 1100 0000 0000
        isDone = ((ctrl & 0x0C00) == 0);
        return true;
    }
    return false;
}

bool AgeMotionDriver::isLimitSensorTriggered(bool &upper, bool &lower)
{
    // if (!m_isConnected || !m_api_readWORD) return false;
    // WORD portStatus = 0;
    // // 读取 IO 端口状态 0x0080
    // if (m_api_readWORD(STATION_ID, AgeReg::ADDR_PORT_STATUS, portStatus, TIMEOUT_MS)) {
    //     // 假设 Bit 0 = 上限位, Bit 1 = 下限位
    //     upper = (portStatus & 0x0001) != 0;
    //     lower = (portStatus & 0x0002) != 0;
    //     return true;
    // }
    return false;
}

// ==========================================
//          新增：脉冲位置功能 (INT32)
// ==========================================

bool AgeMotionDriver::getPulsePosition(int &pulses)
{
    if (!m_isConnected || !m_api_readDWORD) return false;

    DWORD raw = 0;
    if (m_api_readDWORD(STATION_ID, AgeReg::ADDR_PULSE_POS_REAL, raw, TIMEOUT_MS)) {
        pulses = (int)raw; // 强制转换为有符号 int
        return true;
    }
    return false;
}

bool AgeMotionDriver::setTargetPulsePosition(int pulses)
{
    if (!m_isConnected || !m_api_writeDWORD) return false;

    // 写入脉冲目标位置
    return m_api_writeDWORD(STATION_ID, AgeReg::ADDR_PULSE_POS_SET, (DWORD)pulses, TIMEOUT_MS);
}

// ==========================================
//          新增：其他信息读取
// ==========================================

bool AgeMotionDriver::getRealTimeCurrent(double &current)
{
    if (!m_isConnected || !m_api_readWORD) return false;
    WORD raw = 0;
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_CURRENT_REAL, raw, TIMEOUT_MS)) {
        // 假设单位是 0.01A
        current = raw / 100.0;
        return true;
    }
    return false;
}

bool AgeMotionDriver::getCpuTemperature(int &temp)
{
    if (!m_isConnected || !m_api_readWORD) return false;
    WORD raw = 0;
    if (m_api_readWORD(STATION_ID, AgeReg::ADDR_CPU_TEMP, raw, TIMEOUT_MS)) {
        temp = (short)raw; // 转为有符号
        return true;
    }
    return false;
}

// ==========================================
//          新增：分辨率与步长 (UINT32)
// ==========================================

bool AgeMotionDriver::getSingleToothResolution(unsigned int &res)
{
    if (!m_isConnected || !m_api_readDWORD) return false;

    DWORD raw = 0;
    if (m_api_readDWORD(STATION_ID, AgeReg::ADDR_T_RESOLUTION, raw, TIMEOUT_MS)) {
        res = (unsigned int)raw;
        return true;
    }
    return false;
}

bool AgeMotionDriver::getPulseStepLength(unsigned int &length)
{
    if (!m_isConnected || !m_api_readDWORD) return false;

    DWORD raw = 0;
    if (m_api_readDWORD(STATION_ID, AgeReg::ADDR_PULSE_LENGTH, raw, TIMEOUT_MS)) {
        length = (unsigned int)raw;
        return true;
    }
    return false;
}

bool AgeMotionDriver::setPulseStepLength(unsigned int length)
{
    if (!m_isConnected || !m_api_writeDWORD) return false;
    return m_api_writeDWORD(STATION_ID, AgeReg::ADDR_PULSE_LENGTH, (DWORD)length, TIMEOUT_MS);
}

bool AgeMotionDriver::getMinStepUm(double &stepUm)
{
    unsigned int pulses = 0;
    if (getPulseStepLength(pulses)) {
        stepUm = (double)pulses / MMS_PER_UM;
        return true;
    }
    return false;
}

bool AgeMotionDriver::setMinStepUm(double stepUm)
{
    unsigned int pulses = (unsigned int)(stepUm * MMS_PER_UM);
    return setPulseStepLength(pulses);
}
