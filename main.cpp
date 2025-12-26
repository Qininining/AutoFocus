// #include "mainwindow.h"
// #include <QApplication>
// #include <QLibrary>
// #include <QDebug>
// #include <QFileInfo>

// // --- 类型定义 (保持与 AgeCOM.h 一致) ---
// typedef long BOOL32;
// typedef unsigned char BYTE;
// typedef unsigned short WORD;
// typedef unsigned long DWORD;
// typedef unsigned long long QWORD;

// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);

//     // 建议：在该路径下确认 dll 是否存在
//     QString dllPath = QFileInfo(__FILE__).absolutePath() + "/AgeMotionForDriver/x64/AgeCOM.dll";
//     QLibrary lib(dllPath);

//     if (!lib.load()) {
//         qDebug() << "Failed to load AgeCOM.dll at" << dllPath;
//         qDebug() << "Error:" << lib.errorString();
//         return -1;
//     }

//     // --- 1. 修正函数指针定义 (必须严格匹配 .h 文件) ---

//     // AgeCOMIsValid(BOOL32 bAutoConnect)
//     typedef BOOL32 (*AgeCOMIsValidFunc)(BOOL32);

//     // AgeCOMGetUSBID(BYTE* pucUSBID)
//     typedef BOOL32 (*AgeCOMGetUSBIDFunc)(BYTE*);

//     // AgeCOMReadQWORD(BYTE ucRTUAddr, WORD wRegAddr, QWORD& qwData, DWORD dwTimeout)
//     // 注意：原文是 QWORD& (unsigned long long)
//     typedef BOOL32 (*AgeCOMReadQWORDFunc)(BYTE, WORD, QWORD&, DWORD);

//     // AgeCOMGetCOMID(WORD& wCOMID) -> 注意这里必须是 WORD (unsigned short)
//     typedef BOOL32 (*AgeCOMGetCOMIDFunc)(WORD&);

//     // AgeCOMSerial(BYTE* pucSerial, DWORD dwLength) -> 注意这里有两个参数
//     typedef BOOL32 (*AgeCOMSerialFunc)(BYTE*, DWORD);

//     typedef BOOL32 (*AgeCOMGetBusInfoFunc)(
//         long long&, long long&, long long&, long long&, long long&,
//         long long&, long long&, long long&, long long&, long long&,
//         long long&, long long&, long long&, long long&
//         );

//     // --- 2. 解析函数 ---
//     auto isValid   = (AgeCOMIsValidFunc)lib.resolve("AgeCOMIsValid");
//     auto getUSBID  = (AgeCOMGetUSBIDFunc)lib.resolve("AgeCOMGetUSBID");
//     auto readQWORD = (AgeCOMReadQWORDFunc)lib.resolve("AgeCOMReadQWORD");
//     auto getCOMID  = (AgeCOMGetCOMIDFunc)lib.resolve("AgeCOMGetCOMID");
//     auto setSerial = (AgeCOMSerialFunc)lib.resolve("AgeCOMSerial");

//     // 检查指针
//     if (!isValid || !getUSBID || !readQWORD || !getCOMID || !setSerial) {
//         qDebug() << "Failed to resolve one or more AgeCOM functions.";
//         if (!isValid) qDebug() << "Missing AgeCOMIsValid";
//         if (!getUSBID) qDebug() << "Missing AgeCOMGetUSBID";
//         if (!readQWORD) qDebug() << "Missing AgeCOMReadQWORD";
//         if (!getCOMID) qDebug() << "Missing AgeCOMGetCOMID";
//         if (!setSerial) qDebug() << "Missing AgeCOMSerial";
//         // getSerial 已被移除，因为它不存在
//         return -1;
//     }

//     // --- 3. 连接逻辑 ---

