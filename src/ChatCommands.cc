#include "ChatCommands.hh"

#include <string.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <phosg/Strings.hh>
#include <phosg/Time.hh>

#include "Server.hh"
#include "ProxyServer.hh"
#include "Lobby.hh"
#include "Client.hh"
#include "SendCommands.hh"
#include "Text.hh"
#include "StaticGameData.hh"

using namespace std;



////////////////////////////////////////////////////////////////////////////////
// Checks



class precondition_failed {
public:
  precondition_failed(const std::u16string& user_msg) : user_msg(user_msg) { }
  ~precondition_failed() = default;

  const std::u16string& what() const {
    return this->user_msg;
  }

private:
  std::u16string user_msg;
};

static void check_privileges(shared_ptr<Client> c, uint64_t mask) {
  if (!c->license) {
    throw precondition_failed(u"$C6You are not\nlogged in.");
  }
  if ((c->license->privileges & mask) != mask) {
    throw precondition_failed(u"$C6You do not have\npermission to\nrun this command.");
  }
}

static void check_version(shared_ptr<Client> c, GameVersion version) {
  if (c->version != version) {
    throw precondition_failed(u"$C6This command cannot\nbe used for your\nversion of PSO.");
  }
}

static void check_not_version(shared_ptr<Client> c, GameVersion version) {
  if (c->version == version) {
    throw precondition_failed(u"$C6This command cannot\nbe used for your\nversion of PSO.");
  }
}

static void check_is_game(shared_ptr<Lobby> l, bool is_game) {
  if (l->is_game() != is_game) {
    throw precondition_failed(is_game ?
        u"$C6This command cannot\nbe used in lobbies." :
        u"$C6This command cannot\nbe used in games.");
  }
}

static void check_is_ep3(shared_ptr<Client> c, bool is_ep3) {
  if (!!(c->flags & Client::Flag::EPISODE_3) != is_ep3) {
    throw precondition_failed(is_ep3 ?
        u"$C6This command can only\nbe used in Episode 3." :
        u"$C6This command cannot\nbe used in Episode 3.");
  }
}

static void check_cheats_enabled(shared_ptr<Lobby> l) {
  if (!(l->flags & Lobby::Flag::CHEATS_ENABLED)) {
    throw precondition_failed(u"$C6This command can\nonly be used in\ncheat mode.");
  }
}

static void check_is_leader(shared_ptr<Lobby> l, shared_ptr<Client> c) {
  if (l->leader_id != c->lobby_client_id) {
    throw precondition_failed(u"$C6This command can\nonly be used by\nthe game leader.");
  }
}



////////////////////////////////////////////////////////////////////////////////
// Message commands

static void server_command_lobby_info(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  // no preconditions - everyone can use this command

  if (!l) {
    send_text_message(c, u"$C6No lobby information");

  } else if (l->is_game()) {
    string level_string;
    if (l->max_level == 0xFFFFFFFF) {
      level_string = string_printf("Levels: %d+", l->min_level + 1);
    } else {
      level_string = string_printf("Levels: %d-%d", l->min_level + 1, l->max_level + 1);
    }

    send_text_message_printf(c,
        "$C6Game ID: %08X\n%s\nSection ID: %s\nCheat mode: %s",
        l->lobby_id, level_string.c_str(),
        name_for_section_id(l->section_id).c_str(),
        (l->flags & Lobby::Flag::CHEATS_ENABLED) ? "on" : "off");

  } else {
    size_t num_clients = l->count_clients();
    size_t max_clients = l->max_clients;
    send_text_message_printf(c, "$C6Lobby ID: %08X\nPlayers: %zu/%zu",
        l->lobby_id, num_clients, max_clients);
  }
}

