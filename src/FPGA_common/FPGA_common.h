/**
@file FPGA_common.h
@author Lime Microsystems
@brief Common functions used to work with FPGA
*/

#ifndef FPGA_COMMON_H
#define FPGA_COMMON_H
#include "IConnection.h"
#include <stdint.h>
#include <dataTypes.h>

namespace lime
{
namespace fpga
{

int StartStreaming(IConnection* serPort, unsigned endpointIndex = 0);
int StopStreaming(IConnection* serPort, unsigned endpointIndex = 0);
int ResetTimestamp(IConnection* serPort, unsigned endpointIndex = 0);

struct FPGA_PLL_clock
{
    double outFrequency;
    double phaseShift_deg;
    uint8_t index;
    bool bypass;
    double rd_actualFrequency;
};

int SetPllFrequency(IConnection* serPort, uint8_t pllIndex, const double inputFreq, FPGA_PLL_clock* outputs, const uint8_t clockCount);
int SetDirectClocking(IConnection* serPort, uint8_t clockIndex, const double inputFreq, const double phaseShift_deg);

int FPGAPacketPayload2Samples(const uint8_t* buffer, const size_t bufLen, const size_t chCount, const int format, complex16_t** samples, size_t* samplesCount);
int Samples2FPGAPacketPayload(const complex16_t* const* samples, const size_t samplesCount, const size_t chCount, const int format, uint8_t* buffer, size_t* bufLen);

}

}
#endif // FPGA_COMMON_H
