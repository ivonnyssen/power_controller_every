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

#ifdef ARDUINO

#include "bms.h"

BMS::BMS() {
    totalVoltage = 0;
    current = 0;
    balanceCapacity = 0;
    rateCapacity = 0;
    cycleCount = 0;
    productionDate = 0;
    protectionStatus = 0;
    softwareVersion = 0;
    stateOfCharge = 0;  // state of charge, in percent (0-100)
    isDischargeFetEnabled = false;
    isChargeFetEnabled = false;
    numCells = 0;
    numTemperatureSensors = 0;
    for (uint8_t i; i < NUM_TEMP_SENSORS; i++) {
        temperatures[i] = 0;
    }
    for (uint8_t i; i < NUM_CELLS; i++) {
        cellVoltages[i] = 0;
    }
    name = String("");

    comError = false;
    isEnabled = false;
    balanceStatus = 0;
    lastProtectionStatus = 0;
}

void BMS::begin(Stream *port, uint16_t timeout) {
#ifdef BMS_OPTION_DEBUG
    Serial.println("OverkillSolarBMS Begin!");
#endif
    serial = port;
    serial->setTimeout(timeout);
    isEnabled = true;
}

void BMS::end() {
    isEnabled = false;
}

void BMS::poll() {
    if (isEnabled) {
        queryBasicInfo();
        queryCellVoltages();
        if(name.equals("")){
            queryBmsName();
        }
    }
}

bool BMS::hasComError() const {
    return comError;
}

bool BMS::isBalancing(uint8_t cellNumber) const {
    if (cellNumber <= 31) {
        return (balanceStatus >> cellNumber) & 1u;
    }
    else {
        return false;
    }
}

void BMS::clearFaultCounts() {
    faultCounts.singleCellOvervoltageProtection    = 0;
    faultCounts.singleCellUndervoltageProtection   = 0;
    faultCounts.wholePackOvervoltageProtection     = 0;
    faultCounts.wholePackUndervoltageProtection    = 0;
    faultCounts.chargingOverTemperatureProtection  = 0;
    faultCounts.chargingLowTemperatureProtection   = 0;
    faultCounts.dischargeOverTemperatureProtection = 0;
    faultCounts.dischargeLowTemperatureProtection  = 0;
    faultCounts.chargingOvercurrentProtection      = 0;
    faultCounts.dischargeOvercurrentProtection     = 0;
    faultCounts.shortCircuitProtection             = 0;
    faultCounts.frontEndDetectionIcError           = 0;
    faultCounts.softwareLockMos                    = 0;
}

void BMS::setMosfetControl(bool charge, bool discharge) {
#ifdef BMS_OPTION_DEBUG
    Serial.println("Query 0xE1 MOSFET Control");
#endif
    uint8_t xxByte = 0b11;
    xxByte &= charge ? 0b10u : 0b11u;
    xxByte &= discharge ? 0b01u : 0b11u;

    uint8_t data[] = {START_BYTE, WRITE, CMD_CTL_MOSFET, 0x00, xxByte, 0xFF, 0xFD, STOP_BYTE};

    if(serial->availableForWrite()){
        serial->write(data, sizeof(data));
    }

    uint8_t buffer[RX_BUFFER_SIZE] {0};
    comError = readValidResponse(buffer, CMD_CTL_MOSFET);
}