static void proxy_command_lobby_info(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string&) {
  vector<const char*> cheats_tokens;
  if (session.switch_assist) {
    cheats_tokens.emplace_back("SWA");
  }
  if (session.infinite_hp) {
    cheats_tokens.emplace_back("HP");
  }
  if (session.infinite_tp) {
    cheats_tokens.emplace_back("TP");
  }
  string cheats_str = cheats_tokens.empty() ? "none" : join(cheats_tokens, ",");

  vector<const char*> behaviors_tokens;
  if (session.save_files) {
    behaviors_tokens.emplace_back("SF");
  }
  if (session.function_call_return_value >= 0) {
    behaviors_tokens.emplace_back("BFC");
  }
  string behaviors_str = behaviors_tokens.empty() ? "none" : join(behaviors_tokens, ",");

  string section_id_override = "none";
  if (session.override_section_id >= 0) {
    section_id_override = name_for_section_id(session.override_section_id);
  }

  send_text_message_printf(session.client_channel,
      "$C7GC: $C6%" PRIu32 "\n"
      "$C7Client ID: $C6%zu\n"
      "$C7Cheats: $C6%s\n"
      "$C7Flags: $C6%s\n"
      "$C7SecID override: $C6%s\n",
      session.remote_guild_card_number,
      session.lobby_client_id,
      cheats_str.c_str(),
      behaviors_str.c_str(),
      section_id_override.c_str());
}

static void server_command_ax(shared_ptr<ServerState>, shared_ptr<Lobby>,
    shared_ptr<Client> c, const std::u16string& args) {
  check_privileges(c, Privilege::ANNOUNCE);
  string message = encode_sjis(args);
  log(INFO, "[Client message from %010u] %s\n", c->license->serial_number, message.c_str());
}

static void server_command_announce(shared_ptr<ServerState> s, shared_ptr<Lobby>,
    shared_ptr<Client> c, const std::u16string& args) {
  check_privileges(c, Privilege::ANNOUNCE);
  send_text_message(s, args);
}

static void server_command_arrow(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  // no preconditions
  c->lobby_arrow_color = stoull(encode_sjis(args), nullptr, 0);
  if (!l->is_game()) {
    send_arrow_update(l);
  }
}

static void proxy_command_arrow(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string& args) {
  session.server_channel.send(0x89, stoull(encode_sjis(args), nullptr, 0));
}

static void server_command_dbgid(shared_ptr<ServerState>, shared_ptr<Lobby>,
    shared_ptr<Client> c, const std::u16string&) {
  c->prefer_high_lobby_client_id = !c->prefer_high_lobby_client_id;
}

////////////////////////////////////////////////////////////////////////////////
// Lobby commands

static void server_command_cheat(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  check_is_game(l, true);
  check_is_leader(l, c);

  l->flags ^= Lobby::Flag::CHEATS_ENABLED;
  send_text_message_printf(l, "Cheat mode %s",
      (l->flags & Lobby::Flag::CHEATS_ENABLED) ? "enabled" : "disabled");

  // if cheat mode was disabled, turn off all the cheat features that were on
  if (!(l->flags & Lobby::Flag::CHEATS_ENABLED)) {
    for (size_t x = 0; x < l->max_clients; x++) {
      auto c = l->clients[x];
      if (!c) {
        continue;
      }
      c->infinite_hp = false;
      c->infinite_tp = false;
      c->switch_assist = false;
    }
    l->next_drop_item = PlayerInventoryItem();
  }
}

static void server_command_lobby_event(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, false);
  check_privileges(c, Privilege::CHANGE_EVENT);

  uint8_t new_event = event_for_name(args);
  if (new_event == 0xFF) {
    send_text_message(c, u"$C6No such lobby event.");
    return;
  }

  l->event = new_event;
  send_change_event(l, l->event);
}

static void proxy_command_lobby_event(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string& args) {
  if (args.empty()) {
    session.override_lobby_event = -1;
  } else {
    uint8_t new_event = event_for_name(args);
    if (new_event == 0xFF) {
      send_text_message(session.client_channel, u"$C6No such lobby event.");
    } else {
      session.override_lobby_event = new_event;
      if (session.version == GameVersion::GC || session.version == GameVersion::BB) {
        session.client_channel.send(0xDA, session.override_lobby_event);
      }
    }
  }
}

