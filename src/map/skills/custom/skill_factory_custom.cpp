// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "skill_factory_custom.hpp"

#include "../swordman/bash.hpp"

// SafaRO: Concerto-Skills (CONCERTO.md). Als .cpp eingebunden wie in den
// Job-Factories - einzelne Skill-Dateien werden nicht separat uebersetzt.
#include "concerto.cpp"

class SkillCustomBash;

std::unique_ptr<const SkillImpl> SkillFactoryCustom::create(const e_skill skill_id) const {
	// SafaRO Concertos - Musikdatei relativ zu data\wav\ (musik.grf)
	switch (static_cast<uint16>(skill_id)) {
		case SAFA_CONCERTO_HERO:
			return std::make_unique<SkillConcerto>(SAFA_CONCERTO_HERO, "con_hero.wav");
		case SAFA_CONCERTO_RUSH:
			return std::make_unique<SkillConcerto>(SAFA_CONCERTO_RUSH, "con_rush.wav");
		case SAFA_CONCERTO_ELEPHANT:
			return std::make_unique<SkillConcerto>(SAFA_CONCERTO_ELEPHANT, "con_eleph.wav", CONCERTO_BUFF, SC_SAFA_ELEPHANT, SAFA_EFFEKT_ELEPHANT);
		case SAFA_CONCERTO_EAGLE:
			return std::make_unique<SkillConcerto>(SAFA_CONCERTO_EAGLE, "con_eagle.wav", CONCERTO_BUFF, SC_SAFA_EAGLE, SAFA_EFFEKT_EAGLE);
		case SAFA_CONCERTO_RHINO:
			return std::make_unique<SkillConcerto>(SAFA_CONCERTO_RHINO, "con_rhino.wav", CONCERTO_BUFF, SC_SAFA_RHINO, SAFA_EFFEKT_RHINO);
		case SAFA_CONCERTO_NIGHT:
			return std::make_unique<SkillConcerto>(SAFA_CONCERTO_NIGHT, "con_night.wav", CONCERTO_DEBUFF, SC_NONE, SAFA_EFFEKT_NIGHT);
	}

#if 0
	switch( skill_id ){
		case SM_BASH:
			return std::make_unique<SkillCustomBash>();

		default:
			return nullptr;
	}
#endif

	return nullptr;
}

class SkillCustomBash : public SkillBash{
	void calculateSkillRatio(const Damage* wd, const block_list* src, const block_list* target, uint16 skill_lv, int32& base_skillratio, int32 mflag) const override{
		// Normal Bash:
		// Base 100% + 30% per level
		// base_skillratio += 30 * skill_lv;

		// But my custom Bash Skill is stronger:
		// Base 100% + 300% per level
		base_skillratio += 300 * skill_lv;
	}
};
