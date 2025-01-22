#pragma once

namespace gremlinex {

static const int kPacketDataSize = 1024;
static const char* kPublicDownlinkChannel = "muchimi.gremlinex.downlink";
static const char* kPublicUplinkChannel = "muchimi.gremlinex.uplink";

class Packet {
 private:
  // HACK: static offset avoids collisions.
  inline static unsigned int offset = 0;

 public:
  int id = 0;
  int code = 0;
  char data[kPacketDataSize] = {};

  Packet(){};
  Packet(char data[]);
};

enum ClientDataDefinition {
  kPacketDefinition = 6124,
};

enum ClientDataArea {
  kPublicDownlinkArea = 6125,
  kPublicUplinkArea = 6126,
};

enum DataRequest {
  kUplinkRequest = 6125,
  kDownlinkRequest = 6126,
};

enum Opcode {
  kExecuteCalculatorCode = 0,
  kGetNamedVariable = 1,
  kGetVariableList = 2,
  kPing = 3, // ping for alive 
};

}  // namespace gremlinex