//     // 验证序列号 (类似登录授权)
//     // 注意：根据 Demo，如果不调用这个，可能只能访问地址 0 或 1
//     // Demo 中的序列号示例
//     // CString strSerial = _T("44742-40890-65242-54760-32341-31258-35993-51871");
//     // 这里我们用你原来的示例，但记得加上长度
//     unsigned char authSerial[] = {
//         0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34,
//         0x00, 0x00, 0x00, 0x00, 0x00, 0x00
//     };
//     // 注意：如果只是测试连接，Demo中用的是很长的一串字符串转换成的字节。
//     // 如果你没有特定序列号，可能这一步会失败，或者不需要这一步(默认只有部分权限)。
//     // 这里演示正确的调用方式：传入指针和长度。
//     if (setSerial(authSerial, sizeof(authSerial))) {
//         qDebug() << "🔹 AgeCOMSerial authorization call success.";
//     } else {
//         qDebug() << "⚠️ AgeCOMSerial returned FALSE (Auth failed or not needed).";
//     }

//     // 注意：请确保你从官方软件中复制完整的字符串，不要漏掉任何一个0或横杠
//     const char* licenseKey = "AgeMotion-20010203-00000000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000-0000";

//     // 获取字符串长度
//     DWORD keyLength = (DWORD)strlen(licenseKey);

//     // 调用授权函数
//     // 注意：需要将 char* 强转为 BYTE*
//     if (setSerial((BYTE*)licenseKey, keyLength)) {
//         qDebug() << "✅ AgeCOMSerial authorization SUCCESS!";
//     } else {
//         qDebug() << "⚠️ AgeCOMSerial authorization FAILED. (Check string accuracy)";
//     }

//     // 自动连接
//     if (!isValid(1)) {
//         qDebug() << "❌ Failed to connect to USB Device (AgeCOMIsValid returned FALSE).";
//         return -1;
//     }
//     qDebug() << "✅ USB Device connected successfully!";

//     // --- Get COM ID ---
//     // 修正：使用 WORD (unsigned short)
//     WORD comID = 0;
//     if (getCOMID(comID)) {
//         qDebug() << "🔹 COM ID:" << comID;
//     } else {
//         qDebug() << "❌ Failed to get COM ID.";
//     }

//     // --- Get USB ID ---
//     BYTE usbID[256] = {0};
//     if (getUSBID(usbID)) {
//         // 通常 USB ID 是字符串或特定长度的十六进制，这里假设打印前12个字节
//         QByteArray id((char*)usbID, 12); // 注意 Demo 里没有指定返回长度，但buffer通常够大
//         qDebug() << "🔹 USB ID (hex):" << id.toHex().toUpper();
//     }

//     // --- Read Motor Position ---
//     QWORD position = 0; // 使用 unsigned long long
//     // 假设读取 RTU地址 1, 寄存器 0x0020, 超时 0 (自动)
//     if (readQWORD(1, 0x0020, position, 0)) {
//         // 注意：如果你需要将其视为有符号数处理位置（例如负方向），可以强转
//         long long signedPos = (long long)position;
//         double mm = signedPos / 16000.0;
//         qDebug() << "🔹 Motor Position:" << mm << "mm (" << position << ")";
//     } else {
//         qDebug() << "❌ Failed to read motor position.";
//     }


//     auto getBusInfo = (AgeCOMGetBusInfoFunc)lib.resolve("AgeCOMGetBusInfo");

//     if (getBusInfo) {
//         long long hostRunTime, busRunTime, lastOpTime, maxOpTime, minOpTime;
//         long long busOpCounts, txFrames, rxFrames, txBytes, rxBytes;
//         long long hostErrors, busOpErrors, txFrameErrors, rxFrameErrors;

