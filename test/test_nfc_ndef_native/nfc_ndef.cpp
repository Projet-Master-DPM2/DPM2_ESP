#include "../../include/nfc_ndef.h"
#include <string.h>
#include <stdio.h>

bool Nfc_ParseTlvAndExtractNdefText_C(const uint8_t* mem, size_t totalLen, char* outText, size_t outSize) {
  if (!mem || totalLen == 0) return false;
  if (!outText || outSize < 2) return false;
  size_t i = 0;
  while (i < totalLen) {
    uint8_t t = mem[i++];
    if (t == 0x00) continue;
    if (t == 0xFE) break;
    if (i >= totalLen) break;
    uint16_t L = mem[i++];
    if (L == 0xFF) { if (i + 1 >= totalLen) break; L = ((uint16_t)mem[i] << 8) | mem[i + 1]; i += 2; }
    if (i + L > totalLen) break;
    if (t == 0x03) {
      const uint8_t* p = mem + i; const uint8_t* end = p + L;
      if (p >= end) return false;
      uint8_t hdr = *p++;
      bool sr = (hdr & 0x10) != 0;
      bool il = (hdr & 0x08) != 0;
      uint8_t tnf = (hdr & 0x07);
      if (p >= end) return false;
      uint8_t typeLen = *p++;
      uint32_t payloadLen = 0;
      if (sr) { if (p >= end) return false; payloadLen = *p++; }
      else { if (p + 4 > end) return false; payloadLen = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3]; p += 4; }
      uint8_t idLen = 0; if (il) { if (p >= end) return false; idLen = *p++; }
      if (p + typeLen > end) return false;
      const uint8_t* typePtr = p;
      p += typeLen;
      if (il) { if (p + idLen > end) return false; p += idLen; }
      if (p + payloadLen > end) return false;
      if (tnf == 0x01 && typeLen == 1 && typePtr[0] == 'T' && payloadLen >= 1) {
        uint8_t status = p[0];
        uint8_t langLen = status & 0x3F;
        if ((status & 0x80) != 0) return false;
        if (1U + (uint32_t)langLen > payloadLen) return false;
        const uint8_t* txt = p + 1 + langLen;
        uint32_t txtLen = payloadLen - 1 - langLen;
        size_t maxCopy = (outSize - 1);
        size_t n = (txtLen < maxCopy) ? txtLen : maxCopy;
        memcpy(outText, txt, n);
        outText[n] = '\0';
        return true;
      }
    }
    i += L;
  }
  return false;
}

bool Nfc_UidToHex_C(const uint8_t* uidBytes, size_t uidLen, char* outHex, size_t outSize) {
  if (!uidBytes || uidLen == 0 || uidLen > 10 || !outHex || outSize < 3) return false;
  size_t need = uidLen * 2 + 1;
  if (outSize < need) return false;
  size_t index = 0;
  for (size_t i = 0; i < uidLen; ++i) {
    snprintf(&outHex[index], 3, "%02X", uidBytes[i]);
    index += 2;
  }
  outHex[index] = '\0';
  return true;
}
