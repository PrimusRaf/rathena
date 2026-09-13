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
		// Tale of <Region>: Barde/Taenzerin-Fassung, Klassenlinie der Region (tales_verdrahten.py)
		case SAFA_TALE_ALBERTA:
			return std::make_unique<SkillConcerto>(SAFA_TALE_ALBERTA, "con_alberta_m.wav", "con_alberta_f.wav", SC_SAFA_TALE_ALBERTA, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_MERCHANT } });
		case SAFA_TALE_ALDEBARAN:
			return std::make_unique<SkillConcerto>(SAFA_TALE_ALDEBARAN, "con_aldebaran_m.wav", "con_aldebaran_f.wav", SC_SAFA_TALE_ALDEBARAN, std::vector<s_tale_klasse>{ { MAPID_SECONDMASK, MAPID_ALCHEMIST } });
		case SAFA_TALE_AMATSU:
			return std::make_unique<SkillConcerto>(SAFA_TALE_AMATSU, "con_amatsu_m.wav", "con_amatsu_f.wav", SC_SAFA_TALE_AMATSU, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_NINJA } });
		case SAFA_TALE_COMODO:
			return std::make_unique<SkillConcerto>(SAFA_TALE_COMODO, "con_comodo_m.wav", "con_comodo_f.wav", SC_SAFA_TALE_COMODO, std::vector<s_tale_klasse>{ { MAPID_SECONDMASK, MAPID_BARDDANCER }, { MAPID_SECONDMASK, MAPID_ROGUE } });
		case SAFA_TALE_EINBROCH:
			return std::make_unique<SkillConcerto>(SAFA_TALE_EINBROCH, "con_einbroch_m.wav", "con_einbroch_f.wav", SC_SAFA_TALE_EINBROCH, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_GUNSLINGER } });
		case SAFA_TALE_GEFFEN:
			return std::make_unique<SkillConcerto>(SAFA_TALE_GEFFEN, "con_geffen_m.wav", "con_geffen_f.wav", SC_SAFA_TALE_GEFFEN, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_MAGE } });
		case SAFA_TALE_HUGEL:
			return std::make_unique<SkillConcerto>(SAFA_TALE_HUGEL, "con_hugel_m.wav", "con_hugel_f.wav", SC_SAFA_TALE_HUGEL, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_ARCHER } });
		case SAFA_TALE_LASAGNA:
			return std::make_unique<SkillConcerto>(SAFA_TALE_LASAGNA, "con_lasagna_m.wav", "con_lasagna_f.wav", SC_SAFA_TALE_LASAGNA, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_SUMMONER } });
		case SAFA_TALE_MORROC:
			return std::make_unique<SkillConcerto>(SAFA_TALE_MORROC, "con_morroc_m.wav", "con_morroc_f.wav", SC_SAFA_TALE_MORROC, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_THIEF }, { MAPID_SECONDMASK, MAPID_SOUL_LINKER } });
		case SAFA_TALE_PAYON:
			return std::make_unique<SkillConcerto>(SAFA_TALE_PAYON, "con_payon_m.wav", "con_payon_f.wav", SC_SAFA_TALE_PAYON, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_ARCHER }, { MAPID_FIRSTMASK, MAPID_TAEKWON } });
		case SAFA_TALE_PRONTERA:
			return std::make_unique<SkillConcerto>(SAFA_TALE_PRONTERA, "con_prontera_m.wav", "con_prontera_f.wav", SC_SAFA_TALE_PRONTERA, std::vector<s_tale_klasse>{ { MAPID_FIRSTMASK, MAPID_SWORDMAN }, { MAPID_FIRSTMASK, MAPID_ACOLYTE }, { MAPID_FIRSTMASK, MAPID_NOVICE } });
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
