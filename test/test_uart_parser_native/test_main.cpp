#include <unity.h>
#include "uart_parser.h"
#include <string.h>

void test_too_long(){ char buf[70]; memset(buf,'A',69); buf[69]='\0'; TEST_ASSERT_EQUAL(UART_ERR_TOO_LONG, UartParser_HandleLine(buf, true)); }
void test_bad_char(){ char s[] = "STATE:BAD"; s[6] = (char)0x01; TEST_ASSERT_EQUAL(UART_ERR_BAD_CHAR, UartParser_HandleLine(s, true)); }
void test_state_paying_ack(){ TEST_ASSERT_EQUAL(UART_ACK, UartParser_HandleLine("STATE:PAYING", true)); }
void test_state_paying_nak(){ TEST_ASSERT_EQUAL(UART_NAK, UartParser_HandleLine("STATE:PAYING", false)); }

int main(){ UNITY_BEGIN(); RUN_TEST(test_too_long); RUN_TEST(test_bad_char); RUN_TEST(test_state_paying_ack); RUN_TEST(test_state_paying_nak); return UNITY_END(); }


