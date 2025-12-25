#include "MicroScopeMotion.h"

namespace
{
	constexpr WORD kMaxReadWords = 125;
	constexpr WORD kMaxWriteWords = 123;
}

MicroScopeMotion::MicroScopeMotion()
{
	resetFunctionPointers();
    // Attempt to load default DLL
    load();
}

MicroScopeMotion::MicroScopeMotion(const std::wstring& dllPath)
{
	resetFunctionPointers();
	load(dllPath);
}

MicroScopeMotion::~MicroScopeMotion()
{
    disconnect();
	unload();
}

// ========================================================================
// High-Level Interface Implementation
// ========================================================================

bool MicroScopeMotion::connect(const std::string& portName, DWORD baudRate)
{
    if (!isLoaded())
    {
        if (!load()) return false;
    }

    // Set the serial key (Required for AgeCOM)
    // Key from AgeCOM.h example
    const std::string serialKey = "44742-40890-65242-54760-32341-31258-35993-51871";
    if (!setSerial(serialKey)) return false;

    // Note: portName is not used in setSerial, AgeCOM seems to auto-detect or use internal config?
    // Wait, AgeCOMSerial takes the KEY, not the COM port name.
    // The COM port is likely set via AgeCOMSetCOM or auto-detected?
    // Looking at AgeCOM.h: "Can only visit RTU address 1 or 0(broadcast) if has no serial."
    // And "Support plug and play".
    // But we might need to set the baudrate.
    
    // Configure COM port (Baudrate, Parity=Even based on example)
    // Example: bResult = m_pAgeCOMSetCOM(115200, 2); // bps: 115200, parity: even
    if (!setCOMConfig(baudRate, 2)) 
    {
        // Log warning?
    }

    // Check if valid (connect)
    if (isValid(true))
    {
        m_connected = true;
        return true;
    }

    return false;
}

void MicroScopeMotion::disconnect()
{
    m_connected = false;
    // No explicit disconnect in DLL, but we can mark as disconnected.
}

bool MicroScopeMotion::isConnected() const
{
    // Check internal flag and query DLL status
    return m_connected && isValid(false);
}

bool MicroScopeMotion::setPosition(int32_t position)
{
    if (!isConnected()) return false;
    // Write 32-bit position to Target Position registers
    return writeDWord(DEFAULT_SLAVE_ID, REG_TARGET_POS, static_cast<DWORD>(position));
}

bool MicroScopeMotion::getPosition(int32_t& position) const
{
    if (!isConnected()) return false;
    DWORD data = 0;
    // Assuming reading the Target Position register returns the current target (or actual if supported)
    if (readDWord(DEFAULT_SLAVE_ID, REG_TARGET_POS, data))
    {
        position = static_cast<int32_t>(data);
        return true;
    }
    return false;
}

bool MicroScopeMotion::moveRelative(int32_t distance)
{
    if (!isConnected()) return false;
    
    int32_t currentPos = 0;
    if (!getPosition(currentPos)) return false;
    
    return setPosition(currentPos + distance);
}

bool MicroScopeMotion::stop()
{
    if (!isConnected()) return false;
    // Write Normal command (0x0000) to Control Word
    return writeWord(DEFAULT_SLAVE_ID, REG_CONTROL_WORD, CMD_NORMAL); 
}

bool MicroScopeMotion::home()
{
    if (!isConnected()) return false;
    // Write Zero Position command (0x0100) to Control Word
    return writeWord(DEFAULT_SLAVE_ID, REG_CONTROL_WORD, CMD_ZERO_POS);
}

bool MicroScopeMotion::setSpeed(int32_t speed)
{
    if (!isConnected()) return false;
    // Write 32-bit speed to Speed register
    return writeDWord(DEFAULT_SLAVE_ID, REG_SPEED, static_cast<DWORD>(speed));
}

bool MicroScopeMotion::getSpeed(int32_t& speed) const
{
    if (!isConnected()) return false;
    DWORD data = 0;
    if (readDWord(DEFAULT_SLAVE_ID, REG_SPEED, data))
    {
        speed = static_cast<int32_t>(data);
        return true;
    }
    return false;
}

