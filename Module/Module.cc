#include "Module.hh"

// Microsoft Flight Simulator includes.
#include <MSFS/Legacy/gauges.h>
#include <MSFS/MSFS.h>
#include <MSFS/MSFS_WindowsTypes.h>
#include <SimConnect.h>

// Standard Template Library includes.
#include <iostream>
#include <string>
#include <vector>

// Library defines common things e.g. Packet.
#include "../Protocol/Protocol.hh"

HANDLE simconnect = 0;
std::vector<std::string> lVarList; // list of variables


void CALLBACK HandleSimconnectMessage(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {
  if (pData->dwID != SIMCONNECT_RECV_ID_CLIENT_DATA) return;

  auto received_data = static_cast<SIMCONNECT_RECV_CLIENT_DATA*>(pData);
  gremlinex::Packet* packet = (gremlinex::Packet*)&received_data->dwData;

  // We'll prepare our response packet with the same id.
  auto response = new gremlinex::Packet();
  response->id = packet->id;
  response->code = packet->code;

  // Split packet data to opcode and payload.
  char opcode = packet->code;
  std::string payload = (char*)(packet->data);

  switch (opcode) {
    case gremlinex::kExecuteCalculatorCode: {
      execute_calculator_code(payload.c_str(), 0, 0, 0);
      break;
    }

    case gremlinex::kGetNamedVariable: {
      int named_variable = check_named_variable(payload.c_str());
      // Do not send a reply packet if the variable was not found.
      if (named_variable == -1) return;

      double variable_value = get_named_variable_value(named_variable);
      // Copy the result float into the response packet's data member variable.
      std::memcpy(response->data, &variable_value, sizeof(double));
      break;

    }

    case gremlinex::kPing: {
      
        std::strcpy(response->data, "#pong#");
        SimConnect_SetClientData(simconnect, gremlinex::kPublicDownlinkArea, gremlinex::kPacketDefinition, 0, 0,
                               sizeof(gremlinex::Packet), response);
      
    }

    case gremlinex::kGetVariableList: {
      lVarList.clear();

      for (int i = 0; i != 1000; i++) {
        const char* lVarName = get_name_of_named_variable(i); if (lVarName == NULLPTR) break;
        lVarList.push_back(std::string(lVarName));
      }

      std::sort(lVarList.begin(), lVarList.end());

      // send the variables back one at a time

      // Send the response packet using the PublicDownlinkArea.
      // start marker
      std::strcpy(response->data, "#lvar_begin#");
      SimConnect_SetClientData(simconnect, gremlinex::kPublicDownlinkArea, gremlinex::kPacketDefinition, 0, 0,
                               sizeof(gremlinex::Packet), response);

      // iterate
      for (const auto& lVar : lVarList) {
        std::strcpy(response->data, lVar.c_str());
        SimConnect_SetClientData(simconnect, gremlinex::kPublicDownlinkArea, gremlinex::kPacketDefinition, 0, 0,
                                 sizeof(gremlinex::Packet), response);
      }

      // finish marker
      std::strcpy(response->data, "#lvar_end#");
      SimConnect_SetClientData(simconnect, gremlinex::kPublicDownlinkArea, gremlinex::kPacketDefinition, 0, 0, sizeof(gremlinex::Packet), response);

    }

    default:
      return;
  }

  // Send the response packet using the PublicDownlinkArea.
  SimConnect_SetClientData(simconnect, gremlinex::kPublicDownlinkArea, gremlinex::kPacketDefinition, 0, 0,
                           sizeof(gremlinex::Packet), response);
}

extern "C" MSFS_CALLBACK void module_init() {
  // This will (hopefully) be visibile in the Microsoft Flight Simulator console.
  std::cout << "[GremlinExBridge] Module version " << kModuleVersion << " initializing." << std::endl;

  // TODO: implement simconnect error handling.
  SimConnect_Open(&simconnect, kSimconnectClientName, 0, 0, 0, 0);

  // Define a custom ClientDataDefinition for gremlinex::Packet.
  SimConnect_AddToClientDataDefinition(simconnect, gremlinex::kPacketDefinition, 0, sizeof(gremlinex::Packet));

  // Map the public downlink and uplink channels with own ids for them (see CleitnDataAreas enum).
  SimConnect_MapClientDataNameToID(simconnect, gremlinex::kPublicDownlinkChannel, gremlinex::kPublicDownlinkArea);
  SimConnect_MapClientDataNameToID(simconnect, gremlinex::kPublicUplinkChannel, gremlinex::kPublicUplinkArea);

  // Create the public downlink and uplink channels (which are actually ClientData areas).
  SimConnect_CreateClientData(simconnect, gremlinex::kPublicDownlinkArea, sizeof(gremlinex::Packet), 0);
  SimConnect_CreateClientData(simconnect, gremlinex::kPublicUplinkArea, sizeof(gremlinex::Packet), 0);

  // Request to be notified (via Simconnect Dispatch) for any changes to the public uplink channel.
  SimConnect_RequestClientData(simconnect, gremlinex::kPublicUplinkArea, gremlinex::kUplinkRequest,
                               gremlinex::kPacketDefinition, SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET,
                               SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED);

  SimConnect_CallDispatch(simconnect, HandleSimconnectMessage, 0);
}

extern "C" MSFS_CALLBACK void module_deinit() {
  if (!simconnect) return;
  SimConnect_Close(simconnect);
}