static void server_command_lobby_event_all(shared_ptr<ServerState> s, shared_ptr<Lobby>,
    shared_ptr<Client> c, const std::u16string& args) {
  check_privileges(c, Privilege::CHANGE_EVENT);

  uint8_t new_event = event_for_name(args);
  if (new_event == 0xFF) {
    send_text_message(c, u"$C6No such lobby event.");
    return;
  }

  for (auto l : s->all_lobbies()) {
    if (l->is_game() || !(l->flags & Lobby::Flag::DEFAULT)) {
      continue;
    }

    l->event = new_event;
    send_change_event(l, l->event);
  }
}

static void server_command_lobby_type(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, false);
  check_privileges(c, Privilege::CHANGE_EVENT);

  uint8_t new_type = lobby_type_for_name(args);
  if (new_type == 0x80) {
    send_text_message(c, u"$C6No such lobby type.");
    return;
  }

  l->type = new_type;
  if (l->type < ((l->flags & Lobby::Flag::EPISODE_3_ONLY) ? 20 : 15)) {
    l->type = l->block - 1;
  }

  for (size_t x = 0; x < l->max_clients; x++) {
    if (l->clients[x]) {
      send_join_lobby(l->clients[x], l);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Game commands

static void server_command_secid(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, false);

  if (!args[0]) {
    c->override_section_id = -1;
    send_text_message(c, u"$C6Override section ID\nremoved");
  } else {
    uint8_t new_secid = section_id_for_name(args);
    if (new_secid == 0xFF) {
      send_text_message(c, u"$C6Invalid section ID");
    } else {
      c->override_section_id = new_secid;
      send_text_message(c, u"$C6Override section ID\nset");
    }
  }
}

static void proxy_command_secid(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string& args) {
  if (!args[0]) {
    session.override_section_id = -1;
    send_text_message(session.client_channel, u"$C6Override section ID\nremoved");
  } else {
    uint8_t new_secid = section_id_for_name(args);
    if (new_secid == 0xFF) {
      send_text_message(session.client_channel, u"$C6Invalid section ID");
    } else {
      session.override_section_id = new_secid;
      send_text_message(session.client_channel, u"$C6Override section ID\nset");
    }
  }
}

static void server_command_password(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, true);
  check_is_leader(l, c);

  if (!args[0]) {
    l->password[0] = 0;
    send_text_message(l, u"$C6Game unlocked");

  } else {
    l->password = args;
    auto encoded = encode_sjis(l->password);
    send_text_message_printf(l, "$C6Game password:\n%s",
        encoded.c_str());
  }
}

static void server_command_min_level(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, true);
  check_is_leader(l, c);

  u16string buffer;
  l->min_level = stoull(encode_sjis(args)) - 1;
  send_text_message_printf(l, "$C6Minimum level set to %" PRIu32,
      l->min_level + 1);
}

