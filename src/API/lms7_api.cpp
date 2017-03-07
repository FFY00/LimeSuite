#include "IConnection.h"
#include "ConnectionRegistry.h"
#include "lms_gfir.h"
#include "lime/LimeSuite.h"
#include "lms7_device.h"
#include "ErrorReporting.h"
#include "errno.h"
#include "MCU_BD.h"
#include "Si5351C.h"
#include "ADF4002.h"
#include <cmath>
#include "VersionInfo.h"
#include <assert.h>
#include "LMS64CProtocol.h"
#include "FPGA_common.h"

using namespace std;


API_EXPORT int CALL_CONV LMS_GetDeviceList(lms_info_str_t * dev_list)
{
    std::vector<lime::ConnectionHandle> handles;
    handles = lime::ConnectionRegistry::findConnections();

    if (dev_list != nullptr)
    {
        for (size_t i = 0; i < handles.size(); i++)
        {
            string str = handles[i].serialize();
            if (dev_list[i] == nullptr)
                return -1;
            strncpy(dev_list[i],str.c_str(),sizeof(lms_info_str_t)-1);
            dev_list[i][sizeof(lms_info_str_t)-1]=0;
        }
    }
    return handles.size();
}




API_EXPORT int CALL_CONV LMS_Open(lms_device_t** device, lms_info_str_t info, void* args)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device pointer cannot be NULL");
        return -1;
    }

    std::vector<lime::ConnectionHandle> handles;
    handles = lime::ConnectionRegistry::findConnections();
    LMS7_Device* lms = (LMS7_Device*)*device;
    if (lms != nullptr)
        lms->SetConnection(nullptr);

    for (size_t i = 0; i < handles.size(); i++)
    {
        if (info == NULL || strcmp(handles[i].serialize().c_str(),info) == 0)
        {
            auto conn = lime::ConnectionRegistry::makeConnection(handles[i]);
            if (conn->IsOpen() == false)
            {
                if (info != NULL)
                {
                    lime::ReportError(EBUSY, "Failed to open. Device is busy.");
                    *device = nullptr;
                    return -1;
                }
                else
                    continue;
            }
            *device = LMS7_Device::CreateDevice(conn,lms);
            return LMS_SUCCESS;
        }
    }

    if (info == NULL)
    {
        *device = LMS7_Device::CreateDevice(nullptr);
        return LMS_SUCCESS;
    }

    lime::ReportError(ENODEV, "Specified device could not be found");
    *device = nullptr;
    return -1;
}

API_EXPORT int CALL_CONV LMS_Close(lms_device_t * device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS_Disconnect(device);
    LMS7_Device* lms = (LMS7_Device*)device;
    delete lms;
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_Disconnect(lms_device_t *device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->SetConnection(nullptr);
}

API_EXPORT bool CALL_CONV LMS_IsOpen(lms_device_t *device, int port)
{
    if (device == nullptr)
        return false;

    LMS7_Device* lms = (LMS7_Device*)device;

    auto conn = lms->GetConnection();
    if (conn != nullptr)
    {
        return conn->IsOpen();
    }
    return false;
}

API_EXPORT int CALL_CONV LMS_Reset(lms_device_t *device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->Reset();
}

API_EXPORT int CALL_CONV LMS_EnableChannel(lms_device_t * device, bool dir_tx, size_t chan, bool enabled)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->EnableChannel(dir_tx, chan,enabled);
}



API_EXPORT int CALL_CONV LMS_SetSampleRate(lms_device_t * device, float_type rate, size_t oversample)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

   LMS7_Device* lms = (LMS7_Device*)device;

   return lms->SetRate(rate, oversample);
}

API_EXPORT int CALL_CONV LMS_SetSampleRateDir(lms_device_t *device, bool dir_tx, float_type rate, size_t oversample)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->SetRate(dir_tx,rate,oversample);
}


API_EXPORT int CALL_CONV LMS_GetSampleRate(lms_device_t *device, bool dir_tx, size_t chan, float_type *host_Hz, float_type *rf_Hz)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    double rate = lms->GetRate(dir_tx,chan,rf_Hz);
    if (host_Hz)
        *host_Hz = rate;

    return LMS_SUCCESS;

}

