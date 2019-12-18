/* **************************************************************************************
 * Code containing the SerialDevice class that will have the definitions we will need
 * for talking to the Cryocooler and Heater
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/

#include <iostream>

/*For Serial IO*/
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <unistd.h>

#include "SerialDeviceT.hpp"
#include "LakeShoreHeater.hpp"
#include "MysqlCredentials.hpp"


#include <mysqlx/xdevapi.h>



LakeShore::LakeShore(std::string SerialPort) : SerialDevice(SerialPort){


    /* Set Baud Rate */
    cfsetospeed (&this->tty, (speed_t)B1200);
    cfsetispeed (&this->tty, (speed_t)B300);

    /* Setting other Port Stuff */
    tty.c_cflag     |= PARENB;
    tty.c_cflag     |= PARODD;
    tty.c_cflag     |= CMSPAR;
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS7;
    //tty.c_cflag     |=  IXON ;
    tty.c_cflag     &=  ~CRTSCTS;           // no flow control

    /* Flush Port, then applies attributes */
    tcflush( USB, TCIFLUSH );
    if ( tcsetattr ( USB, TCSANOW, &tty ) != 0)
    {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    }


    this->WatchdogFuse = 1;
    this->setPW = 0;
    this->currentTempK = -1;
    this->currentPW=-1;
    this->currentMode=-1;
    this->SetPointTempK=-1;
    this->RampRateKPerMin=-1;

    printf("LakeShore 325 is now ready to accept instructions.\n");

}


LakeShore::~LakeShore()
{
    close(USB);
}


//Read the Heater Power in percentage

void LakeShore::ReadPower()
{
    std::string LSP_String;
    std::string LSCmd = "HEAT?\r\n";

//    std::cout<<"Sending IDN\n";
    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

   // std::cout<<LSP_String<<"\n";
    
    try{
        this->currentPW = std::stof(LSP_String);
    } catch (...) {
        printf("Error in ReadPower. Continuing...\n ");
    }
    
}

//Read the current temperature in Kelvin

void LakeShore::ReadTemperatureK()
{
    std::string LSP_String;
    std::string LSCmd = "CDAT?\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    //std::cout<<this->currentPW<<"\n";
    
    try{
        this->currentTempK = std::stof(LSP_String);
    } catch (...) {
        printf("Error in ReadCurrentTemp. Continuing...\n ");
    }
    
}

//Read which mode the heater is operating in: 0 for OFF, 2 for LOW and 3 for HIGH

void LakeShore::ReadMode()
{
    std::string LSP_String;
    std::string LSCmd = "RANG?\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    try {
        this->currentMode = std::stof(LSP_String);
    } catch (...) {
        printf("Error reading current mode. Continuing... \n");
    }
}

//Read the curent setpoint temperature in Kelvin

void LakeShore::ReadSetPointTempK()
{
    std::string LSP_String;
    std::string LSCmd = "SETP?\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    try {
        this->SetPointTempK = std::stof(LSP_String);
    } catch (...) {
        printf("Error reading SetPoint. Continuing... \n");
    }
}

//Read the ramp rate in Kelvin per minute

void LakeShore::ReadRampRateKPerMin()
{
    std::string LSP_String;
    std::string LSCmd = "RAMPR?\r\n";


    this->WriteString(LSCmd);
    LSP_String = this->ReadLine();

    try {
        this->RampRateKPerMin = std::stof(LSP_String);
    } catch (...) {
        printf("Error reading RampRate. Continuing... \n");
    }
}

//Set the setpoint temperature in Kelvin
 
void LakeShore::SetTemperature(float temp)
{
	std::string LSCmd;

	LSCmd= "SETP "+std::to_string(temp)+"\r\n";
	this->WriteString(LSCmd);
}

//Set the ramp rate in Kelvin per minute

void LakeShore::SetRampingRate(float rampr)
{
	std::string LSCmd;
	LSCmd= "RAMPR "+std::to_string(rampr)+"\r\n";
	this->WriteString(LSCmd);
}

//Set the heater mode

void LakeShore::SetMode(int mode)
{
	std::string LSCmd;

	LSCmd = "RANG "+std::to_string(mode)+"\r\n";
	this->WriteString(LSCmd);
}

void LakeShore::TurnONOFF(int pwstate){

    std::string LSCmd;

    if (pwstate <3 && pwstate >=0) {
        LSCmd = "RANGE 1,"+std::to_string(pwstate)+"\r\n";
        /*Now we have to set the power again*/
        this->SetPowerLevel(this->setPW);
    } else {
        LSCmd = "RANGE 1,0\r\n";
    }

    this->WriteString(LSCmd);

}

void LakeShore::SetPowerLevel(float newPowerLevel){

    /*Make sure power level is not out of bounds*/
    if (newPowerLevel < 0) newPowerLevel = 0;
    if (newPowerLevel > 100 ) newPowerLevel = 100;

    std::string LSCmd;
    LSCmd = "MOUT 1,"+std::to_string(newPowerLevel)+"\r\n";

    this->WriteString(LSCmd);

    /*Finally update the memory of which power level was set*/
    this->setPW = newPowerLevel;
}



void LakeShore::UpdateMysql(void){

    int _cWatchdogFuse;

    
    // Connect to server using a connection URL
    mysqlx::Session DDroneSession("localhost", 33060, DMysqlUser, DMysqlPass);
    mysqlx::Schema DDb = DDroneSession.getSchema("DAMICDrone");
    

    
    /*First lets get the control parameters*/
    mysqlx::Table CtrlTable = DDb.getTable("ControlParameters");
    mysqlx::RowResult ControlResult = CtrlTable.select("HeaterPW", "HeaterMode", "WatchdogFuse","SetTemperature","SetRampRate")
      .bind("IDX", 1).execute();
    /*The row with the result*/
    mysqlx::Row CtrlRow = ControlResult.fetchOne();


    this->setPW = CtrlRow[0];
    this->setMode = CtrlRow[1];
    this->_cWatchdogFuse = CtrlRow[2];
    this->SetNewSetPointTempK=CtrlRow[3];
    this->SetRampRate=CtrlRow[4];



    /*Now update the monitoring table*/

    // Accessing an existing table
    mysqlx::Table LSHStats = DDb.getTable("LSHState");

    // Insert SQL Table data

    mysqlx::Result LSHResult= LSHStats.insert("HeaterPW", "LSHTemp", "SetPW", "HeaterMode", "WatchdogState","SetPointTempK","RampRate","SetMode","SetNewSetPointTempK","SetRampRate")
           .values(this->currentPW, this->currentTempK, this->setPW, this->currentMode, WatchdogFuse,SetPointTempK,RampRateKPerMin,setMode,SetNewSetPointTempK,SetRampRate).execute();

    unsigned int warnings;
    warnings=LSHResult.getWarningsCount();

    if (warnings == 0) this->SQLStatusMsg = "OK";
    else (SQLStatusMsg = "WARN!\n");


    DDroneSession.close();

}


