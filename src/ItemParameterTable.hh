#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <phosg/Encoding.hh>

#include "Text.hh"
#include "ItemData.hh"



class ItemParameterTable {
public:
  struct ItemBase {
    le_uint32_t id;
    le_uint16_t type;
    le_uint16_t skin;
    le_uint32_t team_points;
  } __attribute__((packed));

  struct ArmorOrShield {
    ItemBase base;
    le_uint16_t dfp;
    le_uint16_t evp;
    uint8_t block_particle;
    uint8_t block_effect;
    uint8_t item_class;
    uint8_t unknown_a1;
    uint8_t required_level;
    uint8_t efr;
    uint8_t eth;
    uint8_t eic;
    uint8_t edk;
    uint8_t elt;
    uint8_t dfp_range;
    uint8_t evp_range;
    uint8_t stat_boost;
    uint8_t tech_boost;
    le_uint16_t unknown_a2;
  } __attribute__((packed));

  struct Unit {
    ItemBase base;
    le_uint16_t stat;
    le_uint16_t stat_amount;
    le_int16_t modifier_amount;
    parray<uint8_t, 2> unused;
  } __attribute__((packed));

  struct Mag {
    ItemBase base;
    uint8_t feed_table;
    uint8_t unknown_a1;
    uint8_t photon_blast;
    uint8_t activation;
    uint8_t on_pb_full;
    uint8_t on_low_hp;
    uint8_t on_death;
    uint8_t on_boss;
    uint8_t on_pb_full_flag;
    uint8_t on_low_hp_flag;
    uint8_t on_death_flag;
    uint8_t on_boss_flag;
    uint8_t item_class;
    parray<uint8_t, 3> unused;
  } __attribute__((packed));

  struct Tool {
    ItemBase base;
    le_uint16_t amount;
    le_uint16_t tech;
    le_int32_t cost;
    uint8_t item_flag;
    parray<uint8_t, 3> unused;
  } __attribute__((packed));

  struct Weapon {
    ItemBase base;
    uint8_t item_class;
    uint8_t unknown_a0;
    le_uint16_t atp_min;
    le_uint16_t atp_max;
    le_uint16_t atp_required;
    le_uint16_t mst_required;
    le_uint16_t ata_required;
    le_uint16_t mst;
    uint8_t max_grind;
    uint8_t photon;
    uint8_t special;
    uint8_t ata;
    uint8_t stat_boost;
    uint8_t projectile;
    int8_t trail1_x;
    int8_t trail1_y;
    int8_t trail2_x;
    int8_t trail2_y;
    int8_t color;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint8_t unknown_a3;
    uint8_t unknown_a4;
    uint8_t unknown_a5;
    uint8_t tech_boost;
    uint8_t combo_type;
  } __attribute__((packed));

  struct MagFeedResult {
    int8_t defense;
    int8_t power;
    int8_t dexterity;
    int8_t mind;
    int8_t iq;
    int8_t sync;
    parray<uint8_t, 2> unused;
  } __attribute__((packed));

  struct MagFeedResultsList {
    parray<MagFeedResult, 11> results;
  } __attribute__((packed));

  struct MagFeedResultsTable {
    parray<MagFeedResultsList, 8> table;
  } __attribute__((packed));

  struct ItemStarValue {
    uint8_t num_stars;
  } __attribute__((packed));

  struct Special {
    le_uint16_t type;
    le_uint16_t amount;
  } __attribute__((packed));

  struct StatBoost {
    uint8_t stat1;
    uint8_t stat2;
    le_uint16_t amount1;
    le_uint16_t amount2;
  } __attribute__((packed));

  struct MaxTechniqueLevels {
    // Indexed as [tech_num][char_class]
    parray<parray<uint8_t, 12>, 19> max_level;
  } __attribute__((packed));

  struct ItemCombination {
    parray<uint8_t, 3> used_item;
    parray<uint8_t, 3> equipped_item;
    parray<uint8_t, 3> result_item;
    uint8_t maglevel;
    uint8_t grind;
    uint8_t level;
    uint8_t char_class;
    parray<uint8_t, 3> unused;
  } __attribute__((packed));

  struct TechniqueBoost {
    le_uint32_t tech1;
    le_float boost1;
    le_uint32_t tech2;
    le_float boost2;
    le_uint32_t tech3;
    le_float boost3;
  } __attribute__((packed));

  struct EventItem {
    parray<uint8_t, 3> item;
    uint8_t probability;
  } __attribute__((packed));

