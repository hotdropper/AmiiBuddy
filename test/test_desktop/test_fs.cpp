//
// Created by Jacob Mather on 7/31/20.
//

#define FSTools_Native
#include <filesystem>
#include <FSTools_Native.h>
#include <FSTools.h>
#include <unity.h>

void test_function_calculator_addition(void) {
    int fileCount = FSTools::getFileCount("/sd");
    TEST_ASSERT_EQUAL(1, calc.add(25, 7));
}

