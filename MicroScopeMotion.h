#ifndef MICROSCOPEMOTION_H
#define MICROSCOPEMOTION_H

#include <string>
#include <vector>
#include <windows.h>

#include "AgeMotionForDriver/x64/AgeCOM.h"

struct AgeBusInfo
{
    LONGLONG hostRunTimeMs{};
    LONGLONG busRunTimeMs{};
    LONGLONG lastOpTimeMs{};
    LONGLONG maxOpTimeMs{};
    LONGLONG minOpTimeMs{};
    LONGLONG busOpCounts{};
    LONGLONG txFrames{};
    LONGLONG rxFrames{};
    LONGLONG txBytes{};
    LONGLONG rxBytes{};
    LONGLONG hostErrors{};
    LONGLONG busOpErrors{};
    LONGLONG txFrameErrors{};
    LONGLONG rxFrameErrors{};
};

class MicroScopeMotion
{
public:
    MicroScopeMotion();
    explicit MicroScopeMotion(const std::wstring& dllPath);
    ~MicroScopeMotion();

    // ========================================================================
    // High-Level Interface (Public)
    // ========================================================================

    /**
     * @brief Connect to the motion controller.
     * @param portName The serial port name (e.g., "COM3").
     * @param baudRate The baud rate (default 115200).
     * @return True if connected successfully, false otherwise.
     */
    bool connect(const std::string& portName, DWORD baudRate = 115200);

    /**
     * @brief Disconnect from the motion controller.
     */
    void disconnect();

    /**
     * @brief Check if the controller is connected.
     */
    bool isConnected() const;

    /**
     * @brief Move to an absolute position.
     * @param position The target position in steps (or user units).
     * @return True if the command was sent successfully.
     */
    bool setPosition(int32_t position);

    /**
     * @brief Get the current position.
     * @param position Output parameter for the current position.
     * @return True if the position was read successfully.
     */
    bool getPosition(int32_t& position) const;

    /**
     * @brief Move relative to the current position.
     * @param distance The distance to move.
     * @return True if the command was sent successfully.
     */
    bool moveRelative(int32_t distance);

    /**
     * @brief Stop the motion immediately.
     */
    bool stop();

    /**
     * @brief Home the axis (return to zero/reference position).
     */
    bool home();

    /**
     * @brief Set the motion speed.
     * @param speed The speed value.
     */
    bool setSpeed(int32_t speed);

    /**
     * @brief Get the current motion speed.
     */
    bool getSpeed(int32_t& speed) const;

    /**
     * @brief Check if the motor is currently moving.
     */
    bool isMoving() const;

private:
    // ========================================================================
    // Low-Level Interface (Private)
    // ========================================================================

    bool load(const std::wstring& dllPath = L"AgeCOM.dll");
    void unload();
    bool isLoaded() const;

    bool setSerial(const std::string& serial) const;
    bool isValid(bool autoConnect = true) const;

    bool getCOMConfig(DWORD& baudRate, WORD& parity) const;
    bool setCOMConfig(DWORD baudRate, WORD parity) const;

    bool getBusInfo(AgeBusInfo& info) const;

    bool readWord(BYTE rtuAddr, WORD regAddr, WORD& data, DWORD timeoutMs = 0) const;
    bool readDWord(BYTE rtuAddr, WORD regAddr, DWORD& data, DWORD timeoutMs = 0) const;
    bool readQWord(BYTE rtuAddr, WORD regAddr, QWORD& data, DWORD timeoutMs = 0) const;
    bool readMWord(BYTE rtuAddr, WORD regAddr, std::vector<WORD>& buffer, DWORD timeoutMs = 0) const;

    bool writeWord(BYTE rtuAddr, WORD regAddr, WORD data, DWORD timeoutMs = 0) const;
    bool writeDWord(BYTE rtuAddr, WORD regAddr, DWORD data, DWORD timeoutMs = 0) const;
    bool writeQWord(BYTE rtuAddr, WORD regAddr, QWORD data, DWORD timeoutMs = 0) const;
    bool writeMWord(BYTE rtuAddr, WORD regAddr, const std::vector<WORD>& buffer, DWORD timeoutMs = 0) const;

private:
    void resetFunctionPointers();
    bool resolveSymbols();

    // Register Map based on AgeCOM.h example
    static constexpr BYTE DEFAULT_SLAVE_ID = 1;
    
    enum RegisterMap : WORD {
        REG_CONTROL_WORD    = 0x0000, // Control register (16-bit)
        REG_STATUS_WORD     = 0x0001, // Status register (16-bit)
        REG_PULSE_LENGTH    = 0x002A, // Pulse Length / Resolution (32-bit)
        REG_SPEED           = 0x002C, // Target Speed / Frequency (32-bit) - TODO: Verify address
        REG_TARGET_POS      = 0x002E, // Target Position (32-bit)
    };

    // Commands for Control Word (REG_CONTROL_WORD)
    enum ControlCommand : WORD {
        CMD_NORMAL          = 0x0000, // Normal / Enable
        CMD_RESET           = 0x0001, // Reset
        CMD_FREE            = 0x0004, // Free / Disable
        CMD_ZERO_POS        = 0x0100  // Offset current position to 0
    };

    HMODULE m_dll{nullptr};
    bool m_connected{false};

    FnAgeCOMSerial* m_fnSerial{nullptr};
    FnAgeCOMIsValid* m_fnIsValid{nullptr};
    FnAgeCOMGetCOM* m_fnGetCOM{nullptr};
    FnAgeCOMSetCOM* m_fnSetCOM{nullptr};
    FnAgeCOMGetBusInfo* m_fnGetBusInfo{nullptr};

    FnAgeCOMReadWORD* m_fnReadWord{nullptr};
    FnAgeCOMReadDWORD* m_fnReadDWord{nullptr};
    FnAgeCOMReadQWORD* m_fnReadQWord{nullptr};
    FnAgeCOMReadMWORD* m_fnReadMWord{nullptr};

    FnAgeCOMWriteWORD* m_fnWriteWord{nullptr};
    FnAgeCOMWriteDWORD* m_fnWriteDWord{nullptr};
    FnAgeCOMWriteQWORD* m_fnWriteQWord{nullptr};
    FnAgeCOMWriteMWORD* m_fnWriteMWord{nullptr};
};

#endif // MICROSCOPEMOTION_H