static void server_command_max_level(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, true);
  check_is_leader(l, c);

  l->max_level = stoull(encode_sjis(args)) - 1;
  if (l->max_level >= 200) {
    l->max_level = 0xFFFFFFFF;
  }

  if (l->max_level == 0xFFFFFFFF) {
    send_text_message(l, u"$C6Maximum level set to unlimited");
  } else {
    send_text_message_printf(l, "$C6Maximum level set to %" PRIu32, l->max_level + 1);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Character commands

static void server_command_edit(shared_ptr<ServerState> s, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, false);
  check_version(c, GameVersion::BB);

  string encoded_args = encode_sjis(args);
  vector<string> tokens = split(encoded_args, ' ');

  if (tokens.size() < 3) {
    send_text_message(c, u"$C6Not enough arguments");
    return;
  }

  if (tokens[0] == "atp") {
    c->game_data.player()->disp.stats.atp = stoul(tokens[1]);
  } else if (tokens[0] == "mst") {
    c->game_data.player()->disp.stats.mst = stoul(tokens[1]);
  } else if (tokens[0] == "evp") {
    c->game_data.player()->disp.stats.evp = stoul(tokens[1]);
  } else if (tokens[0] == "hp") {
    c->game_data.player()->disp.stats.hp = stoul(tokens[1]);
  } else if (tokens[0] == "dfp") {
    c->game_data.player()->disp.stats.dfp = stoul(tokens[1]);
  } else if (tokens[0] == "ata") {
    c->game_data.player()->disp.stats.ata = stoul(tokens[1]);
  } else if (tokens[0] == "lck") {
    c->game_data.player()->disp.stats.lck = stoul(tokens[1]);
  } else if (tokens[0] == "meseta") {
    c->game_data.player()->disp.meseta = stoul(tokens[1]);
  } else if (tokens[0] == "exp") {
    c->game_data.player()->disp.experience = stoul(tokens[1]);
  } else if (tokens[0] == "level") {
    c->game_data.player()->disp.level = stoul(tokens[1]) - 1;
  } else if (tokens[0] == "namecolor") {
    uint32_t new_color;
    sscanf(tokens[1].c_str(), "%8X", &new_color);
    c->game_data.player()->disp.name_color = new_color;
  } else if (tokens[0] == "secid") {
    uint8_t secid = section_id_for_name(decode_sjis(tokens[1]));
    if (secid == 0xFF) {
      send_text_message(c, u"$C6No such section ID.");
      return;
    } else {
      c->game_data.player()->disp.section_id = secid;
    }
  } else if (tokens[0] == "name") {
    c->game_data.player()->disp.name = add_language_marker(tokens[1], 'J');
  } else if (tokens[0] == "npc") {
    if (tokens[1] == "none") {
      c->game_data.player()->disp.extra_model = 0;
      c->game_data.player()->disp.v2_flags &= 0xFD;
    } else {
      uint8_t npc = npc_for_name(decode_sjis(tokens[1]));
      if (npc == 0xFF) {
        send_text_message(c, u"$C6No such NPC.");
        return;
      }
      c->game_data.player()->disp.extra_model = npc;
      c->game_data.player()->disp.v2_flags |= 0x02;
    }
  } else if ((tokens[0] == "tech") && (tokens.size() > 2)) {
    uint8_t level = stoul(tokens[2]) - 1;
    if (tokens[1] == "all") {
      for (size_t x = 0; x < 0x14; x++) {
        c->game_data.player()->disp.technique_levels.data()[x] = level;
      }
    } else {
      uint8_t tech_id = technique_for_name(decode_sjis(tokens[1]));
      if (tech_id == 0xFF) {
        send_text_message(c, u"$C6No such technique.");
        return;
      }
      c->game_data.player()->disp.technique_levels.data()[tech_id] = level;
    }
  } else {
    send_text_message(c, u"$C6Unknown field.");
    return;
  }

  // reload the client in the lobby/game
  send_player_leave_notification(l, c->lobby_client_id);
  send_complete_player_bb(c);
  s->send_lobby_join_notifications(l, c);
}

// TODO: implement this
// TODO: make sure the bank name is filesystem-safe
/* static void server_command_change_bank(shared_ptr<ServerState>, shared_ptr<Lobby>,
    shared_ptr<Client> c, const std::u16string&) {
  check_version(c, GameVersion::BB);

  TODO
} */

