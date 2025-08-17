#include <unity.h>
#include "cli.h"

void test_help(){ TEST_ASSERT_EQUAL(CMD_HELP, parseCommand("HELP")); }
void test_unknown(){ TEST_ASSERT_EQUAL(CMD_UNKNOWN, parseCommand("FOO")); }

int main(){ UNITY_BEGIN(); RUN_TEST(test_help); RUN_TEST(test_unknown); return UNITY_END(); }


