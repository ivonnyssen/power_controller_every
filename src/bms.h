//
// adapted from OverkillSolar BMS here: https://github.com/FurTrader/OverkillSolarBMS
//

/* Copyright 2020 Neil Jansen (njansen1@gmail.com)
 * Copyright 2020 Igor von Nyssen (igor@vonnyssen.com)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef POWER_CONTROLLER_EVERY_BMS_H
#define POWER_CONTROLLER_EVERY_BMS_H


#include "Arduino.h"

#define BMS_OPTION_DEBUG true

#define NUM_TEMP_SENSORS 2
#define NUM_CELLS 8
#define RX_BUFFER_SIZE 64

// Constants
#define START_BYTE 0xDD
#define STOP_BYTE  0x77
#define READ       0xA5
#define WRITE      0x5A

// Commands
#define CMD_BASIC_SYSTEM_INFO 0x03
#define CMD_CELL_VOLTAGES     0x04
#define CMD_NAME              0x05
#define CMD_CTL_MOSFET        0xE1


typedef struct SoftwareVersion {
    uint8_t major;
    uint8_t minor;

    SoftwareVersion(){
        major = 0;
        minor = 0;
    }

    SoftwareVersion& operator=(uint8_t version){
        major = version / 0x0Fu & 0b1111u;
        minor = version & 0b1111u;
    }

} SoftwareVersion;

typedef struct ProductionDate {
    uint16_t year;
    uint8_t month;
    uint8_t day;

    ProductionDate(){
        day = 01;
        month = 01;
        year = 2000;
    }

    ProductionDate& operator=(uint16_t date) {
        // Production date is stored internally as a uint16_t, bit-packed as follows:
        //         1111110000000000
        // Field   5432109876543210  # bits  offset
        // ======  ================  ======  ======
        // Day:               xxxxx  5       0
        // Month:         xxxx       4       5
        // Year:   xxxxxxx           7       9
        day = date & 0x1fu;
        month = (date >> 5u) & 0x0fu;
        year = 2000u + (date >> 9u);
    }
} ProductionDate;

typedef struct ProtectionStatus {
    bool singleCellOvervoltageProtection;
    bool singleCellUndervoltageProtection;
    bool wholePackOvervoltageProtection;
    bool wholePackUndervoltageProtection;
    bool chargingOverTemperatureProtection;
    bool chargingLowTemperatureProtection;
    bool dischargeOverTemperatureProtection;
    bool dischargeLowTemperatureProtection;
    bool chargingOvercurrentProtection;
    bool dischargeOvercurrentProtection;
    bool shortCircuitProtection;
    bool frontEndDetectionIcError;
    bool softwareLockMos;

    ProtectionStatus(){
        singleCellOvervoltageProtection    = false;
        singleCellUndervoltageProtection   = false;
        wholePackOvervoltageProtection     = false;
        wholePackUndervoltageProtection    = false;
        chargingOverTemperatureProtection  = false;
        chargingLowTemperatureProtection   = false;
        dischargeOverTemperatureProtection = false;
        dischargeLowTemperatureProtection  = false;
        chargingOvercurrentProtection      = false;
        dischargeOvercurrentProtection     = false;
        shortCircuitProtection             = false;
        frontEndDetectionIcError           = false;
        softwareLockMos                    = false;
    }

    ProtectionStatus& operator=(uint16_t status) {
        singleCellOvervoltageProtection    = status & 0b0000000000000001u;
        singleCellUndervoltageProtection   = status & 0b0000000000000010u;
        wholePackOvervoltageProtection     = status & 0b0000000000000100u;
        wholePackUndervoltageProtection    = status & 0b0000000000001000u;
        chargingOverTemperatureProtection  = status & 0b0000000000010000u;
        chargingLowTemperatureProtection   = status & 0b0000000000100000u;
        dischargeOverTemperatureProtection = status & 0b0000000001000000u;
        dischargeLowTemperatureProtection  = status & 0b0000000010000000u;
        chargingOvercurrentProtection      = status & 0b0000000100000000u;
        dischargeOvercurrentProtection     = status & 0b0000001000000000u;
        shortCircuitProtection             = status & 0b0000010000000000u;
        frontEndDetectionIcError           = status & 0b0000100000000000u;
        softwareLockMos                    = status & 0b0001000000000000u;
    }
} ProtectionStatus;

typedef struct FaultCounts {
    uint8_t singleCellOvervoltageProtection;
    uint8_t singleCellUndervoltageProtection;
    uint8_t wholePackOvervoltageProtection;
    uint8_t wholePackUndervoltageProtection;
    uint8_t chargingOverTemperatureProtection;
    uint8_t chargingLowTemperatureProtection;
    uint8_t dischargeOverTemperatureProtection;
    uint8_t dischargeLowTemperatureProtection;
    uint8_t chargingOvercurrentProtection;
    uint8_t dischargeOvercurrentProtection;
    uint8_t shortCircuitProtection;
    uint8_t frontEndDetectionIcError;
    uint8_t softwareLockMos;

    FaultCounts(){
        singleCellOvervoltageProtection = 0;
        singleCellUndervoltageProtection = 0;
        wholePackOvervoltageProtection = 0;
        wholePackUndervoltageProtection = 0;
        chargingOverTemperatureProtection = 0;
        chargingLowTemperatureProtection = 0;
        dischargeOverTemperatureProtection = 0;
        dischargeLowTemperatureProtection = 0;
        chargingOvercurrentProtection = 0;
        dischargeOvercurrentProtection = 0;
        shortCircuitProtection = 0;
        frontEndDetectionIcError = 0;
        softwareLockMos = 0;
    }
} FaultCounts;


class BMS {
public:
    BMS();

    void begin(Stream *port, uint16_t timeout = 2000); // serial port stream and timeout
    void poll(); // Call this every time you want to poll the BMS
    void end();    // End processing.  Call this to stop querying the BMS and processing data.
    bool hasComError() const;  // Returns true if there was a timeout or checksum error on the last call

    float totalVoltage;
    float current;
    float balanceCapacity;
    float rateCapacity;
    uint16_t cycleCount;
    ProductionDate productionDate;
    ProtectionStatus protectionStatus;
    SoftwareVersion softwareVersion;
    uint8_t stateOfCharge;  // state of charge, in percent (0-100)
    bool isDischargeFetEnabled;
    bool isChargeFetEnabled;
    uint8_t numCells; // number of battery cells
    uint8_t numTemperatureSensors;  // number of temperature sensors
    float temperatures[NUM_TEMP_SENSORS]{};
    float cellVoltages[NUM_CELLS]{};
    String name;
    FaultCounts faultCounts;
    void clearFaultCounts();
    bool isBalancing(uint8_t cellNumber) const;

    // #######################################################################
    // 0xE1 MOSFET Control

    void setMosfetControl(bool charge, bool discharge);  // Controls the charge and discharge MOSFETs

#ifdef BMS_OPTION_DEBUG
    void debug();  // Calling this method will print out the received data to the main serial port
#endif

private:
    bool isEnabled;
    Stream* serial{};
    bool comError;
    uint32_t balanceStatus;  // The cell balance statuses, stored as a bitfield
    ProtectionStatus lastProtectionStatus;
    void queryBasicInfo();
    void queryCellVoltages();
    void queryBmsName();
    uint16_t calculateChecksum(uint8_t* buffer, int len);

    bool readValidResponse(uint8_t *buffer, uint8_t command);
};

#endif //POWER_CONTROLLER_EVERY_BMS_H
