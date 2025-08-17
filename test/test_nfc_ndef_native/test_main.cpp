#include <unity.h>
#include "nfc_ndef.h"

void test_ndef_text_ok(){
  const uint8_t tlv[] = { 0x03, 0x0C, 0xD1, 0x01, 0x08, 0x54, 0x02, 'e','n','H','e','l','l','o', 0xFE };
  char out[120]; TEST_ASSERT_TRUE(Nfc_ParseTlvAndExtractNdefText_C(tlv, sizeof(tlv), out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING_LEN("Hello", out, 5);
}

void test_uid_hex(){
  const uint8_t uid[] = { 0xDE, 0xAD, 0xBE, 0xEF };
  char hex[2*4+1]; TEST_ASSERT_TRUE(Nfc_UidToHex_C(uid, sizeof(uid), hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING("DEADBEEF", hex);
}

int main(){ UNITY_BEGIN(); RUN_TEST(test_ndef_text_ok); RUN_TEST(test_uid_hex); return UNITY_END(); }


