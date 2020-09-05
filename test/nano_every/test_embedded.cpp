//
// Created by Igor von Nyssen on 9/3/20.
//

#if defined(ARDUINO) && defined(UNIT_TEST)

#include <unity.h>
#include <Arduino.h>
#include <bms.h>
#include "../../lib/bms/bms.h"

//turn on below to test one at a time if flash is low
#define TEST_STRUCS false
#define TEST_COMMANDS1 false
#define TEST_COMMANDS2 false

void testSoftwareVersion(){
    SoftwareVersion version;
    TEST_ASSERT_EQUAL(0, version.major);
    TEST_ASSERT_EQUAL(0, version.minor);
}

void testSoftwareVersionAssignment(){
    SoftwareVersion version = 0x10;
    TEST_ASSERT_EQUAL(1, version.major);
    TEST_ASSERT_EQUAL(0, version.minor);
}

void testProductionDate(){
    ProductionDate date;
    TEST_ASSERT_EQUAL(1, date.day);
    TEST_ASSERT_EQUAL(1, date.month);
    TEST_ASSERT_EQUAL(2000, date.year);
}

void testProductionDateAssignment(){
    ProductionDate date = (uint16_t) 0x2068;
    TEST_ASSERT_EQUAL(8, date.day);
    TEST_ASSERT_EQUAL(3, date.month);
    TEST_ASSERT_EQUAL(2016, date.year);
}

void testProtectionStatus() {
    ProtectionStatus status;
    TEST_ASSERT_EQUAL(false, status.singleCellOvervoltageProtection);
    TEST_ASSERT_EQUAL(false, status.singleCellUndervoltageProtection);
    TEST_ASSERT_EQUAL(false, status.wholePackOvervoltageProtection);
    TEST_ASSERT_EQUAL(false, status.wholePackUndervoltageProtection);
    TEST_ASSERT_EQUAL(false, status.chargingOverTemperatureProtection);
    TEST_ASSERT_EQUAL(false, status.chargingLowTemperatureProtection);
    TEST_ASSERT_EQUAL(false, status.dischargeOverTemperatureProtection);
    TEST_ASSERT_EQUAL(false, status.dischargeLowTemperatureProtection);
    TEST_ASSERT_EQUAL(false, status.chargingOvercurrentProtection);
    TEST_ASSERT_EQUAL(false, status.dischargeOvercurrentProtection);
    TEST_ASSERT_EQUAL(false, status.shortCircuitProtection);
    TEST_ASSERT_EQUAL(false, status.frontEndDetectionIcError);
    TEST_ASSERT_EQUAL(false, status.softwareLockMos);
}

void testProtectionStatusAssignment() {
    ProtectionStatus status = 0x1FFF;
    TEST_ASSERT_EQUAL(true, status.singleCellOvervoltageProtection);
    TEST_ASSERT_EQUAL(true, status.singleCellUndervoltageProtection);
    TEST_ASSERT_EQUAL(true, status.wholePackOvervoltageProtection);
    TEST_ASSERT_EQUAL(true, status.wholePackUndervoltageProtection);
    TEST_ASSERT_EQUAL(true, status.chargingOverTemperatureProtection);
    TEST_ASSERT_EQUAL(true, status.chargingLowTemperatureProtection);
    TEST_ASSERT_EQUAL(true, status.dischargeOverTemperatureProtection);
    TEST_ASSERT_EQUAL(true, status.dischargeLowTemperatureProtection);
    TEST_ASSERT_EQUAL(true, status.chargingOvercurrentProtection);
    TEST_ASSERT_EQUAL(true, status.dischargeOvercurrentProtection);
    TEST_ASSERT_EQUAL(true, status.shortCircuitProtection);
    TEST_ASSERT_EQUAL(true, status.frontEndDetectionIcError);
    TEST_ASSERT_EQUAL(true, status.softwareLockMos);
}

void testCalculateChecksumCmdBasicSystemInfo(){
    uint8_t *data = BMS::basicSystemInfoCommand;
    TEST_ASSERT_EQUAL(0xFFFD, BMS::calculateChecksum(&data[2], 2));
}

void testCalculateChecksumCmdCellVoltages(){
    uint8_t *data = BMS::cellVoltagesCommand;
    TEST_ASSERT_EQUAL(0xFFFC, BMS::calculateChecksum(&data[2], 2));
}

void testCalculateChecksumCmdName(){
    uint8_t *data = BMS::nameCommand;
    TEST_ASSERT_EQUAL(0xFFFB, BMS::calculateChecksum(&data[2], 2));
}

void testMosfetCommandStringNoChargeNoDischarge(){
    BMS bms;
    uint8_t data[] = {START_BYTE, WRITE, CMD_CTL_MOSFET, 0x02, 0x00, 0x00, 0x00, 0x00, STOP_BYTE};
    bms.calculateMosfetCommandString(data, false, false);
    TEST_ASSERT_EQUAL_HEX(0xFF1A, BMS::calculateChecksum(&data[2], 4));
}

void testMosfetCommandStringChargeNoDischarge(){
    BMS bms;
    uint8_t data[]  = {START_BYTE, WRITE, CMD_CTL_MOSFET, 0x02, 0x00, 0x00, 0x00, 0x00, STOP_BYTE};
    bms.calculateMosfetCommandString(data, true, false);
    TEST_ASSERT_EQUAL_HEX(0xFF1B, BMS::calculateChecksum(&data[2], 4));
}

void testMosfetCommandStringNoChargeDischarge(){
    BMS bms;
    uint8_t data[]  = {START_BYTE, WRITE, CMD_CTL_MOSFET, 0x02, 0x00, 0x00, 0x00, 0x00, STOP_BYTE};
    bms.calculateMosfetCommandString(data, false, true);
    TEST_ASSERT_EQUAL_HEX(0xFF1C, BMS::calculateChecksum(&data[2], 4));
}

void testMosfetCommandStringChargeDischarge(){
    BMS bms;
    uint8_t data[]  = {START_BYTE, WRITE, CMD_CTL_MOSFET, 0x02, 0x00, 0x00, 0x00, 0x00, STOP_BYTE};
    bms.calculateMosfetCommandString(data, true, true);
    TEST_ASSERT_EQUAL_HEX(0xFF1D, BMS::calculateChecksum(&data[2], 4));
}



void setup() {
    UNITY_BEGIN();

#if TEST_STRUCTS
    RUN_TEST(testSoftwareVersion);
    RUN_TEST(testSoftwareVersionAssignment);
    RUN_TEST(testProductionDate);
    RUN_TEST(testProductionDateAssignment);
    RUN_TEST(testProtectionStatus);
    RUN_TEST(testProtectionStatusAssignment);
#endif
#if TEST_COMMANDS1
    RUN_TEST(testCalculateChecksumCmdBasicSystemInfo);
    RUN_TEST(testCalculateChecksumCmdCellVoltages);
    RUN_TEST(testCalculateChecksumCmdName);
#endif
#if TEST_COMMANDS2
    RUN_TEST(testMosfetCommandStringNoChargeNoDischarge);
    RUN_TEST(testMosfetCommandStringChargeNoDischarge);
    RUN_TEST(testMosfetCommandStringNoChargeDischarge);
    RUN_TEST(testMosfetCommandStringChargeDischarge);
#endif

    UNITY_END();
}

void loop() {

}

#endif