// TODO: This can be implemented on the proxy server too.
static void server_command_convert_char_to_bb(shared_ptr<ServerState> s,
    shared_ptr<Lobby> l, shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, false);
  check_not_version(c, GameVersion::BB);

  vector<string> tokens = split(encode_sjis(args), L' ');
  if (tokens.size() != 3) {
    send_text_message(c, u"$C6Incorrect argument count");
    return;
  }

  // username/password are tokens[0] and [1]
  c->pending_bb_save_player_index = stoul(tokens[2]) - 1;
  if (c->pending_bb_save_player_index > 3) {
    send_text_message(c, u"$C6Player index must be 1-4");
    return;
  }

  try {
    s->license_manager->verify_bb(tokens[0].c_str(), tokens[1].c_str());
  } catch (const exception& e) {
    send_text_message_printf(c, "$C6Login failed: %s", e.what());
    return;
  }

  c->pending_bb_save_username = tokens[0];

  // request the player data. the client will respond with a 61, and the handler
  // for that command will execute the conversion
  send_command(c, 0x95, 0x00);
}

////////////////////////////////////////////////////////////////////////////////
// Administration commands

static string name_for_client(shared_ptr<Client> c) {
  auto player = c->game_data.player(false);
  if (player.get()) {
    return encode_sjis(player->disp.name);
  }

  if (c->license.get()) {
    return string_printf("SN:%" PRIu32, c->license->serial_number);
  }

  return "Player";
}

static void server_command_silence(shared_ptr<ServerState> s, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_privileges(c, Privilege::SILENCE_USER);

  auto target = s->find_client(&args);
  if (!target->license) {
    // this should be impossible, but I'll bet it's not actually
    send_text_message(c, u"$C6Client not logged in");
    return;
  }

  if (target->license->privileges & Privilege::MODERATOR) {
    send_text_message(c, u"$C6You do not have\nsufficient privileges.");
    return;
  }

  target->can_chat = !target->can_chat;
  string target_name = name_for_client(target);
  send_text_message_printf(l, "$C6%s %ssilenced", target_name.c_str(),
      target->can_chat ? "un" : "");
}

static void server_command_kick(shared_ptr<ServerState> s, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_privileges(c, Privilege::KICK_USER);

  auto target = s->find_client(&args);
  if (!target->license) {
    // This should be impossible, but I'll bet it's not actually
    send_text_message(c, u"$C6Client not logged in");
    return;
  }

  if (target->license->privileges & Privilege::MODERATOR) {
    send_text_message(c, u"$C6You do not have\nsufficient privileges.");
    return;
  }

  send_message_box(target, u"$C6You were kicked off by a moderator.");
  target->should_disconnect = true;
  string target_name = name_for_client(target);
  send_text_message_printf(l, "$C6%s kicked off", target_name.c_str());
}

