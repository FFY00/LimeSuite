/**
    @file   IConnection.cpp
    @author Lime Microsystems (www.limemicro.com)
    @brief  Implementation of IConnection interface
*/

#include "IConnection.h"
#include "ErrorReporting.h"
#include <cstring> //memcpy
#include <chrono>
#include <thread>
#include <iso646.h>

using namespace lime;

DeviceInfo::DeviceInfo(void):
    addrSi5351(-1),
    addrADF4002(-1)
{
    return;
}

StreamMetadata::StreamMetadata(void):
    timestamp(0),
    hasTimestamp(false),
    endOfBurst(false),
    lateTimestamp(false),
    packetDropped(false)
{
    return;
}

StreamConfig::StreamConfig(void):
    isTx(false),
    bufferLength(0),
    format(STREAM_12_BIT_IN_16),
    linkFormat(STREAM_12_BIT_IN_16)
{
    return;
}

IConnection::IConnection(void)
{
    callback_logData = nullptr;
    unsigned short test = 0x1234;
    unsigned char* bytes = (unsigned char*)&test;
    if(bytes[0] == 0x12 && bytes[1] == 0x34)
        mSystemBigEndian = true;
    else
        mSystemBigEndian = false;
}

IConnection::~IConnection(void)
{
    return;
}

const ConnectionHandle &IConnection::GetHandle(void) const
{
    return _handle;
}

bool IConnection::IsOpen(void)
{
    return false;
}

DeviceInfo IConnection::GetDeviceInfo(void)
{
    DeviceInfo info;
    info.addrsLMS7002M.push_back(0);
    return info;
}

/***********************************************************************
 * Serial API
 **********************************************************************/

int IConnection::TransactSPI(const int addr, const uint32_t *writeData, uint32_t *readData, const size_t size)
{
    ReportError(ENOTSUP);
    return -1;
}

int IConnection::WriteI2C(const int addr, const std::string &data)
{
    ReportError(ENOTSUP);
    return -1;
}

int IConnection::ReadI2C(const int addr, const size_t numBytes, std::string &data)
{
    ReportError(ENOTSUP);
    return -1;
}

/***********************************************************************
 * LMS7002M Driver callbacks
 **********************************************************************/

int IConnection::DeviceReset(void)
{
    ReportError(ENOTSUP);
    return -1;
}

void IConnection::UpdateExternalBandSelect(const size_t channel, const int trfBand, const int rfePath)
{
    return;
}

int IConnection::UpdateExternalDataRate(const size_t channel, const double txRate, const double rxRate, const double txPhase, const double rxPhase)
{
    return ReportError(EPERM, "UpdateExternalDataRate not implemented");
}

void IConnection::EnterSelfCalibration(const size_t channel)
{
    return;
}

void IConnection::ExitSelfCalibration(const size_t channel)
{
    return;
}

/***********************************************************************
 * Reference clocks API
 **********************************************************************/

double IConnection::GetReferenceClockRate(void)
{
    //this is the populated TCXO on many boards
    return 61.44e6/2;
}

int IConnection::SetReferenceClockRate(const double rate)
{
    return ReportError(EPERM, "SetReferenceClockRate not implemented");
}

double IConnection::GetTxReferenceClockRate(void)
{
    return this->GetReferenceClockRate();
}

int IConnection::SetTxReferenceClockRate(const double rate)
{
    return this->SetReferenceClockRate(rate);
}

/***********************************************************************
 * Timestamp API
 **********************************************************************/

uint64_t IConnection::GetHardwareTimestamp(void)
{
    return 0;
}

void IConnection::SetHardwareTimestamp(const uint64_t now)
{
    return;
}

double IConnection::GetHardwareTimestampRate(void)
{
    return 1.0;
}

/***********************************************************************
 * Stream API
 **********************************************************************/

int IConnection::SetupStream(size_t &streamID, const StreamConfig &config)
{
    streamID = ~0;
    return ReportError(EPERM, "SetupStream not implemented");
}

int IConnection::CloseStream(const size_t streamID)
{
    return ReportError(EPERM, "CloseStream not implemented");
}

size_t IConnection::GetStreamSize(const size_t streamID)
{
    //this should be overloaded, but if not,
    //pick a number that will probably work (power of 2)
    return 16*1024;
}

int IConnection::ControlStream(const size_t streamID, const bool enable)
{
    return ReportError(EPERM, "ControlStream not implemented");
}

int IConnection::ReadStream(const size_t streamID, void* buffs, const size_t length, const long timeout_ms, StreamMetadata &metadata)
{
    return ReportError(EPERM, "ReadStream not implemented");
}

int IConnection::WriteStream(const size_t streamID, const void* buffs, const size_t length, const long timeout_ms, const StreamMetadata &metadata)
{
    return ReportError(EPERM, "WriteStream not implemented");
}

int IConnection::ReadStreamStatus(const size_t streamID, const long timeout_ms, StreamMetadata &metadata)
{
    return ReportError(EPERM, "ReadStreamStatus not implemented");
}

int IConnection::UploadWFM(const void * const* samples, uint8_t chCount, size_t sample_count, StreamConfig::StreamDataFormat format)
{
    return ReportError(EPERM, "UploadTxWFM not implemented");   
}