API_EXPORT int CALL_CONV LMS_GetSampleRateRange(lms_device_t *device, bool dir_tx, lms_range_t *range)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (dir_tx)
        *range = lms->GetRxRateRange();
    else
        *range = lms->GetTxRateRange();
    return LMS_SUCCESS;
}



API_EXPORT int CALL_CONV LMS_Init(lms_device_t * device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->Init();
}

API_EXPORT int CALL_CONV LMS_SetReferenceClock(lms_device_t *device, float_type clock_Hz)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->SetReferenceClock(clock_Hz);
}

API_EXPORT int CALL_CONV LMS_GetReferenceClock(lms_device_t * device, float_type * clock_Hz)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->GetClockFreq(LMS_CLOCK_REF,clock_Hz);
}

API_EXPORT int CALL_CONV LMS_ReadCustomBoardParam(lms_device_t *device,
                           uint8_t param_id, float_type *val, lms_name_t units)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    std::string str;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    int ret=conn->CustomParameterRead(&param_id,val,1,&str);
    strncpy(units,str.c_str(),sizeof(lms_name_t)-1);
    return ret;
}

API_EXPORT int CALL_CONV LMS_WriteCustomBoardParam(lms_device_t *device,
                        uint8_t param_id, float_type val, const lms_name_t units)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    std::string str = units == nullptr ? "":units;
    return conn->CustomParameterWrite(&param_id,&val,1,&str);
}

API_EXPORT int CALL_CONV LMS_VCTCXOWrite(lms_device_t * device, uint16_t val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->DACWrite(val);
}

API_EXPORT int CALL_CONV LMS_VCTCXORead(lms_device_t * device, uint16_t *val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    int ret = lms->DACRead();
    *val = ret;
    return ret < 0 ? -1 : 0;
}

API_EXPORT int CALL_CONV LMS_GetVCORange(lms_device_t * device, size_t vco_id, lms_range_t* range)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->GetVCORange(vco_id, range);
}

API_EXPORT int CALL_CONV LMS_SetVCORange(lms_device_t * device, size_t vco_id, lms_range_t range)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->SetVCORange(vco_id, range);
}

API_EXPORT int CALL_CONV LMS_SetDataLogCallback(lms_device_t *dev, void (*func)(bool, const unsigned char*, const unsigned int))
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)dev;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    conn->SetDataLogCallback(func);
    return 0;
}

API_EXPORT int CALL_CONV LMS_SetLogCallback(lms_device_t *dev, void (*func)(const char* cstr, const unsigned int type))
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)dev;
    lms->SetLogCallback(func);
    return 0;
}

API_EXPORT int CALL_CONV LMS_GetClockFreq(lms_device_t *device, size_t clk_id, float_type *freq)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->GetClockFreq(clk_id, freq);
}

API_EXPORT int CALL_CONV LMS_SetClockFreq(lms_device_t *device, size_t clk_id, float_type freq)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->SetClockFreq(clk_id, freq);
}

API_EXPORT int CALL_CONV LMS_LoadConfigSi5351C(lms_device_t *dev, const char* filename)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)dev;

    lime::Si5351C obj;
    obj.Initialize(lms->GetConnection());
    obj.LoadRegValuesFromFile(std::string(filename));
    return obj.UploadConfiguration();
}

API_EXPORT int CALL_CONV LMS_ConfigureSi5351C(lms_device_t *dev, float_type clkin,  float_type *clks, unsigned src)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)dev;
    lime::Si5351C obj;
    obj.Initialize(lms->GetConnection());
    if (clks != nullptr)
    {
        obj.SetPLL(0,clkin,src);
        obj.SetPLL(1,clkin,src);

        for (int i = 0; i < 8;i++)
        {
            unsigned clock = abs(clks[i]);
            obj.SetClock(i,clock,clock!=0,clks[i]<0);
        }

        if (obj.ConfigureClocks()!=0)
            return -1;
    }
    else obj.Reset();

    return obj.UploadConfiguration();
}

API_EXPORT int CALL_CONV LMS_StatusSi5351C(lms_device_t *dev, uint32_t *status)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)dev;
    lime::Si5351C obj;
    obj.Initialize(lms->GetConnection());
    if (status != nullptr)
    {
        lime::Si5351C::StatusBits stat = obj.GetStatusBits();
        *status = 0;
        *status |= stat.lol_a;
        *status |= stat.lol_a_stky<<1;
        *status |= stat.lol_b<<2;
        *status |= stat.lol_b_stky<<3;
        *status |= stat.los<<4;
        *status |= stat.los_stky<<5;
        *status |= stat.sys_init<<6;
        *status |= stat.sys_init_stky<<7;
        return 0;
    }
    return obj.ClearStatus();
}