static void server_command_ban(shared_ptr<ServerState> s, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_privileges(c, Privilege::BAN_USER);

  u16string args_str(args);
  size_t space_pos = args_str.find(L' ');
  if (space_pos == string::npos) {
    send_text_message(c, u"$C6Incorrect argument count");
    return;
  }

  u16string identifier = args_str.substr(space_pos + 1);
  auto target = s->find_client(&identifier);
  if (!target->license) {
    // This should be impossible, but I'll bet it's not actually
    send_text_message(c, u"$C6Client not logged in");
    return;
  }

  if (target->license->privileges & Privilege::BAN_USER) {
    send_text_message(c, u"$C6You do not have\nsufficient privileges.");
    return;
  }

  uint64_t usecs = stoull(encode_sjis(args), nullptr, 0) * 1000000;

  size_t unit_offset = 0;
  for (; isdigit(args[unit_offset]); unit_offset++);
  if (args[unit_offset] == 'm') {
    usecs *= 60;
  } else if (args[unit_offset] == 'h') {
    usecs *= 60 * 60;
  } else if (args[unit_offset] == 'd') {
    usecs *= 60 * 60 * 24;
  } else if (args[unit_offset] == 'w') {
    usecs *= 60 * 60 * 24 * 7;
  } else if (args[unit_offset] == 'M') {
    usecs *= 60 * 60 * 24 * 30;
  } else if (args[unit_offset] == 'y') {
    usecs *= 60 * 60 * 24 * 365;
  }

  // TODO: put the length of time in this message. or don't; presumably the
  // person deserved it
  s->license_manager->ban_until(target->license->serial_number, now() + usecs);
  send_message_box(target, u"$C6You were banned by a moderator.");
  target->should_disconnect = true;
  string target_name = name_for_client(target);
  send_text_message_printf(l, "$C6%s banned", target_name.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// Cheat commands

static void server_command_warp(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, true);
  check_cheats_enabled(l);

  uint32_t area = stoul(encode_sjis(args), nullptr, 0);
  if (!l->episode || (l->episode > 3)) {
    return;
  }
  if (c->area == area) {
    return;
  }

  if ((l->episode == 1) && (area > 17)) {
    send_text_message(c, u"$C6Area numbers must be\n17 or less.");
    return;
  }
  if ((l->episode == 2) && (area > 17)) {
    send_text_message(c, u"$C6Area numbers must be\n17 or less.");
    return;
  }
  if ((l->episode == 3) && (area > 10)) {
    send_text_message(c, u"$C6Area numbers must be\n10 or less.");
    return;
  }

  send_warp(c, area);
}

static void proxy_command_warp(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string& args) {
  uint32_t area = stoul(encode_sjis(args), nullptr, 0);
  send_warp(session.client_channel, session.lobby_client_id, area);
}

static void server_command_next(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  check_is_game(l, true);
  check_cheats_enabled(l);

  if (!l->episode || (l->episode > 3)) {
    return;
  }

  uint8_t new_area = c->area + 1;
  if (((l->episode == 1) && (new_area > 17)) ||
      ((l->episode == 2) && (new_area > 17)) ||
      ((l->episode == 3) && (new_area > 10))) {
    new_area = 0;
  }

  send_warp(c, new_area);
}

static void server_command_what(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  check_is_game(l, true);
  if (!l->episode || (l->episode > 3)) {
    return;
  }
  if (!(l->flags & Lobby::Flag::ITEM_TRACKING_ENABLED)) {
    send_text_message(c, u"$C4Item tracking is off");
  } else {
    float min_dist2 = 0.0f;
    uint32_t nearest_item_id = 0xFFFFFFFF;
    for (const auto& it : l->item_id_to_floor_item) {
      if (it.second.area != c->area) {
        continue;
      }
      float dx = it.second.x - c->x;
      float dz = it.second.z - c->z;
      float dist2 = (dx * dx) + (dz * dz);
      if ((nearest_item_id == 0xFFFFFFFF) || (dist2 < min_dist2)) {
        nearest_item_id = it.first;
        min_dist2 = dist2;
      }
    }

    if (nearest_item_id == 0xFFFFFFFF) {
      send_text_message(c, u"$C4No items are near you");
    } else {
      const auto& item = l->item_id_to_floor_item.at(nearest_item_id);
      string name = name_for_item(item.inv_item.data, true);
      send_text_message(c, decode_sjis(name));
    }
  }
}

static void server_command_song(shared_ptr<ServerState>, shared_ptr<Lobby>,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_ep3(c, true);

  uint32_t song = stoul(encode_sjis(args), nullptr, 0);
  send_ep3_change_music(c, song);
}

static void server_command_infinite_hp(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  check_is_game(l, true);
  check_cheats_enabled(l);

  c->infinite_hp = !c->infinite_hp;
  send_text_message_printf(c, "$C6Infinite HP %s", c->infinite_hp ? "enabled" : "disabled");
}

static void proxy_command_infinite_hp(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string&) {
  session.infinite_hp = !session.infinite_hp;
  send_text_message_printf(session.client_channel, "$C6Infinite HP %s",
      session.infinite_hp ? "enabled" : "disabled");
}

static void server_command_infinite_tp(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  check_is_game(l, true);
  check_cheats_enabled(l);

  c->infinite_tp = !c->infinite_tp;
  send_text_message_printf(c, "$C6Infinite TP %s", c->infinite_tp ? "enabled" : "disabled");
}

static void proxy_command_infinite_tp(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string&) {
  session.infinite_tp = !session.infinite_tp;
  send_text_message_printf(session.client_channel, "$C6Infinite TP %s",
      session.infinite_tp ? "enabled" : "disabled");
}

static void server_command_switch_assist(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string&) {
  check_is_game(l, true);
  check_cheats_enabled(l);

  c->switch_assist = !c->switch_assist;
  send_text_message_printf(c, "$C6Switch assist %s", c->switch_assist ? "enabled" : "disabled");
}

static void proxy_command_switch_assist(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string&) {
  session.switch_assist = !session.switch_assist;
  send_text_message_printf(session.client_channel, "$C6Switch assist %s", session.switch_assist ? "enabled" : "disabled");
}

static void server_command_item(shared_ptr<ServerState>, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args) {
  check_is_game(l, true);
  check_cheats_enabled(l);

  string data = parse_data_string(encode_sjis(args));
  if (data.size() < 2) {
    send_text_message(c, u"$C6Item codes must be\n2 bytes or more.");
    return;
  }
  if (data.size() > 16) {
    send_text_message(c, u"$C6Item codes must be\n16 bytes or fewer.");
    return;
  }

  ItemData item_data;
  if (data.size() <= 12) {
    memcpy(&l->next_drop_item.data.data1, data.data(), data.size());
  } else {
    memcpy(&l->next_drop_item.data.data1, data.data(), 12);
    memcpy(&l->next_drop_item.data.data2, data.data() + 12, 12 - data.size());
  }

  send_text_message(c, u"$C6Next drop chosen.");
}



////////////////////////////////////////////////////////////////////////////////

typedef void (*server_handler_t)(shared_ptr<ServerState> s, shared_ptr<Lobby> l,
    shared_ptr<Client> c, const std::u16string& args);
typedef void (*proxy_handler_t)(shared_ptr<ServerState>,
    ProxyServer::LinkedSession& session, const std::u16string& args);
struct ChatCommandDefinition {
  server_handler_t server_handler;
  proxy_handler_t proxy_handler;
  u16string usage;
};

static const unordered_map<u16string, ChatCommandDefinition> chat_commands({
    // TODO: implement command_help and actually use the usage strings here
    {u"$allevent"  , {server_command_lobby_event_all   , nullptr                    , u"Usage:\nallevent <name/ID>"}},
    {u"$ann"       , {server_command_announce          , nullptr                    , u"Usage:\nann <message>"}},
    {u"$arrow"     , {server_command_arrow             , proxy_command_arrow        , u"Usage:\narrow <color>"}},
    {u"$ax"        , {server_command_ax                , nullptr                    , u"Usage:\nax <message>"}},
    {u"$ban"       , {server_command_ban               , nullptr                    , u"Usage:\nban <name-or-number>"}},
    // TODO: implement this on proxy server
    {u"$bbchar"    , {server_command_convert_char_to_bb, nullptr                    , u"Usage:\nbbchar <user> <pass> <1-4>"}},
    {u"$cheat"     , {server_command_cheat             , nullptr                    , u"Usage:\ncheat"}},
    {u"$dbgid"     , {server_command_dbgid             , nullptr                    , u"Usage:\ndbgid"}},
    {u"$edit"      , {server_command_edit              , nullptr                    , u"Usage:\nedit <stat> <value>"}},
    {u"$event"     , {server_command_lobby_event       , proxy_command_lobby_event  , u"Usage:\nevent <name>"}},
    {u"$infhp"     , {server_command_infinite_hp       , proxy_command_infinite_hp  , u"Usage:\ninfhp"}},
    {u"$inftp"     , {server_command_infinite_tp       , proxy_command_infinite_tp  , u"Usage:\ninftp"}},
    {u"$item"      , {server_command_item              , nullptr                    , u"Usage:\nitem <item-code>"}},
    {u"$kick"      , {server_command_kick              , nullptr                    , u"Usage:\nkick <name-or-number>"}},
    {u"$li"        , {server_command_lobby_info        , proxy_command_lobby_info   , u"Usage:\nli"}},
    {u"$maxlevel"  , {server_command_max_level         , nullptr                    , u"Usage:\nmax_level <level>"}},
    {u"$minlevel"  , {server_command_min_level         , nullptr                    , u"Usage:\nmin_level <level>"}},
    // TODO: implement this on proxy server
    {u"$next"      , {server_command_next              , nullptr                    , u"Usage:\nnext"}},
    {u"$password"  , {server_command_password          , nullptr                    , u"Usage:\nlock [password]\nomit password to\nunlock game"}},
    {u"$secid"     , {server_command_secid             , proxy_command_secid        , u"Usage:\nsecid [section ID]\nomit section ID to\nrevert to normal"}},
    {u"$silence"   , {server_command_silence           , nullptr                    , u"Usage:\nsilence <name-or-number>"}},
    // TODO: implement this on proxy server
    {u"$song"      , {server_command_song              , nullptr                    , u"Usage:\nsong <song-number>"}},
    {u"$swa"       , {server_command_switch_assist     , proxy_command_switch_assist, u"Usage:\nswa"}},
    {u"$type"      , {server_command_lobby_type        , nullptr                    , u"Usage:\ntype <name>"}},
    {u"$warp"      , {server_command_warp              , proxy_command_warp         , u"Usage:\nwarp <area-number>"}},
    {u"$what"      , {server_command_what              , nullptr                    , u"Usage:\nwhat"}},
});

struct SplitCommand {
  u16string name;
  u16string args;

  SplitCommand(const u16string& text) {
    size_t space_pos = text.find(u' ');
    if (space_pos != string::npos) {
      this->name = text.substr(0, space_pos);
      this->args = text.substr(space_pos + 1);
    } else {
      this->name = text;
    }
  }
};

// This function is called every time any player sends a chat beginning with a
// dollar sign. It is this function's responsibility to see if the chat is a
// command, and to execute the command and block the chat if it is.
void process_chat_command(std::shared_ptr<ServerState> s, std::shared_ptr<Lobby> l,
    std::shared_ptr<Client> c, const std::u16string& text) {
  SplitCommand cmd(text);

  const ChatCommandDefinition* def = nullptr;
  try {
    def = &chat_commands.at(cmd.name);
  } catch (const out_of_range&) {
    send_text_message(c, u"$C6Unknown command");
    return;
  }

  if (!def->server_handler) {
    send_text_message(c, u"$C6Command not available\non game server");
  } else {
    try {
      def->server_handler(s, l, c, cmd.args);
    } catch (const precondition_failed& e) {
      send_text_message(c, e.what());
    } catch (const exception& e) {
      send_text_message_printf(c, "$C6Failed:\n%s", e.what());
    }
  }
}

void process_chat_command(std::shared_ptr<ServerState> s,
    ProxyServer::LinkedSession& session, const std::u16string& text) {
  SplitCommand cmd(text);

  const ChatCommandDefinition* def = nullptr;
  try {
    def = &chat_commands.at(cmd.name);
  } catch (const out_of_range&) {
    send_text_message(session.client_channel, u"$C6Unknown command");
    return;
  }

  if (!def->proxy_handler) {
    send_text_message(session.client_channel, u"$C6Command not available\non proxy server");
  } else {
    try {
      def->proxy_handler(s, session, cmd.args);
    } catch (const precondition_failed& e) {
      send_text_message(session.client_channel, e.what());
    } catch (const exception& e) {
      send_text_message_printf(session.client_channel, "$C6Failed:\n%s", e.what());
    }
  }
}
