#pragma once

#include <map>
namespace data {
	struct StatInfo {
		std::string name;
		bool isPercentage = false;
	};

	const inline std::map<uint32_t, StatInfo> statMap = {
		{11102, StatInfo{"hp_", true}},
		{11103, StatInfo{"hp", false}},
		{12102, StatInfo{"atk_", true}},
		{12103, StatInfo{"atk", false}},
		{12202, StatInfo{"impact_", true}},
		{13102, StatInfo{"def_", true}},
		{13103, StatInfo{"def", false}},
		{20103, StatInfo{"crit_", true}},
		{21103, StatInfo{"crit_dmg_", true}},
		{23103, StatInfo{"pen_", true}},
		{23203, StatInfo{"pen", false}},
		{30502, StatInfo{"enerRegen_", true}},
		{31203, StatInfo{"anomProf", false}},
		{31402, StatInfo{"anomMas_", true}},
		{31503, StatInfo{"physical_dmg_", true}},
		{31603, StatInfo{"fire_dmg_", true}},
		{31703, StatInfo{"ice_dmg_", true}},
		{31803, StatInfo{"electric_dmg_", true}},
		{31903, StatInfo{"ether_dmg_", true}},
		{32303, StatInfo{"wind_dmg_", true}},
	};
}// namespace data