API_EXPORT int CALL_CONV LMS_ConfigureADF4002(lms_device_t *dev, lms_adf4002_conf_t *config)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)dev;
    lime::ADF4002 obj;
    auto serPort = lms->GetConnection();
    if (serPort == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }

    //reference counter latch
    obj.SetReferenceCounterLatch(config->lockDetectPrec, config->antiBacklash, config->referenceCounter);

    //n counter latch
    obj.SetNCounterLatch(config->cpGain, config->nCounter);

    //function latch
    obj.SetFunctionLatch(config->flCurrent1, config->flCurrent2, config->flTimerCounter, config->flFastlock, config->flMuxCtrl);
    obj.SetFunctionLatchRgr(config->flPDPolarity, config->flPD1, config->flPD2, config->flCounterReset, config->flCPState);

    //Initialization latch
    obj.SetInitializationLatch(config->ilCurrent1, config->ilCurrent2, config->ilTimerCounter, config->ilFastlock, config->ilMuxCtrl);
    obj.SetInitializationLatchRgr(config->ilPDPolarity, config->ilPD1, config->ilPD2, config->ilCounterReset, config->ilCPState);

    if (config->fRef > 0 && config->fVCO)
    {
      int rcnt;
      int ncnt;
      obj.SetFrefFvco(config->fRef, config->fVCO, rcnt, ncnt);
    }

    config->fRef = obj.lblFcomp;
    config->fVCO = obj.lblFvco;

    unsigned char data[12];
    obj.GetConfig(data);

    vector<uint32_t> dataWr;
    for(int i=0; i<12; i+=3)
        dataWr.push_back((uint32_t)data[i] << 16 | (uint32_t)data[i+1] << 8 | data[i+2]);

    int status;
    // ADF4002 needs to be writen 4 values of 24 bits
    int adf4002SpiAddr = serPort->GetDeviceInfo().addrADF4002;
    status = serPort->TransactSPI(adf4002SpiAddr, dataWr.data(), nullptr, 4);
    return status;
}

API_EXPORT  int CALL_CONV LMS_Synchronize(lms_device_t *dev, bool toChip)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
     LMS7_Device* lms = (LMS7_Device*)dev;

     return lms->Synchronize(toChip);
}


API_EXPORT int CALL_CONV LMS_GPIORead(lms_device_t *dev,  uint8_t* buffer, size_t len)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)dev;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    return conn->GPIORead(buffer,len);
}


API_EXPORT int CALL_CONV LMS_GPIOWrite(lms_device_t *dev, const uint8_t* buffer, size_t len)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
     LMS7_Device* lms = (LMS7_Device*)dev;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
       lime::ReportError(EINVAL, "Device not connected");
       return -1;
    }
     return conn->GPIOWrite(buffer,len);
}

API_EXPORT int CALL_CONV LMS_TransferLMS64C(lms_device_t *dev, int cmd, uint8_t* data, size_t *len)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)dev;
    lime::LMS64CProtocol::GenericPacket pkt;

    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }

    pkt.cmd = lime::eCMD_LMS(cmd);
    for (size_t i = 0; i < *len; ++i)
        pkt.outBuffer.push_back(data[i]);
    lime::LMS64CProtocol* port = dynamic_cast<lime::LMS64CProtocol *>(conn);
    if (port->TransferPacket(pkt) != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < pkt.inBuffer.size(); ++i)
        data[i] = pkt.inBuffer[i];
    *len = pkt.inBuffer.size();

    if(pkt.status != lime::STATUS_COMPLETED_CMD)
    {
        return lime::ReportError(-1, "%s", lime::status2string(pkt.status));
    }
    return 0;
}


API_EXPORT int CALL_CONV LMS_EnableCalibCache(lms_device_t *dev, bool enable)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)dev;
    return lms->EnableCalibCache(enable);
}

API_EXPORT int CALL_CONV LMS_GetChipTemperature(lms_device_t *dev, size_t ind, float_type *temp)
{
    if (dev == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)dev;
    return lms->GetChipTemperature(ind,temp);
}

