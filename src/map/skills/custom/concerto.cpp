// SafaRO - Concerto-Skills, siehe concerto.hpp und CONCERTO.md
#include "concerto.hpp"

#include <ctime>

#include <common/showmsg.hpp>

#include "map/clif.hpp"
#include "map/map.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"
#include "map/skill.hpp"

// Session-Variablen (@-Variablen, ueberleben keinen Relog - genau richtig,
// denn ein Relog beendet die Musik im Client ohnehin).
//   @concerto_bis        Unixzeit, bis zu der das EIGENE Concerto spielt
//   @concerto_map        Karte, auf der es gewirkt wurde
//   @concerto_hoert_bis  Unixzeit, bis zu der der Spieler ein Concerto hoert
//   @concerto_hoert_map  Karte, auf der er es zu hoeren begann
static const char* REG_BIS = "@concerto_bis";
static const char* REG_MAP = "@concerto_map";
static const char* REG_HOERT_BIS = "@concerto_hoert_bis";
static const char* REG_HOERT_MAP = "@concerto_hoert_map";

bool concerto_ist_skill(uint16 skill_id) {
	return skill_id == SAFA_CONCERTO_HERO || skill_id == SAFA_CONCERTO_RUSH;
}

static bool concerto_hoert_noch(map_session_data& sd, const char* reg_bis, const char* reg_map, int64 jetzt) {
	int64 bis = pc_readreg(&sd, add_str(reg_bis));
	int64 karte = pc_readreg(&sd, add_str(reg_map));
	return bis > jetzt && karte == sd.m;
}

bool concerto_darf_wirken(map_session_data& sd, uint16 skill_id) {
	int64 jetzt = static_cast<int64>(time(nullptr));
	if (concerto_hoert_noch(sd, REG_BIS, REG_MAP, jetzt)) {
		clif_displaymessage(sd.fd, "Your concerto is still playing.");
		clif_skill_fail(sd, skill_id);
		return false;
	}
	return true;
}

// Ein Spieler im Umkreis: Musik nur, wenn er gerade nichts hoert.
static int32 concerto_musik_sub(block_list* bl, va_list ap) {
	const char* wav = va_arg(ap, const char*);
	int64 jetzt = va_arg(ap, int64);
	int64 dauer_s = va_arg(ap, int64);

	map_session_data* sd = BL_CAST(BL_PC, bl);
	if (sd == nullptr)
		return 0;
	if (concerto_hoert_noch(*sd, REG_HOERT_BIS, REG_HOERT_MAP, jetzt))
		return 0;

	clif_soundeffect(*bl, wav, 0, SELF);
	pc_setreg(sd, add_str(REG_HOERT_BIS), jetzt + dauer_s);
	pc_setreg(sd, add_str(REG_HOERT_MAP), sd->m);
	return 1;
}

SkillConcerto::SkillConcerto(uint16 skill_id, const char* wav)
	: SkillImpl(static_cast<e_skill>(skill_id)), wav_(wav) {
}

void SkillConcerto::castendNoDamageId(block_list* src, block_list* target, uint16 skill_lv, t_tick tick, int32& flag) const {
	clif_skill_nodamage(src, *src, getSkillId(), skill_lv);
	anstimmen(src, src->x, src->y, skill_lv);
}

void SkillConcerto::castendPos2(block_list* src, int32 x, int32 y, uint16 skill_lv, t_tick tick, int32& flag) const {
	anstimmen(src, x, y, skill_lv);
}

void SkillConcerto::anstimmen(block_list* src, int32 x, int32 y, uint16 skill_lv) const {
	map_session_data* sd = BL_CAST(BL_PC, src);
	if (sd == nullptr)
		return;

	std::shared_ptr<s_skill_unit_group> group = skill_unitsetting(src, getSkillId(), skill_lv, x, y, 0);
	if (group == nullptr)
		return;

	int64 jetzt = static_cast<int64>(time(nullptr));
	int64 dauer_s = skill_get_time(getSkillId(), skill_lv) / 1000;   // Duration1 = Liedlaenge

	pc_setreg(sd, add_str(REG_BIS), jetzt + dauer_s);
	pc_setreg(sd, add_str(REG_MAP), src->m);

	// Hoerweite = Sichtweite des Clients um den Wirkpunkt.
	int32 n = map_foreachinallarea(concerto_musik_sub, src->m,
		x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, BL_PC,
		wav_, jetzt, dauer_s);

	if (battle_config.skill_log)
		ShowInfo("Concerto %d von %s: Musik an %d Spieler, %lld s\n", getSkillId(), sd->status.name, n, (long long)dauer_s);
}
