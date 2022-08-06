#include "Version.hh"

#include <strings.h>

#include <stdexcept>

#include "Client.hh"

using namespace std;



uint16_t flags_for_version(GameVersion version, int64_t sub_version) {
  switch (sub_version) {
    case -1: // Initial check (before 9E recognition)
      switch (version) {
        case GameVersion::DC:
          // TODO: For DCv1, the flags should be:
          //   Client::Flag::DCV1 |
          //   Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
          //   Client::Flag::DOES_NOT_SUPPORT_SEND_FUNCTION_CALL
          return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION;
        case GameVersion::GC:
        case GameVersion::XB:
          return 0;
        case GameVersion::PC:
          return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
                 Client::Flag::SEND_FUNCTION_CALL_CHECKSUM_ONLY;
        case GameVersion::PATCH:
          return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
                 Client::Flag::DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;
        case GameVersion::BB:
          return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
                 Client::Flag::SAVE_ENABLED;
      }
      break;
    case 0x29: // PC
      return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
             Client::Flag::SEND_FUNCTION_CALL_CHECKSUM_ONLY;
    case 0x30: // GC Ep1&2 JP v1.02, at least one version of PSO XB
    case 0x31: // GC Ep1&2 US v1.00, GC US v1.01, GC EU v1.00, GC JP v1.00
    case 0x34: // GC Ep1&2 JP v1.03
      return 0;
    case 0x32: // GC Ep1&2 EU 50Hz
    case 0x33: // GC Ep1&2 EU 60Hz
      return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN;
    case 0x35: // GC Ep1&2 JP v1.04 (Plus)
      return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
             Client::Flag::ENCRYPTED_SEND_FUNCTION_CALL;
    case 0x36: // GC Ep1&2 US v1.02 (Plus)
    case 0x39: // GC Ep1&2 JP v1.05 (Plus)
      return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
             Client::Flag::DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;
    case 0x42: // GC Ep3 JP
      return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
             Client::Flag::EPISODE_3 |
             Client::Flag::ENCRYPTED_SEND_FUNCTION_CALL;
    case 0x40: // GC Ep3 trial (TODO: Does this support send_function_call?)
    case 0x41: // GC Ep3 US
    case 0x43: // GC Ep3 EU
      return Client::Flag::NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
             Client::Flag::EPISODE_3 |
             Client::Flag::DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;
  }
  throw runtime_error("unknown sub_version");
}

const char* name_for_version(GameVersion version) {
  switch (version) {
    case GameVersion::GC:
      return "GC";
    case GameVersion::XB:
      return "XB";
    case GameVersion::PC:
      return "PC";
    case GameVersion::BB:
      return "BB";
    case GameVersion::DC:
      return "DC";
    case GameVersion::PATCH:
      return "Patch";
    default:
      return "Unknown";
  }
}

GameVersion version_for_name(const char* name) {
  if (!strcasecmp(name, "DC") || !strcasecmp(name, "DreamCast")) {
    return GameVersion::DC;
  } else if (!strcasecmp(name, "PC")) {
    return GameVersion::PC;
  } else if (!strcasecmp(name, "GC") || !strcasecmp(name, "GameCube")) {
    return GameVersion::GC;
  } else if (!strcasecmp(name, "XB") || !strcasecmp(name, "Xbox")) {
    return GameVersion::XB;
  } else if (!strcasecmp(name, "BB") || !strcasecmp(name, "BlueBurst") ||
      !strcasecmp(name, "Blue Burst")) {
    return GameVersion::BB;
  } else if (!strcasecmp(name, "Patch")) {
    return GameVersion::PATCH;
  } else {
    throw invalid_argument("incorrect version name");
  }
}

const char* name_for_server_behavior(ServerBehavior behavior) {
  switch (behavior) {
    case ServerBehavior::SPLIT_RECONNECT:
      return "split_reconnect";
    case ServerBehavior::LOGIN_SERVER:
      return "login_server";
    case ServerBehavior::LOBBY_SERVER:
      return "lobby_server";
    case ServerBehavior::DATA_SERVER_BB:
      return "data_server_bb";
    case ServerBehavior::PATCH_SERVER_PC:
      return "patch_server_pc";
    case ServerBehavior::PATCH_SERVER_BB:
      return "patch_server_bb";
    case ServerBehavior::PROXY_SERVER:
      return "proxy_server";
    default:
      throw logic_error("invalid server behavior");
  }
}

ServerBehavior server_behavior_for_name(const char* name) {
  if (!strcasecmp(name, "split_reconnect")) {
    return ServerBehavior::SPLIT_RECONNECT;
  } else if (!strcasecmp(name, "login_server") || !strcasecmp(name, "login")) {
    return ServerBehavior::LOGIN_SERVER;
  } else if (!strcasecmp(name, "lobby_server") || !strcasecmp(name, "lobby")) {
    return ServerBehavior::LOBBY_SERVER;
  } else if (!strcasecmp(name, "data_server_bb") || !strcasecmp(name, "data_server") || !strcasecmp(name, "data")) {
    return ServerBehavior::DATA_SERVER_BB;
  } else if (!strcasecmp(name, "patch_server_pc") || !strcasecmp(name, "patch_pc")) {
    return ServerBehavior::PATCH_SERVER_PC;
  } else if (!strcasecmp(name, "patch_server_bb") || !strcasecmp(name, "patch_bb")) {
    return ServerBehavior::PATCH_SERVER_BB;
  } else if (!strcasecmp(name, "proxy_server") || !strcasecmp(name, "proxy")) {
    return ServerBehavior::PROXY_SERVER;
  } else {
    throw invalid_argument("incorrect server behavior name");
  }
}