API_EXPORT int CALL_CONV LMS_GetNumChannels(lms_device_t * device, bool dir_tx)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->GetNumChannels(dir_tx);
}


API_EXPORT int CALL_CONV LMS_SetLOFrequency(lms_device_t *device, bool dir_tx, size_t chan, float_type frequency)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(false))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    if (dir_tx)
    {
        if (lms->SetTxFrequency(chan,frequency)!=0)
            return -1;
    }
    else
    {
        if (lms->SetRxFrequency(chan,frequency)!=0)
            return -1;
    }

    return LMS_SUCCESS;
}


API_EXPORT int CALL_CONV LMS_GetLOFrequency(lms_device_t *device, bool dir_tx, size_t chan, float_type *frequency)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    *frequency = lms->GetTRXFrequency(dir_tx, chan);
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_GetLOFrequencyRange(lms_device_t *device, bool dir_tx, lms_range_t *range)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    *range = lms->GetFrequencyRange(dir_tx);
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_GetAntennaList(lms_device_t *device, bool dir_tx, size_t chan, lms_name_t *list)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(false))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    auto names = lms->GetPathNames(dir_tx, chan);
    if (list != nullptr)
        for (size_t i = 0; i < names.size(); i++)
        {
            strncpy(list[i], names[i].c_str(), sizeof(lms_name_t) - 1);
            list[i][sizeof(lms_name_t) - 1] = 0;
        }
    return names.size();
}

API_EXPORT int CALL_CONV LMS_SetAntenna(lms_device_t *device, bool dir_tx, size_t chan, size_t path)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(false))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    lms->SetPath(dir_tx,chan,path);

    return LMS_SUCCESS;
}


API_EXPORT int CALL_CONV LMS_GetAntenna(lms_device_t *device, bool dir_tx, size_t chan)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->GetPath(dir_tx, chan);
}



API_EXPORT int CALL_CONV LMS_GetAntennaBW(lms_device_t *device, bool dir_tx, size_t chan, size_t path, lms_range_t *range)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    if (dir_tx)
        *range = lms->GetTxPathBand(path,chan);
    else
        *range = lms->GetRxPathBand(path,chan);

    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_SetLPFBWFixed(lms_device_t *device, bool dir_tx,
                                             size_t chan, LPF_FixedBW bandwidth)
{
    if (device == nullptr)
        return lime::ReportError(EINVAL, "Device cannot be NULL.");
    LMS7_Device* lms = (LMS7_Device*)device;
    if (chan >= lms->GetNumChannels(dir_tx))
        return lime::ReportError(EINVAL, "Invalid channel number.");
    return lms->SetLPF_Fixed(dir_tx, chan, bandwidth);
}

API_EXPORT int CALL_CONV LMS_SetLPFBW(lms_device_t *device, bool dir_tx, size_t chan, float_type bandwidth)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }
    return lms->SetLPF(dir_tx,chan,true,true,bandwidth);
}

API_EXPORT int CALL_CONV LMS_GetLPFBW(lms_device_t *device, bool dir_tx, size_t chan, float_type *bandwidth)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }
    *bandwidth = lms->GetLPFBW(dir_tx,chan,true);
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_SetLPF(lms_device_t *device, bool dir_tx, size_t chan, bool enabled)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }
    return lms->SetLPF(dir_tx,chan,true,enabled,1);
}


API_EXPORT int CALL_CONV LMS_SetGFIRLPF(lms_device_t *device, bool dir_tx, size_t chan, bool enabled, float_type bandwidth)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->SetLPF(dir_tx,chan,false,enabled,bandwidth);
}

API_EXPORT int CALL_CONV LMS_GetLPFBWRange(lms_device_t *device, bool dir_tx, lms_range_t *range)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    *range = lms->GetLPFRange(dir_tx,0,true);

    return LMS_SUCCESS;
}


API_EXPORT int CALL_CONV LMS_SetNormalizedGain(lms_device_t *device, bool dir_tx, size_t chan, float_type gain)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    if (gain > 1.0)
        gain = 1.0;
    else if (gain < 0)
        gain = 0;

   return lms->SetNormalizedGain(dir_tx,chan,gain);
}

