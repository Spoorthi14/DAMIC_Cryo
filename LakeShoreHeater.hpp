/* **************************************************************************************
 * Code containing the CCD class that will have the entire definitions we will need
 * such as the erase procedure, the set biases and the CCD tuning etc
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/


#ifndef Lakeshore_HPP_INCLUDED
#define Lakeshore_HPP_INCLUDED


/*Includes*/
#include <iostream>
#include "SerialDeviceT.hpp"

class LakeShore : public SerialDevice {
public:
    LakeShore(std::string );
    ~LakeShore();
    void ReadPower();
    void ReadMode(void );
    void SetPowerLevel(float PW);
    void TurnONOFF(int );
    void UpdateMysql(void);
    void ReadTemperatureK(void);
    void ReadSetPointTempK(void);
    void ReadRampRateKPerMin(void);
    void SetTemperature(float temp);
    void SetRampingRate(float rampr);
    void SetMode(int mode);

    float currentPW;
    float currentTempK;
    float setPW;
    int currentMode;
    int setMode;
    bool WatchdogFuse;
    std::string SQLStatusMsg;
    float SetPointTempK;
    float RampRateKPerMin;

    float SetRampRate;
    float SetNewSetPointTempK;
   // int SetMode;


    int _cHeaterMode, _cWatchdogFuse;


};

#endif
