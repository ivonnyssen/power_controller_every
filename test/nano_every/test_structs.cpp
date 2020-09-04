//
// Created by Igor von Nyssen on 9/3/20.
//

#define ARDUINO
#define UNIT_TEST

#if defined(ARDUINO) && defined(UNIT_TEST)

#include <unity.h>
#include <Arduino.h>
#include <bms.h>
#include "../../lib/bms/bms.h"


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

void setup() {
    UNITY_BEGIN();

    RUN_TEST(testSoftwareVersion);
    RUN_TEST(testSoftwareVersionAssignment);
    RUN_TEST(testProductionDate);
    RUN_TEST(testProductionDateAssignment);
    RUN_TEST(testProtectionStatus);
    RUN_TEST(testProtectionStatusAssignment);

    UNITY_END();
}

void loop() {

}

#endif