API_EXPORT int CALL_CONV LMS_GetNormalizedGain(lms_device_t *device, bool dir_tx, size_t chan,float_type *gain)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    *gain = lms->GetNormalizedGain(dir_tx,chan);
    if (gain < 0)
        return -1;
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_Calibrate(lms_device_t *device, bool dir_tx, size_t chan, double bw, unsigned flags)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->Calibrate(dir_tx, chan, bw, flags);
}

API_EXPORT int CALL_CONV LMS_LoadConfig(lms_device_t *device, const char *filename)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->LoadConfig(filename);
}

API_EXPORT int CALL_CONV LMS_SaveConfig(lms_device_t *device, const char *filename)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->SaveConfig(filename);
}
API_EXPORT int CALL_CONV LMS_SetTestSignal(lms_device_t *device, bool dir_tx, size_t chan, lms_testsig_t sig, int16_t dc_i, int16_t dc_q)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    if (sig > LMS_TESTSIG_DC)
    {
        lime::ReportError(EINVAL, "Invalid signal.");
        return -1;
    }

    lms->SetTestSignal(dir_tx,chan,sig,dc_i,dc_q);

    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_GetTestSignal(lms_device_t *device, bool dir_tx, size_t chan, lms_testsig_t *sig)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    int tmp = lms->GetTestSignal(dir_tx,chan);
    if (tmp < 0)
        return -1;

    *sig = (lms_testsig_t)tmp;
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_SetNCOFrequency(lms_device_t *device, bool dir_tx, size_t ch, const float_type *freq, float_type pho)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (ch >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->SetNCOFreq(dir_tx, ch,freq, pho);
}


API_EXPORT int CALL_CONV LMS_GetNCOFrequency(lms_device_t *device, bool dir_tx, size_t chan, float_type *freq, float_type *pho)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->GetNCOFreq(dir_tx,chan,freq,pho);
}

API_EXPORT int CALL_CONV LMS_SetNCOPhase(lms_device_t *device, bool dir_tx, size_t ch, const float_type *phase, float_type fcw)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (ch >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->SetNCOPhase(dir_tx, ch,phase, fcw);
}


API_EXPORT int CALL_CONV LMS_GetNCOPhase(lms_device_t *device, bool dir_tx, size_t ch, size_t index, float_type *phase, float_type *fcw)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (ch >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->GetNCOPhase(dir_tx,ch,phase,fcw);
}

API_EXPORT int CALL_CONV LMS_SetNCOIndex(lms_device_t *device, bool dir_tx, size_t chan, size_t index,bool down)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->SetNCO(dir_tx,chan,index,down);
}

API_EXPORT int CALL_CONV LMS_GetNCOIndex(lms_device_t *device, bool dir_tx, size_t chan)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(dir_tx))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->GetNCO(dir_tx, chan);
}

API_EXPORT int CALL_CONV LMS_GenerateLPFCoef(size_t n, float_type w1, float_type w2, float_type g_stop, float_type *coef)
{
    if (w1 > 0.5 || w1 < 0 || w2 > 0.5 || w2 < 0)
    {
        lime::ReportError(EINVAL, "Normalized frequency (w1 or w2) out of range [0, 0.5]");
           return -1;
    }

    if (g_stop > 0.5 || g_stop < 0)
    {
        lime::ReportError(EINVAL, "stop band gain (g_stop )out of range [0, 0.5]");
           return -1;
    }

    float_type a1,a2;
    if (w1 < w2)
    {
        a1 = 1.0;
        a2 = g_stop;
    }
    else
    {
        a2 = 1.0;
        a1 = g_stop;
    }

    GenerateFilter(n, w1, w2, a1, a2, coef);
    return LMS_SUCCESS;

}

API_EXPORT int CALL_CONV LMS_ReadLMSReg(lms_device_t *device, uint32_t address, uint16_t *val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->ReadLMSReg(address, val);
}

API_EXPORT int CALL_CONV LMS_WriteLMSReg(lms_device_t *device, uint32_t address, uint16_t val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->WriteLMSReg(address, val);
}

API_EXPORT int CALL_CONV LMS_RegisterTest(lms_device_t *device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->RegisterTest();
}


