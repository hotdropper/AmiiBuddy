//
// Created by Jacob Mather on 8/10/20.
//

#include <unity.h>
#include "m5stack/tests.h"


void loop() {
    test_setup();
//    RUN_TEST(test_truncate);
    RUN_TEST(test_find_amiibo_by_hash);
    UNITY_END(); // stop unit testing
}

