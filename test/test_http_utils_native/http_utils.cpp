#include "../../include/http_utils.h"
#include <string.h>

bool Http_IsValidUrl(const char* url) {
  if (url == nullptr) return false;
  size_t len = strlen(url);
  if (len == 0 || len > 240) return false;
  if (strncmp(url, "http://", 7) == 0) return true;
  if (strncmp(url, "https://", 8) == 0) return true;
  return false;
}
