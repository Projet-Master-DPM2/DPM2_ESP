#include "../../include/cli.h"
#include <string.h>

static bool eq(const char* a, const char* b){ return a && b && strcmp(a,b)==0; }

CliCommand parseCommand(const char* token) {
  if (!token) return CMD_UNKNOWN;
  if (eq(token, "HELP")) return CMD_HELP;
  if (eq(token, "SCAN")) return CMD_SCAN;
  if (eq(token, "TX1")) return CMD_TX1;
  if (eq(token, "INFO")) return CMD_INFO;
  if (eq(token, "WIFI?")) return CMD_WIFI_Q;
  if (eq(token, "WIFI")) return CMD_WIFI;
  if (eq(token, "HTTPGET")) return CMD_HTTPGET;
  if (eq(token, "HTTPPOST")) return CMD_HTTPPOST;
  if (eq(token, "HEX")) return CMD_HEX;
  if (eq(token, "TX2")) return CMD_TX2;
  if (eq(token, "TX2HEX")) return CMD_TX2HEX;
  return CMD_UNKNOWN;
}