  struct UnsealableItem {
    parray<uint8_t, 3> item;
    uint8_t unused;
  } __attribute__((packed));

  struct NonWeaponSaleDivisors {
    le_float armor_divisor;
    le_float shield_divisor;
    le_float unit_divisor;
    le_float mag_divisor;
  } __attribute__((packed));

  struct TableOffsets {
    /* 00 / 14884 */ le_uint32_t weapon_table; // -> [{count, offset -> [Weapon]}](0xED)
    /* 04 / 1478C */ le_uint32_t armor_table; // -> [{count, offset -> [ArmorOrShield]}](2; armors and shields)
    /* 08 / 1479C */ le_uint32_t unit_table; // -> {count, offset -> [Unit]} (last if out of range)
    /* 0C / 147AC */ le_uint32_t tool_table; // -> [{count, offset -> [Tool]}](0x1A) (last if out of range)
    /* 10 / 147A4 */ le_uint32_t mag_table; // -> {count, offset -> [Mag]}
    /* 14 / 0F4B8 */ le_uint32_t attack_animation_table; // -> [uint8_t](0xED)
    /* 18 / 0DE7C */ le_uint32_t photon_color_table; // -> [0x24-byte structs](0x20)
    /* 1C / 0E194 */ le_uint32_t weapon_range_table; // -> ???
    /* 20 / 0F5A8 */ le_uint32_t weapon_sale_divisor_table; // -> [float](0xED)
    /* 24 / 0F83C */ le_uint32_t sale_divisor_table; // -> NonWeaponSaleDivisors
    /* 28 / 1502C */ le_uint32_t mag_feed_table; // -> [offset -> MagFeedResultsList](8)
    /* 2C / 0FB0C */ le_uint32_t star_value_table; // -> [uint8_t] (indexed by .id from weapon, armor, etc.)
    /* 30 / 0FE3C */ le_uint32_t special_data_table; // -> [Special]
    /* 34 / 0FEE0 */ le_uint32_t weapon_effect_table; // -> [16-byte structs]
    /* 38 / 1275C */ le_uint32_t stat_boost_table; // -> [StatBoost]
    /* 3C / 11C80 */ le_uint32_t shield_effect_table; // -> [8-byte structs]
    /* 40 / 12894 */ le_uint32_t max_tech_level_table; // -> MaxTechniqueLevels
    /* 44 / 14FF4 */ le_uint32_t combination_table; // -> [{count, offset -> [ItemCombination]}]
    /* 48 / 12754 */ le_uint32_t unknown_a1;
    /* 4C / 14278 */ le_uint32_t tech_boost_table; // -> [TechniqueBoost] (always 0x2C of them? from counts struct?)
    /* 50 / 15014 */ le_uint32_t unwrap_table; // -> {count, offset -> [{count, offset -> [4-byte structs]}]}
    /* 54 / 1501C */ le_uint32_t unsealable_table; // -> {count, offset -> [UnsealableItem]}
    /* 58 / 15024 */ le_uint32_t ranged_special_table; // -> {count, offset -> [4-byte structs]}
  } __attribute__((packed));

  struct CountAndOffset {
    le_uint32_t count;
    le_uint32_t offset;
  } __attribute__((packed));

  ItemParameterTable(std::shared_ptr<const std::string> data);
  ~ItemParameterTable() = default;

  const Weapon& get_weapon(uint8_t data1_1, uint8_t data1_2) const;
  const ArmorOrShield& get_armor_or_shield(uint8_t data1_1, uint8_t data1_2) const;
  const Unit& get_unit(uint8_t data1_2) const;
  const Tool& get_tool(uint8_t data1_1, uint8_t data1_2) const;
  std::pair<uint8_t, uint8_t> find_tool_by_class(uint8_t tool_class) const;
  const Mag& get_mag(uint8_t data1_1) const;
  float get_sale_divisor(uint8_t data1_0, uint8_t data1_1) const;
  const MagFeedResult& get_mag_feed_result(uint8_t table_index, uint8_t which) const;
  uint8_t get_item_stars(uint16_t slot) const;
  uint8_t get_max_tech_level(uint8_t char_class, uint8_t tech_num) const;

  const ItemBase& get_item_definition(const ItemData& item) const;
  uint8_t get_item_stars(const ItemData& item) const;
  bool is_item_rare(const ItemData& item) const;
  bool is_unsealable_item(const ItemData& param_1) const;

private:
  std::shared_ptr<const std::string> data;
  StringReader r;
  const TableOffsets* offsets;
};