API_EXPORT int CALL_CONV LMS_ReadFPGAReg(lms_device_t *device, uint32_t address, uint16_t *val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    uint32_t addr = address;
    uint32_t data;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    *val = conn->ReadRegisters(&addr,&data,1);
    if (*val != LMS_SUCCESS)
        return *val;
    *val = data;
    return LMS_SUCCESS;
}

API_EXPORT int CALL_CONV LMS_WriteFPGAReg(lms_device_t *device, uint32_t address, uint16_t val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    return conn->WriteRegister(address,val);
}

API_EXPORT int CALL_CONV LMS_ReadParam(lms_device_t *device, struct LMS7Parameter param, uint16_t *val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->ReadParam(param,val);
}



API_EXPORT int CALL_CONV LMS_WriteParam(lms_device_t *device, struct LMS7Parameter param, uint16_t val)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->WriteParam(param, val);
}

API_EXPORT int CALL_CONV LMS_SetGFIRCoeff(lms_device_t * device, bool dir_tx, size_t chan, lms_gfir_t filt, const float_type* coef,size_t count)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(false))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }
    return lms->SetGFIRCoef(dir_tx,chan,filt,coef,count);
}

API_EXPORT int CALL_CONV LMS_GetGFIRCoeff(lms_device_t * device, bool dir_tx, size_t chan, lms_gfir_t filt, float_type* coef)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(false))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }
    return lms->GetGFIRCoef(dir_tx,chan,filt,coef);
}
API_EXPORT int CALL_CONV LMS_SetGFIR(lms_device_t * device, bool dir_tx, size_t chan, lms_gfir_t filt, bool enabled)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }
    LMS7_Device* lms = (LMS7_Device*)device;

    if (chan >= lms->GetNumChannels(false))
    {
        lime::ReportError(EINVAL, "Invalid channel number.");
        return -1;
    }

    return lms->SetGFIR(dir_tx,chan,filt,enabled);
}


/**
 * @brief Initializes samples streaming channels with given parameters
 * @param[in]   device LMS device to be used for streaming
 * @param[out]  stream Handle to be used with LMS_RecvStream()/LMS_SendStream()
 * @return      0-success
*/
API_EXPORT int CALL_CONV LMS_SetupStream(lms_device_t *device, lms_stream_t *stream)
{
    if(device == nullptr)
        return lime::ReportError(EINVAL, "Device is NULL.");
    if(stream == nullptr)
        return lime::ReportError(EINVAL, "stream is NULL.");

    LMS7_Device* lms = (LMS7_Device*)device;

    lime::StreamConfig config;
    config.bufferLength = 65536;
    config.channelID = stream->channel;
    config.performanceLatency = stream->throughputVsLatency;
    switch(stream->dataFmt)
    {
        case lms_stream_t::LMS_FMT_F32:
            config.format = lime::StreamConfig::STREAM_COMPLEX_FLOAT32;
            break;
        case lms_stream_t::LMS_FMT_I16:
            config.format = lime::StreamConfig::STREAM_12_BIT_IN_16;
            break;
        case lms_stream_t::LMS_FMT_I12:
            config.format = lime::StreamConfig::STREAM_12_BIT_COMPRESSED;
            break;
        default:
            config.format = lime::StreamConfig::STREAM_COMPLEX_FLOAT32;
    }
    config.isTx = stream->isTx;
    return lms->GetConnection(stream->channel)->SetupStream(stream->handle, config);
}

API_EXPORT int CALL_CONV LMS_DestroyStream(lms_device_t *device, lms_stream_t *stream)
{
    if(stream == nullptr)
        return lime::ReportError(EINVAL, "stream is NULL.");

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->GetConnection(stream->channel)->CloseStream(stream->handle);
}

API_EXPORT int CALL_CONV LMS_StartStream(lms_stream_t *stream)
{
    assert(stream != nullptr);
    return reinterpret_cast<lime::IStreamChannel*>(stream->handle)->Start();
}

API_EXPORT int CALL_CONV LMS_StopStream(lms_stream_t *stream)
{
    assert(stream != nullptr);
    return reinterpret_cast<lime::IStreamChannel*>(stream->handle)->Stop();
}

