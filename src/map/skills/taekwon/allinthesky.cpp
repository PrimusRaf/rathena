// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "allinthesky.hpp"

#include "map/clif.hpp"
#include "map/pc.hpp"
#include "map/status.hpp"
#include "map/unit.hpp"

SkillAllInTheSky::SkillAllInTheSky() : SkillImpl(SKE_ALL_IN_THE_SKY) {
}

void SkillAllInTheSky::castendDamageId(block_list* src, block_list* target, uint16 skill_lv, t_tick tick, int32& flag) const {
	if (target->type == BL_PC)
		status_zap(target, 0, 0, status_get_ap(target));
	if( unit_movepos( src, target->x, target->y, 2, true ) ){
		clif_snap(src, src->x, src->y);
	}
	skill_attack(skill_get_type(getSkillId()), src, src, target, getSkillId(), skill_lv, tick, flag);
}

// SafaRO: Der Parameter heisst hier bewusst skillratio und nicht
// base_skillratio wie in der Oberklasse. RE_LVL_DMOD (config/const.hpp)
// schreibt fest auf eine Variable dieses Namens - mit base_skillratio
// uebersetzt die Datei nicht. Genau daran ist die Zeile urspruenglich
// gescheitert; roundtrip.cpp zeigt dasselbe Muster.
void SkillAllInTheSky::calculateSkillRatio(const Damage* wd, const block_list* src, const block_list* target, uint16 skill_lv, int32& skillratio, int32 mflag) const {
	const status_data* sstatus = status_get_status_data(*src);

	skillratio += -100 + 250 + 1200 * skill_lv;
	skillratio += 5 * sstatus->pow;
	// SafaRO: Diese Zeile fehlte. SKE_ALL_IN_THE_SKY war die EINZIGE
	// Fertigkeit einer vierten Klasse ohne den Levelbonus - auf
	// Stufe 500 blieb sie bei 13.150 Prozent stehen, waehrend jede
	// andere Fertigkeit verfuenffacht wird. Der Vorzeigeskill des
	// Sky Emperor war damit schwaecher als seine Nebenfertigkeiten.
	RE_LVL_DMOD(100);
}

void SkillAllInTheSky::modifyDamageData(Damage& dmg, const block_list& src, const block_list& target, uint16 skill_lv) const {
	switch (status_get_race(&target)) {
		case RC_DEMIHUMAN:
		case RC_DEMON:
			dmg.div_ = 3;
			break;
	}
}