/** @brief Sets callback function which gets called each time data is sent or received
*/
void IConnection::SetDataLogCallback(std::function<void(bool, const unsigned char*, const unsigned int)> callback)
{
    callback_logData = callback;
}

/***********************************************************************
 * Programming API
 **********************************************************************/

int IConnection::ProgramWrite(const char *buffer, const size_t length, const int programmingMode, const int index, ProgrammingCallback callback)
{
    ReportError(ENOTSUP, "ProgramWrite not supported");
    return -1;
}

int IConnection::ProgramRead(char *buffer, const size_t length, const int index, ProgrammingCallback callback)
{
    ReportError(ENOTSUP, "ProgramRead not supported");
    return -1;
}

int IConnection::ProgramMCU(const uint8_t *buffer, const size_t length,
                const IConnection::MCU_PROG_MODE mode,
                IConnection::ProgrammingCallback callback)
{
#ifndef NDEBUG
    auto timeStart = std::chrono::high_resolution_clock::now();
#endif // NDEBUG
    const auto timeout = std::chrono::milliseconds(100);
    const int LMS_ADDR = 0x10;
    const uint32_t controlAddr = 0x0002 << 16;
    const uint32_t statusReg = 0x0003 << 16;
    const uint32_t addrDTM = 0x0004 << 16; //data to MCU
    const uint16_t EMTPY_WRITE_BUFF = 1 << 0;
    const uint16_t PROGRAMMED = 1 << 6;
    const uint8_t fifoLen = 32;
    uint32_t wrdata[fifoLen];
    uint32_t rddata = 0;
    int status;
    bool abort = false;

    //reset MCU, set mode
    wrdata[0] = controlAddr | 0;
    wrdata[1] = controlAddr | (mode & 0x3);
    if((status = TransactSPI(LMS_ADDR, wrdata, nullptr, 2)))
        return status;

    if(callback)
        abort = callback(0, length, "");

    for(uint16_t i=0; i<length && !abort; i+=fifoLen)
    {
        //wait till EMPTY_WRITE_BUFF = 1
        bool fifoEmpty = false;
        wrdata[0] = statusReg;
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = t1;
        do{
            if((status = TransactSPI(LMS_ADDR, wrdata, &rddata, 1)))
                return status;
            fifoEmpty = rddata & EMTPY_WRITE_BUFF;
            t2 = std::chrono::high_resolution_clock::now();
        }while( not fifoEmpty && (t2-t1)<timeout);

        if(not fifoEmpty)
            return ReportError(ETIMEDOUT, "MCU FIFO full");

        //write 32 bytes into FIFO
        for(uint8_t j=0; j<fifoLen; ++j)
            wrdata[j] = addrDTM | buffer[i+j];
        if((status = TransactSPI(LMS_ADDR, wrdata, nullptr, fifoLen)))
            return status;
        if(callback)
            abort = callback(i+fifoLen, length, "");
#ifndef NDEBUG
        printf("MCU programming : %4i/%4li\r", i+fifoLen, long(length));
#endif
    };
    if(abort)
        return ReportError(-1, "operation aborted by user");

    //wait until programmed flag
    wrdata[0] = statusReg;
    bool programmed = false;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = t1;
    do{
        if((status = TransactSPI(LMS_ADDR, wrdata, &rddata, 1)))
            return status;
        programmed = rddata & PROGRAMMED;
        t2 = std::chrono::high_resolution_clock::now();
    }while( not programmed && (t2-t1)<timeout);

    if(not programmed)
        return ReportError(ETIMEDOUT, "MCU not programmed");
#ifndef NDEBUG
    auto timeEnd = std::chrono::high_resolution_clock::now();
    printf("\nMCU Programming finished, %li ms\n",
            std::chrono::duration_cast<std::chrono::milliseconds>
            (timeEnd-timeStart).count());
#endif //NDEBUG
    return 0;
}

/***********************************************************************
 * GPIO API
 **********************************************************************/

int IConnection::GPIOWrite(const uint8_t *buffer, const size_t bufLength)
{
    ReportError(ENOTSUP, "GPIOWrite not supported");
    return -1;
}

int IConnection::GPIORead(uint8_t *buffer, const size_t bufLength)
{
    ReportError(ENOTSUP, "GPIORead not supported");
    return -1;
}

/***********************************************************************
 * Register API
 **********************************************************************/

int IConnection::WriteRegisters(const uint32_t *addrs, const uint32_t *data, const size_t size)
{
    ReportError(ENOTSUP, "WriteRegisters not supported");
    return -1;
}

int IConnection::ReadRegisters(const uint32_t *addrs, uint32_t *data, const size_t size)
{
    ReportError(ENOTSUP, "ReadRegisters not supported");
    return -1;
}

int IConnection::WriteRegister(const uint32_t addr, const uint32_t data)
{
    return this->WriteRegisters(&addr, &data, 1);
}

/***********************************************************************
 * Aribtrary settings API
 **********************************************************************/

int IConnection::CustomParameterWrite(const uint8_t *ids, const double *values, const size_t count, const std::string* units)
{
    ReportError(ENOTSUP, "CustomParameterWrite not supported");
    return -1;
}

int IConnection::CustomParameterRead(const uint8_t *ids, double *values, const size_t count, std::string* units)
{
    ReportError(ENOTSUP, "CustomParameterRead not supported");
    return -1;
}