API_EXPORT int CALL_CONV LMS_RecvStream(lms_stream_t *stream, void *samples, size_t sample_count, lms_stream_meta_t *meta, unsigned timeout_ms)
{
    assert(stream != nullptr);
    lime::IStreamChannel* channel = (lime::IStreamChannel*)stream->handle;
    assert(channel != nullptr);
    lime::IStreamChannel::Metadata metadata;
    metadata.flags = 0;
    if (meta)
    {
        metadata.flags |= meta->waitForTimestamp * lime::IStreamChannel::Metadata::SYNC_TIMESTAMP;
        metadata.timestamp = meta->timestamp;
    }
    else metadata.timestamp = 0;

    int status = channel->Read(samples, sample_count, &metadata, timeout_ms);
    if (meta)
        meta->timestamp = metadata.timestamp;
    return status;
}

API_EXPORT int CALL_CONV LMS_SendStream(lms_stream_t *stream, const void *samples, size_t sample_count, const lms_stream_meta_t *meta, unsigned timeout_ms)
{
    assert(stream != nullptr);
    lime::IStreamChannel* channel = (lime::IStreamChannel*)stream->handle;
    assert(channel != nullptr);
    lime::IStreamChannel::Metadata metadata;
    metadata.flags = 0;
    if (meta)
    {
        metadata.flags |= meta->waitForTimestamp * lime::IStreamChannel::Metadata::SYNC_TIMESTAMP;
        metadata.timestamp = meta->timestamp;
    }
    else metadata.timestamp = 0;

    return channel->Write(samples, sample_count, &metadata, timeout_ms);
}

API_EXPORT int CALL_CONV LMS_UploadWFM(lms_device_t *device,
                                         const void **samples, uint8_t chCount,
                                         size_t sample_count, int format)
{
    LMS7_Device* lms = (LMS7_Device*)device;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return -1;
    }
    lime::StreamConfig::StreamDataFormat fmt;
    switch(format)
    {
        case 0:
            fmt = lime::StreamConfig::StreamDataFormat::STREAM_12_BIT_COMPRESSED;
            break;
        case 1:
            fmt = lime::StreamConfig::StreamDataFormat::STREAM_12_BIT_IN_16;
            break;
        case 2:
            fmt = lime::StreamConfig::StreamDataFormat::STREAM_COMPLEX_FLOAT32;
            break;
    }
    return conn->UploadWFM(samples, chCount, sample_count, fmt);
}

API_EXPORT int CALL_CONV LMS_EnableTxWFM(lms_device_t *device, const bool active)
{
    uint16_t regAddr = 0x000D;
    uint16_t regValue = 0;
    int status = 0;
    status = LMS_ReadFPGAReg(device, regAddr, &regValue);
    if(status != 0)
        return status;
    regValue = regValue & ~0x0006; //clear WFM_LOAD, WFM_PLAY
    regValue |= (active << 1);
    status = LMS_WriteFPGAReg(device, regAddr, regValue);
    return status;
}

API_EXPORT int CALL_CONV LMS_GetStreamStatus(lms_stream_t *stream, lms_stream_status_t* status)
{
    assert(stream != nullptr);
    lime::IStreamChannel* channel = (lime::IStreamChannel*)stream->handle;
    if(channel == nullptr)
        return -1;
    lime::IStreamChannel::Info info = channel->GetInfo();

    status->active = false;
    status->droppedPackets = 0;
    status->fifoFilledCount = info.fifoItemsCount;
    status->fifoSize = info.fifoSize;
    status->linkRate = info.linkRate;
    status->overrun = 0;
    status->underrun = 0;
    status->sampleRate = 0;
    status->timestamp = 0;
    return 0;
}


API_EXPORT const lms_dev_info_t* CALL_CONV LMS_GetDeviceInfo(lms_device_t *device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return nullptr;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    auto conn = lms->GetConnection();
    if (conn == nullptr)
    {
        lime::ReportError(EINVAL, "Device not connected");
        return nullptr;
    }
    if (!conn->IsOpen())
    {
       lime::ReportError(EINVAL, "No cennection to board.");
	   return nullptr;
    }

    return lms->GetInfo();
}

API_EXPORT int CALL_CONV LMS_ProgramFPGA(lms_device_t *device, const char *data,
                                            size_t size, lms_target_t target, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->ProgramFPGA(data,size,target, callback);
}

API_EXPORT int CALL_CONV LMS_ProgramFPGAFile(lms_device_t *device,
                                        const char *file, lms_target_t target, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    std::string str = file;
    return lms->ProgramFPGA(str,target, callback);
}


