#include <unity.h>
#include "http_utils.h"

void test_url_valid() {
  TEST_ASSERT_TRUE(Http_IsValidUrl("http://example.com"));
  TEST_ASSERT_TRUE(Http_IsValidUrl("https://api.example.com/v1"));
}

void test_url_invalid() {
  TEST_ASSERT_FALSE(Http_IsValidUrl("ftp://example.com"));
  TEST_ASSERT_FALSE(Http_IsValidUrl(""));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_url_valid);
  RUN_TEST(test_url_invalid);
  return UNITY_END();
}