//         if (getBusInfo(
//                 hostRunTime, busRunTime, lastOpTime, maxOpTime, minOpTime,
//                 busOpCounts, txFrames, rxFrames, txBytes, rxBytes,
//                 hostErrors, busOpErrors, txFrameErrors, rxFrameErrors
//                 )) {
//             qDebug() << "✅ AgeCOMGetBusInfo Success - Detailed Bus Statistics:";
//             qDebug() << "   Host Run Time (ms):" << hostRunTime;           // DLL 启动至今主机运行时间
//             qDebug() << "   Bus Run Time (ms):" << busRunTime;            // 总线通信总耗时（Tx+Rx）
//             qDebug() << "   Last Op Time (ms):" << lastOpTime;            // 上次操作耗时
//             qDebug() << "   Max Op Time (ms):" << maxOpTime;              // 最大单次操作耗时
//             qDebug() << "   Min Op Time (ms):" << minOpTime;              // 最小单次操作耗时
//             qDebug() << "   Bus Op Counts:" << busOpCounts;               // 总线操作总次数
//             qDebug() << "   TX Frames:" << txFrames;                      // 发送帧数
//             qDebug() << "   RX Frames:" << rxFrames;                      // 接收帧数
//             qDebug() << "   TX Bytes:" << txBytes;                        // 发送字节数
//             qDebug() << "   RX Bytes:" << rxBytes;                        // 接收字节数
//             qDebug() << "   Host Errors:" << hostErrors;                  // 主机软硬件错误计数
//             qDebug() << "   Bus Op Errors:" << busOpErrors;               // 总线操作错误（如超时、校验错）
//             qDebug() << "   TX Frame Errors:" << txFrameErrors;           // 发送帧错误
//             qDebug() << "   RX Frame Errors:" << rxFrameErrors;           // 接收帧错误
//         } else {
//             qDebug() << "❌ AgeCOMGetBusInfo returned FALSE";
//         }
//     } else {
//         qDebug() << "⚠️ AgeCOMGetBusInfo not found in DLL";
//     }


//     MainWindow w;
//     w.show();
//     return a.exec();
// }



#include "mainwindow.h"
#include <QApplication>
#include <QLibrary>
#include <QDebug>
#include <QFileInfo>
#include <QThread>