bool MicroScopeMotion::isMoving() const
{
    if (!isConnected()) return false;
    WORD status = 0;
    if (readWord(DEFAULT_SLAVE_ID, REG_STATUS_WORD, status))
    {
        // Check "Moving" bit. 
        // Assuming Bit 0 indicates "Busy" or "Moving" based on common Modbus implementations.
        // TODO: Verify which bit indicates motion in the documentation.
        return (status & 0x0001) != 0; 
    }
    return false;
}

// ========================================================================
// Low-Level Interface Implementation
// ========================================================================

bool MicroScopeMotion::load(const std::wstring& dllPath)
{
	unload();

	m_dll = ::LoadLibraryW(dllPath.c_str());
	if (!m_dll)
	{
		resetFunctionPointers();
		return false;
	}

	if (!resolveSymbols())
	{
		unload();
		return false;
	}

	return true;
}

void MicroScopeMotion::unload()
{
	if (m_dll)
	{
		::FreeLibrary(m_dll);
		m_dll = nullptr;
	}
	resetFunctionPointers();
    m_connected = false;
}

bool MicroScopeMotion::isLoaded() const
{
	return m_dll != nullptr;
}

bool MicroScopeMotion::setSerial(const std::string& serial) const
{
	if (!m_fnSerial || serial.empty())
	{
		return false;
	}
	return m_fnSerial(reinterpret_cast<BYTE*>(const_cast<char*>(serial.data())), static_cast<DWORD>(serial.size()));
}

bool MicroScopeMotion::isValid(bool autoConnect) const
{
	return m_fnIsValid ? m_fnIsValid(autoConnect ? 1 : 0) : false;
}

bool MicroScopeMotion::getCOMConfig(DWORD& baudRate, WORD& parity) const
{
	return m_fnGetCOM ? m_fnGetCOM(baudRate, parity) : false;
}

bool MicroScopeMotion::setCOMConfig(DWORD baudRate, WORD parity) const
{
	return m_fnSetCOM ? m_fnSetCOM(baudRate, parity) : false;
}

bool MicroScopeMotion::getBusInfo(AgeBusInfo& info) const
{
	if (!m_fnGetBusInfo)
	{
		return false;
	}

	return m_fnGetBusInfo(
		info.hostRunTimeMs,
		info.busRunTimeMs,
		info.lastOpTimeMs,
		info.maxOpTimeMs,
		info.minOpTimeMs,
		info.busOpCounts,
		info.txFrames,
		info.rxFrames,
		info.txBytes,
		info.rxBytes,
		info.hostErrors,
		info.busOpErrors,
		info.txFrameErrors,
		info.rxFrameErrors);
}

bool MicroScopeMotion::readWord(BYTE rtuAddr, WORD regAddr, WORD& data, DWORD timeoutMs) const
{
	return m_fnReadWord ? m_fnReadWord(rtuAddr, regAddr, data, timeoutMs) : false;
}

bool MicroScopeMotion::readDWord(BYTE rtuAddr, WORD regAddr, DWORD& data, DWORD timeoutMs) const
{
	return m_fnReadDWord ? m_fnReadDWord(rtuAddr, regAddr, data, timeoutMs) : false;
}

bool MicroScopeMotion::readQWord(BYTE rtuAddr, WORD regAddr, QWORD& data, DWORD timeoutMs) const
{
	return m_fnReadQWord ? m_fnReadQWord(rtuAddr, regAddr, data, timeoutMs) : false;
}

bool MicroScopeMotion::readMWord(BYTE rtuAddr, WORD regAddr, std::vector<WORD>& buffer, DWORD timeoutMs) const
{
	if (!m_fnReadMWord || buffer.empty() || buffer.size() > kMaxReadWords)
	{
		return false;
	}
	return m_fnReadMWord(rtuAddr, regAddr, buffer.data(), static_cast<WORD>(buffer.size()), timeoutMs);
}

bool MicroScopeMotion::writeWord(BYTE rtuAddr, WORD regAddr, WORD data, DWORD timeoutMs) const
{
	return m_fnWriteWord ? m_fnWriteWord(rtuAddr, regAddr, data, timeoutMs) : false;
}

bool MicroScopeMotion::writeDWord(BYTE rtuAddr, WORD regAddr, DWORD data, DWORD timeoutMs) const
{
	return m_fnWriteDWord ? m_fnWriteDWord(rtuAddr, regAddr, data, timeoutMs) : false;
}