#if BMS_OPTION_DEBUG
void BMS::debug() {
    Serial.println("==============================================");
    Serial.print("Voltage:           ");
    Serial.print(totalVoltage, 3);
    Serial.println(" V");

    Serial.print("Current:           ");
    Serial.print(current, 3);
    Serial.println(" A");

    Serial.print("Balance capacity:  ");
    Serial.print(balanceCapacity, 3);
    Serial.println(" Ah");

    Serial.print("Rate capacity:     ");
    Serial.print(rateCapacity, 3);
    Serial.println(" Ah");

    Serial.print("Cycle count:       ");
    Serial.println(cycleCount , DEC);

    Serial.print("Production Date:   ");
    Serial.print(productionDate.day, DEC);
    Serial.print("/");
    Serial.print(productionDate.month, DEC);
    Serial.print("/");
    Serial.println(productionDate.year, DEC);

    Serial.println("Protection Status: ");
    Serial.print("  softwareLockMos:                    ");
    Serial.println(protectionStatus.softwareLockMos, DEC);
    Serial.print("  frontEndDetectionIcError:         ");
    Serial.println(protectionStatus.frontEndDetectionIcError, DEC);
    Serial.print("  shortCircuitProtection:             ");
    Serial.println(protectionStatus.shortCircuitProtection, DEC);
    Serial.print("  dischargeOvercurrentProtection:     ");
    Serial.println(protectionStatus.dischargeOvercurrentProtection, DEC);
    Serial.print("  chargingOvercurrentProtection:      ");
    Serial.println(protectionStatus.chargingOvercurrentProtection, DEC);
    Serial.print("  dischargeLowTemperatureProtection: ");
    Serial.println(protectionStatus.dischargeLowTemperatureProtection, DEC);
    Serial.print("  dischargeOverTemperatureProtection:");
    Serial.println(protectionStatus.dischargeOverTemperatureProtection, DEC);
    Serial.print("  chargingLowTemperatureProtection:  ");
    Serial.println(protectionStatus.chargingLowTemperatureProtection, DEC);
    Serial.print("  chargingOverTemperatureProtection: ");
    Serial.println(protectionStatus.chargingOverTemperatureProtection, DEC);
    Serial.print("  wholePackUndervoltageProtection:   ");
    Serial.println(protectionStatus.wholePackUndervoltageProtection, DEC);
    Serial.print("  wholePackOvervoltageProtection:    ");
    Serial.println(protectionStatus.wholePackOvervoltageProtection, DEC);
    Serial.print("  singleCellUndervoltageProtection:  ");
    Serial.println(protectionStatus.singleCellUndervoltageProtection, DEC);
    Serial.print("  singleCellOvervoltageProtection:   ");
    Serial.println(protectionStatus.singleCellOvervoltageProtection, DEC);

    Serial.print("Software version:  ");
    Serial.print(softwareVersion.major, DEC);
    Serial.print(".");
    Serial.println(softwareVersion.minor, DEC);

    Serial.print("State of Charge:   ");
    Serial.print(stateOfCharge, DEC);
    Serial.println("%");

    Serial.print("Discharge MOSFET:  ");
    Serial.println(isDischargeFetEnabled ? "ON" : "OFF");

    Serial.print("Charge MOSFET:     ");
    Serial.println(isChargeFetEnabled ? "ON" : "OFF");

    Serial.print("# of cells:        ");
    Serial.println(numCells, DEC);

    Serial.print("# of temp sensors: ");
    Serial.println(numTemperatureSensors, DEC);

    Serial.println("Temperatures:");
    for (int i=0; i < min(NUM_TEMP_SENSORS, numTemperatureSensors); i++) {
        Serial.print("  ");
        Serial.print(temperatures[i], 1);
        Serial.println(" deg C");
    }

    Serial.println("Cell Voltages & Balance Status: ");
    for (int i=0; i < min(NUM_CELLS, numCells); i++) {
        Serial.print("  ");
        Serial.print(cellVoltages[i], 3);  // Returns the cell voltage, in volts
        Serial.print("V  ");
        Serial.println(isBalancing(i) ? "(balancing)" : "(not balancing)");
    }

    Serial.print("BMS Name:         ");
    Serial.println(name);
    Serial.println();
}
#endif