API_EXPORT int CALL_CONV LMS_ProgramFirmware(lms_device_t *device, const char *data,
                                            size_t size, lms_target_t target, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->ProgramFW(data,size,target,callback);
}


API_EXPORT int CALL_CONV LMS_ProgramFirmwareFile(lms_device_t *device,
                                         const char *file, lms_target_t target, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->ProgramFW(file,target,callback);
}


API_EXPORT int CALL_CONV LMS_ProgramHPM7(lms_device_t *device, const char *data, size_t size, unsigned mode, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->ProgramHPM7(data,size,mode,callback);
}


API_EXPORT int CALL_CONV LMS_ProgramHPM7File(lms_device_t *device, const char *file, unsigned mode, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->ProgramHPM7(file,mode,callback);
}

API_EXPORT int CALL_CONV LMS_ProgramLMSMCU(lms_device_t *device, const char *data,
                                              size_t size, lms_target_t target, lms_prog_callback_t callback)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;

    return lms->ProgramMCU(data,size,target,callback);
}

API_EXPORT int CALL_CONV LMS_ResetLMSMCU(lms_device_t *device)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    lime::MCU_BD mcu;
    mcu.Initialize(lms->GetConnection());
    mcu.Reset_MCU();
    return 0;
}

API_EXPORT int CALL_CONV LMS_SetActiveChipID(lms_device_t *device, unsigned id)
{
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->SetActiveChip(id);
}

API_EXPORT int CALL_CONV LMS_ReadDPDBuffer(lms_device_t *device, char* buffer, unsigned length)
{
    LMS7_Device* lms = (LMS7_Device*)device;
    return lms->GetConnection()->ReadDPDBuffer(buffer, length);
}

API_EXPORT int CALL_CONV LMS_QSparkConfigPLL(lms_device_t *device, double *freqRxMHz, double *freqTxMHz)
{
    LMS7_Device* lms = (LMS7_Device*)device;
    lime::fpga::FPGA_PLL_clock clocks[3];
    //ADC
    clocks[0].bypass = false;
    clocks[0].index = 0;
    clocks[0].outFrequency = *freqRxMHz;
    clocks[0].phaseShift_deg = 0;
    //DAC
    clocks[1].bypass = false;
    clocks[1].index = 1;
    clocks[1].outFrequency = *freqTxMHz;
    clocks[1].phaseShift_deg = 0;
    //DAC
    clocks[2].bypass = false;
    clocks[2].index = 2;
    clocks[2].outFrequency = (*freqTxMHz)*2;
    clocks[2].phaseShift_deg = 0;


    if (lime::fpga::SetPllFrequency(lms->GetConnection(), 4, 30.72e6, clocks, 3) != 0)
        return -1;
    lms->extra_parameters["ADC_MHz"] = clocks[0].rd_actualFrequency;
    lms->extra_parameters["DAC_MHz"] = clocks[1].rd_actualFrequency;
    *freqTxMHz = clocks[1].rd_actualFrequency;
    *freqRxMHz = clocks[0].rd_actualFrequency;
    return 0;
}

API_EXPORT int CALL_CONV LMS_GetExtraParam(lms_device_t *device, const char* name, double* value)
{
    if (device == nullptr)
    {
        lime::ReportError(EINVAL, "Device cannot be NULL.");
        return -1;
    }

    LMS7_Device* lms = (LMS7_Device*)device;
    try
    {
        *value = lms->extra_parameters.at(std::string(name));
    }
    catch(...)
    {
        lime::ReportError(EINVAL, "Invalid parameter");
        return -1;
    }
    
    return 0;
}


API_EXPORT int CALL_CONV LMS_GetLastError()
{
    return lime::GetLastError();
}

API_EXPORT const char * CALL_CONV LMS_GetLastErrorMessage(void)
{
    return lime::GetLastErrorMessage();
}



static char buildTimestamp[32];
API_EXPORT const char* LMS_GetBuildTimestamp()
{
    sprintf(buildTimestamp, "%.32s", lime::GetBuildTimestamp().c_str());
    return buildTimestamp;
}

static char libraryVersion[32];
API_EXPORT const char* LMS_GetLibraryVersion()
{
    sprintf(libraryVersion, "%.32s", lime::GetLibraryVersion().c_str());
    return libraryVersion;
}