// 引入 AgeCOM 头文件以获取类型定义
// 注意：由于没有 .lib 文件，我们不能直接调用头文件中声明的函数
// 我们将使用 QLibrary 动态加载 DLL，并使用头文件中的 typedef
#include "AgeMotionForDriver/x64/AgeCOM.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "============================================";
    qDebug() << "   Starting AgeCOM Dynamic Load Test";
    qDebug() << "============================================";

    // --- 1. 加载 DLL ---
    QString dllPath = QFileInfo(__FILE__).absolutePath() + "/AgeMotionForDriver/x64/AgeCOM.dll";
    QLibrary lib(dllPath);

    if (!lib.load()) {
        qDebug() << "Failed to load AgeCOM.dll at" << dllPath;
        qDebug() << "Error:" << lib.errorString();
        return -1;
    }

    // --- 2. 解析函数指针 ---
    // 使用 AgeCOM.h 中定义的 typedef (例如 FnAgeCOMSerial)

    auto pAgeCOMSerial      = (FnAgeCOMSerial*)     lib.resolve("AgeCOMSerial");
    auto pAgeCOMIsValid     = (FnAgeCOMIsValid*)    lib.resolve("AgeCOMIsValid");
    auto pAgeCOMGetCOMID    = (FnAgeCOMGetCOMID*)   lib.resolve("AgeCOMGetCOMID");
    auto pAgeCOMGetUSBID    = (FnAgeCOMGetUSBID*)   lib.resolve("AgeCOMGetUSBID");
    auto pAgeCOMGetCOM      = (FnAgeCOMGetCOM*)     lib.resolve("AgeCOMGetCOM");
    auto pAgeCOMSetCOM      = (FnAgeCOMSetCOM*)     lib.resolve("AgeCOMSetCOM");
    auto pAgeCOMGetBusInfo  = (FnAgeCOMGetBusInfo*) lib.resolve("AgeCOMGetBusInfo");
    auto pAgeCOMReadWORD    = (FnAgeCOMReadWORD*)   lib.resolve("AgeCOMReadWORD");
    auto pAgeCOMWriteWORD   = (FnAgeCOMWriteWORD*)  lib.resolve("AgeCOMWriteWORD");
    auto pAgeCOMReadDWORD   = (FnAgeCOMReadDWORD*)  lib.resolve("AgeCOMReadDWORD");
    auto pAgeCOMWriteDWORD  = (FnAgeCOMWriteDWORD*) lib.resolve("AgeCOMWriteDWORD");
    auto pAgeCOMReadQWORD   = (FnAgeCOMReadQWORD*)  lib.resolve("AgeCOMReadQWORD");

    // 检查关键函数是否加载成功
    if (!pAgeCOMIsValid || !pAgeCOMSerial || !pAgeCOMReadWORD || !pAgeCOMWriteWORD) {
        qDebug() << "Failed to resolve one or more AgeCOM functions.";
        return -1;
    }

    // --- 3. 授权 (AgeCOMSerial) ---
    const char* strSerial = "44742-40890-65242-54760-32341-31258-35993-51871";
    DWORD dwLength = (DWORD)strlen(strSerial);

    if (pAgeCOMSerial((BYTE*)strSerial, dwLength)) {
        qDebug() << "[Auth] AgeCOMSerial: Authorization Success";
    } else {
        qDebug() << "[Auth] AgeCOMSerial: Authorization Failed (or not needed)";
    }

    // --- 4. 检查连接 (AgeCOMIsValid) ---
    if (pAgeCOMIsValid(true)) {
        qDebug() << "[Conn] AgeCOMIsValid: Device Connected and Valid";
    } else {
        qDebug() << "[Conn] AgeCOMIsValid: Device NOT Connected or Invalid";
    }

    // --- 5. 获取设备信息 ---
    if (pAgeCOMGetCOMID) {
        WORD wCOMID = 0;
        if (pAgeCOMGetCOMID(wCOMID)) {
            qDebug() << "[Info] AgeCOMGetCOMID: COM ID =" << wCOMID;
        }
    }

    // --- Get USB ID ---不好用
    if (pAgeCOMGetUSBID) {
        BYTE pucUSBID[256] = {0};
        if (pAgeCOMGetUSBID(pucUSBID)) {
            qDebug() << "[Info] AgeCOMGetUSBID: USB ID =" << (char*)pucUSBID;
        }
    }
    // --- Get USB ID ---好用
    BYTE usbID[256] = {0};
    if (pAgeCOMGetUSBID(usbID)) {
        // 通常 USB ID 是字符串或特定长度的十六进制，这里假设打印前12个字节
        QByteArray id((char*)usbID, 12); // 注意 Demo 里没有指定返回长度，但buffer通常够大
        qDebug() << "[Info] AgeCOMGetUSBID: USB ID =" << id.toHex().toUpper();
    }

    // --- 6. 配置串口 ---
    if (pAgeCOMGetCOM && pAgeCOMSetCOM) {
        DWORD dwBaudRate = 0;
        WORD wParity = 0;
        if (pAgeCOMGetCOM(dwBaudRate, wParity)) {
            qDebug() << "[Comm] AgeCOMGetCOM: Current BaudRate =" << dwBaudRate << ", Parity =" << wParity;
        }

        // 设置为 115200, Even (2)
        if (pAgeCOMSetCOM(115200, 2)) {
            qDebug() << "[Comm] AgeCOMSetCOM: Set to 115200, Even Parity Success";
        }
    }

    // --- 7. 获取总线信息 ---
    if (pAgeCOMGetBusInfo) {
        LONGLONG llHostRunTime, llBusRunTime, llLastOpTime, llMaxOpTime, llMinOpTime;
        LONGLONG llBusOpCounts, llTxFrames, llRxFrames, llTxBytes, llRxBytes;
        LONGLONG llHostErrors, llBusOpErrors, llTxFrameErrors, llRxFrameErrors;

        if (pAgeCOMGetBusInfo(llHostRunTime, llBusRunTime, llLastOpTime, llMaxOpTime, llMinOpTime,
                              llBusOpCounts, llTxFrames, llRxFrames, llTxBytes, llRxBytes,
                              llHostErrors, llBusOpErrors, llTxFrameErrors, llRxFrameErrors)) {
            qDebug() << "[Stat] AgeCOMGetBusInfo: Success";
            qDebug() << "       HostRunTime:" << llHostRunTime << "ms";
            qDebug() << "       BusOpCounts:" << llBusOpCounts;
            qDebug() << "       TxFrames:" << llTxFrames << "RxFrames:" << llRxFrames;
        }
    }

    // // --- 8. 读写测试 (Read/Write WORD/DWORD) ---（无法使用）
    // BYTE ucRTUAddr = 1;
    // DWORD dwTimeout = 0; // 0 = auto calculate

    // qDebug() << "\n--- Starting Register Read/Write Tests (RTU Addr: 1) ---";

    // // 读取 Control 寄存器 (Reg 0)
    // WORD wControl = 0;
    // if (pAgeCOMReadWORD(ucRTUAddr, 0, wControl, dwTimeout)) {
    //     qDebug() << "[Read] Control Reg (0x00): 0x" << QString::number(wControl, 16);
    // } else {
    //     qDebug() << "[Read] Control Reg (0x00): Failed";
    // }

    // // 写入测试：Reset (0x0001) -> Normal (0x0000) -> Free (0x0004) -> Normal (0x0000)
    // qDebug() << "[Test] Performing Reset Sequence...";
    // bool bRet = true;
    // bRet &= pAgeCOMWriteWORD(ucRTUAddr, 0, 0x0001, dwTimeout); // Reset
    // bRet &= pAgeCOMWriteWORD(ucRTUAddr, 0, 0x0000, dwTimeout); // Normal
    // bRet &= pAgeCOMWriteWORD(ucRTUAddr, 0, 0x0004, dwTimeout); // Free
    // bRet &= pAgeCOMWriteWORD(ucRTUAddr, 0, 0x0000, dwTimeout); // Normal (Enable)

    // if (bRet) qDebug() << "[Test] Reset Sequence Completed Successfully.";
    // else qDebug() << "[Test] Reset Sequence Failed at some step.";

    // // 运动控制测试
    // if (pAgeCOMWriteDWORD) {
    //     // 设置脉冲长度 (Reg 0x002A) -> 1000 step/rev
    //     if (pAgeCOMWriteDWORD(ucRTUAddr, 0x002A, 3840, dwTimeout)) {
    //         qDebug() << "[Write] Set Pulse Length (Reg 0x002A) to 3840: Success";
    //     }

    //     // 位置清零 (Reg 0, 0x0100)
    //     if (pAgeCOMWriteWORD(ucRTUAddr, 0, 0x0100, dwTimeout)) {
    //         qDebug() << "[Write] Offset Position to 0 (Reg 0, 0x0100): Success";
    //     }

    //     // 移动到 +45度 (Reg 0x002E, Value 125)
    //     if (pAgeCOMWriteDWORD(ucRTUAddr, 0x002E, 125, dwTimeout)) {
    //         qDebug() << "[Move] Move to +45 Deg (Reg 0x002E, 125): Success";
    //     }
    // }

    // // 简单的延时
    // QThread::msleep(500);

    // // 读取当前位置
    // if (pAgeCOMReadDWORD) {
    //     DWORD dwPosition = 0;
    //     if (pAgeCOMReadDWORD(ucRTUAddr, 0x002E, dwPosition, dwTimeout)) {
    //         qDebug() << "[Read] Current Position (Reg 0x002E):" << (int)dwPosition;
    //     }
    // }

    // // 移动到 +90度
    // if (pAgeCOMWriteDWORD) {
    //     if (pAgeCOMWriteDWORD(ucRTUAddr, 0x002E, 250, dwTimeout)) {
    //         qDebug() << "[Move] Move to +90 Deg (Reg 0x002E, 250): Success";
    //     }
    // }

        // --- Read Motor Position --- ---（可以使用）
        QWORD position = 0; // 使用 unsigned long long
        // 假设读取 RTU地址 1, 寄存器 0x0020, 超时 0 (自动)
        if (pAgeCOMReadQWORD(1, 0x0020, position, 0)) {
            // 注意：如果你需要将其视为有符号数处理位置（例如负方向），可以强转
            long long signedPos = (long long)position;
            double mm = signedPos / 16000.0;
            qDebug() << "Motor Position:" << mm << "mm (" << position << ")";
        } else {
            qDebug() << "❌ Failed to read motor position.";
        }

    MainWindow w;
    w.show();
    return a.exec();
}