void BMS::queryBasicInfo() {
#ifdef BMS_OPTION_DEBUG
    Serial.println("Query 0x03 Basic Info");
#endif

    uint8_t data[] = {START_BYTE, READ, CMD_BASIC_SYSTEM_INFO, 0x00, 0xFF, 0xFD, STOP_BYTE};

    if(serial->availableForWrite()){
        serial->write(data, sizeof(data));
    }

    uint8_t buffer[RX_BUFFER_SIZE] {0};
    comError = readValidResponse(buffer, CMD_BASIC_SYSTEM_INFO);
    if(comError){
        return;
    }

    totalVoltage = 0.01f * ((uint16_t)(buffer[3] << 8u) | (uint16_t)(buffer[4]));
    current = ((uint16_t)(buffer[4]  << 8u) | (uint16_t)(buffer[5])) * 0.01;
    balanceCapacity = ((uint16_t)(buffer[6]  << 8u) | (uint16_t)(buffer[7])) * 0.01;
    rateCapacity = ((uint16_t)(buffer[8]  << 8u) | (uint16_t)(buffer[9])) * 0.01;
    cycleCount = (uint16_t)(buffer[10]  << 8u) | (uint16_t)(buffer[11]);
    productionDate = (uint16_t)(buffer[12]  << 8u) | (uint16_t)(buffer[13]);
    balanceStatus = (uint32_t)(buffer[14]  << 8u) | (uint32_t)(buffer[15]) | (uint32_t)(buffer[16]  << 24u) | (uint32_t)(buffer[17] << 16u) ;
    lastProtectionStatus = protectionStatus;
    protectionStatus = (uint16_t)(buffer[18] << 8u) | (uint16_t)(buffer[19]);

    // See if there are any new faults.  If so, then increment the count.
    if (!lastProtectionStatus.singleCellOvervoltageProtection && protectionStatus.singleCellOvervoltageProtection)  { faultCounts.singleCellOvervoltageProtection += 1; }
    if (!lastProtectionStatus.singleCellUndervoltageProtection && protectionStatus.singleCellUndervoltageProtection)  { faultCounts.singleCellUndervoltageProtection += 1; }
    if (!lastProtectionStatus.wholePackOvervoltageProtection && protectionStatus.wholePackOvervoltageProtection)  { faultCounts.wholePackOvervoltageProtection += 1; }
    if (!lastProtectionStatus.wholePackUndervoltageProtection && protectionStatus.wholePackUndervoltageProtection)  { faultCounts.wholePackUndervoltageProtection += 1; }
    if (!lastProtectionStatus.chargingOverTemperatureProtection && protectionStatus.chargingOverTemperatureProtection)  { faultCounts.chargingOverTemperatureProtection += 1; }
    if (!lastProtectionStatus.chargingLowTemperatureProtection && protectionStatus.chargingLowTemperatureProtection)  { faultCounts.chargingLowTemperatureProtection += 1; }
    if (!lastProtectionStatus.dischargeOverTemperatureProtection && protectionStatus.dischargeOverTemperatureProtection)  { faultCounts.dischargeOverTemperatureProtection += 1; }
    if (!lastProtectionStatus.dischargeLowTemperatureProtection && protectionStatus.dischargeLowTemperatureProtection)  { faultCounts.dischargeLowTemperatureProtection += 1; }
    if (!lastProtectionStatus.chargingOvercurrentProtection && protectionStatus.chargingOvercurrentProtection)  { faultCounts.chargingOvercurrentProtection += 1; }
    if (!lastProtectionStatus.dischargeOvercurrentProtection && protectionStatus.dischargeOvercurrentProtection)  { faultCounts.dischargeOvercurrentProtection += 1; }
    if (!lastProtectionStatus.shortCircuitProtection && protectionStatus.shortCircuitProtection) { faultCounts.shortCircuitProtection += 1; }
    if (!lastProtectionStatus.frontEndDetectionIcError && protectionStatus.frontEndDetectionIcError) { faultCounts.frontEndDetectionIcError += 1; }
    if (!lastProtectionStatus.softwareLockMos && protectionStatus.softwareLockMos) { faultCounts.softwareLockMos += 1; }

    softwareVersion = buffer[20];
    stateOfCharge = buffer[21];
    isDischargeFetEnabled = buffer[22] & 0b00000010u;
    isChargeFetEnabled = buffer[22] & 0b00000001u;
    numCells = buffer[23];
    numTemperatureSensors = buffer[24];

    for (int i = 0; i < min(numTemperatureSensors, NUM_TEMP_SENSORS); i++) {
        temperatures[i] = ((uint16_t)(buffer[25 + (i * 2)] << 8u) | (uint16_t)(buffer[26 + (i * 2)])) * 0.1f -273.15f;
    }
}


void BMS::queryCellVoltages() {
#ifdef BMS_OPTION_DEBUG
    Serial.println("Query 0x04 Cell Voltages");
#endif
    uint8_t data[] = {START_BYTE, READ, CMD_CELL_VOLTAGES, 0x00, 0xFF, 0xFC, STOP_BYTE};

    if(serial->availableForWrite()){
        serial->write(data, sizeof(data));
    }

    uint8_t buffer[RX_BUFFER_SIZE] {0};
    comError = readValidResponse(buffer, CMD_CELL_VOLTAGES);
    if(comError){
        return;
    }

    for (int i = 0; i < min(numCells, NUM_CELLS); i++) {
        cellVoltages[i] = ((uint16_t)(buffer[i * 2 + 2] << 8u) | (uint16_t)(buffer[(i * 2) + 3])) * 0.001f;
    }
}

void BMS::queryBmsName() {
#ifdef BMS_OPTION_DEBUG
    Serial.println("Query 0x05 BMS Name");
#endif
    uint8_t data[] = {START_BYTE, READ, CMD_NAME, 0x00, 0xFF, 0xFB, STOP_BYTE};

    if(serial->availableForWrite()){
        serial->write(data, sizeof(data));
    }

    uint8_t buffer[RX_BUFFER_SIZE] {0};
    comError = readValidResponse(buffer, CMD_NAME);
    if(comError){
        return;
    }

    name = String();
    for(int i = 0; i < buffer[3]; i++){
        name.concat((char)buffer[i]);
    }
}

uint16_t BMS::calculateChecksum(uint8_t *buffer, int len) {
    uint16_t checksum =0;
    for(int i = 2; i < 2 + len; i++){
        checksum += buffer[i];
    }
    return 0xFFFF - checksum + 1;
}

bool BMS::readValidResponse(uint8_t* buffer, uint8_t command){
    if( serial->readBytesUntil((char)STOP_BYTE, buffer, sizeof(buffer)) <= 0) {
        return false;
    }

    if(!(buffer[0] == START_BYTE && buffer[1] == command && buffer[2] == 0x00)){
        return false;
    }

    int length = ((uint16_t)(buffer[2]  << 8u) | (uint16_t)(buffer[3]));
    uint16_t calculatedCheckSum = calculateChecksum(&buffer[04], length);
    uint16_t transmittedChecksum = ((uint16_t)(buffer[length+4]  << 8u) | (uint16_t)(buffer[length+5]));
    if(calculatedCheckSum != transmittedChecksum) {
        return false;
    }
}

#endif