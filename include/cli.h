#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

enum CliCommand {
  CMD_HELP,
  CMD_SCAN,
  CMD_TX1,
  CMD_INFO,
  CMD_WIFI_Q,   // WIFI?
  CMD_WIFI,     // WIFI <arg>
  CMD_HTTPGET,
  CMD_HTTPPOST,
  CMD_HEX,
  CMD_TX2,
  CMD_TX2HEX,
  CMD_UNKNOWN
};

CliCommand parseCommand(const char* token);

#ifdef __cplusplus
}
#endif