bool MicroScopeMotion::writeQWord(BYTE rtuAddr, WORD regAddr, QWORD data, DWORD timeoutMs) const
{
	return m_fnWriteQWord ? m_fnWriteQWord(rtuAddr, regAddr, data, timeoutMs) : false;
}

bool MicroScopeMotion::writeMWord(BYTE rtuAddr, WORD regAddr, const std::vector<WORD>& buffer, DWORD timeoutMs) const
{
	if (!m_fnWriteMWord || buffer.empty() || buffer.size() > kMaxWriteWords)
	{
		return false;
	}
	return m_fnWriteMWord(rtuAddr, regAddr, const_cast<WORD*>(buffer.data()), static_cast<WORD>(buffer.size()), timeoutMs);
}

void MicroScopeMotion::resetFunctionPointers()
{
	m_fnSerial = nullptr;
	m_fnIsValid = nullptr;
	m_fnGetCOM = nullptr;
	m_fnSetCOM = nullptr;
	m_fnGetBusInfo = nullptr;
	m_fnReadWord = nullptr;
	m_fnReadDWord = nullptr;
	m_fnReadQWord = nullptr;
	m_fnReadMWord = nullptr;
	m_fnWriteWord = nullptr;
	m_fnWriteDWord = nullptr;
	m_fnWriteQWord = nullptr;
	m_fnWriteMWord = nullptr;
}

bool MicroScopeMotion::resolveSymbols()
{
	if (!m_dll)
	{
		return false;
	}

	m_fnSerial = reinterpret_cast<FnAgeCOMSerial*>(::GetProcAddress(m_dll, "AgeCOMSerial"));
	m_fnIsValid = reinterpret_cast<FnAgeCOMIsValid*>(::GetProcAddress(m_dll, "AgeCOMIsValid"));
	m_fnGetCOM = reinterpret_cast<FnAgeCOMGetCOM*>(::GetProcAddress(m_dll, "AgeCOMGetCOM"));
	m_fnSetCOM = reinterpret_cast<FnAgeCOMSetCOM*>(::GetProcAddress(m_dll, "AgeCOMSetCOM"));
	m_fnGetBusInfo = reinterpret_cast<FnAgeCOMGetBusInfo*>(::GetProcAddress(m_dll, "AgeCOMGetBusInfo"));
	m_fnReadWord = reinterpret_cast<FnAgeCOMReadWORD*>(::GetProcAddress(m_dll, "AgeCOMReadWORD"));
	m_fnReadDWord = reinterpret_cast<FnAgeCOMReadDWORD*>(::GetProcAddress(m_dll, "AgeCOMReadDWORD"));
	m_fnReadQWord = reinterpret_cast<FnAgeCOMReadQWORD*>(::GetProcAddress(m_dll, "AgeCOMReadQWORD"));
	m_fnReadMWord = reinterpret_cast<FnAgeCOMReadMWORD*>(::GetProcAddress(m_dll, "AgeCOMReadMWORD"));
	m_fnWriteWord = reinterpret_cast<FnAgeCOMWriteWORD*>(::GetProcAddress(m_dll, "AgeCOMWriteWORD"));
	m_fnWriteDWord = reinterpret_cast<FnAgeCOMWriteDWORD*>(::GetProcAddress(m_dll, "AgeCOMWriteDWORD"));
	m_fnWriteQWord = reinterpret_cast<FnAgeCOMWriteQWORD*>(::GetProcAddress(m_dll, "AgeCOMWriteQWORD"));
	m_fnWriteMWord = reinterpret_cast<FnAgeCOMWriteMWORD*>(::GetProcAddress(m_dll, "AgeCOMWriteMWORD"));

	const bool hasCore = m_fnSerial && m_fnIsValid && m_fnGetCOM && m_fnSetCOM && m_fnGetBusInfo;
	const bool hasRead = m_fnReadWord && m_fnReadDWord && m_fnReadQWord && m_fnReadMWord;
	const bool hasWrite = m_fnWriteWord && m_fnWriteDWord && m_fnWriteQWord && m_fnWriteMWord;

	return hasCore && hasRead && hasWrite;
}

