// SafaRO - Concerto-Skills, siehe concerto.hpp und CONCERTO.md
#include "concerto.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>

#include <common/core.hpp>      // db_path
#include <common/showmsg.hpp>
#include <common/timer.hpp>

#include "map/clif.hpp"
#include "map/map.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"
#include "map/skill.hpp"
#include "map/status.hpp"

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
static const char* REG_GRUPPE = "@concerto_gruppe";       // group_id der laufenden Flaeche

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

void concerto_mitziehen(block_list* bl, int16 dx, int16 dy) {
	if (bl == nullptr || bl->type != BL_PC || (dx == 0 && dy == 0))
		return;
	map_session_data* sd = BL_CAST(BL_PC, bl);
	int64 gid = pc_readreg(sd, add_str(REG_GRUPPE));
	if (gid == 0)
		return;
	std::shared_ptr<s_skill_unit_group> group = skill_id2group(static_cast<int32>(gid));
	if (group == nullptr || group->src_id != bl->id || !concerto_ist_skill(group->skill_id)) {
		pc_setreg(sd, add_str(REG_GRUPPE), 0);   // abgelaufen oder fremd - vergessen
		return;
	}
	skill_unit_move_unit_group(group, bl->m, dx, dy);
}

void concerto_nachschicken(map_session_data& sd) {
	for (uint16 id : { SAFA_CONCERTO_HERO, SAFA_CONCERTO_RUSH }) {
		uint16 idx = skill_get_index(id);
		if (idx == 0 || sd.status.skill[idx].id != id || sd.status.skill[idx].lv == 0)
			continue;
		clif_addskill(sd, id);
		clif_skillinfo(sd, id);
	}
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

// Schadensfaktor je Tick ist als Basis x1000 gedacht; 35000 % = 100 % + 34900.
constexpr int32 CONCERTO_RATIO_PLUS = 34900;

// Timer-Daten: group_id * 1024 + faktor (faktor <= 1000, passt in 10 Bit
// und in die 12 Bit, die skill_attack als flag an battle_calc weiterreicht).
static TIMER_FUNC(concerto_tick_timer);

SkillConcerto::SkillConcerto(uint16 skill_id, const char* wav)
	: SkillImpl(static_cast<e_skill>(skill_id)), wav_(wav) {
	static bool registriert = false;
	if (!registriert) {
		add_timer_func_list(concerto_tick_timer, "concerto_tick_timer");
		registriert = true;
	}
	beats_laden();
}

void SkillConcerto::beats_laden() {
	std::string pfad = std::string(db_path) + "/import/concerto/" + std::to_string(getSkillId()) + ".beats";
	std::ifstream in(pfad);
	if (!in) {
		ShowWarning("Concerto %d: keine Beat-Map %s - Fallback auf Unit.Interval\n", getSkillId(), pfad.c_str());
		return;
	}
	std::string zeile;
	while (std::getline(in, zeile)) {
		if (zeile.empty() || zeile[0] == '#')
			continue;
		std::istringstream z(zeile);
		int32 ms = 0; double f = 0;
		if (!(z >> ms >> f))
			continue;
		int32 fak = static_cast<int32>(std::lround(f * 1000));
		if (ms < 0 || fak <= 0)
			continue;
		beats_.push_back({ ms, std::min(fak, 1000) });
	}
	ShowStatus("Concerto %d: %zu Ticks aus %s\n", getSkillId(), beats_.size(), pfad.c_str());
}

void SkillConcerto::calculateSkillRatio(const Damage* wd, const block_list* src, const block_list* target, uint16 skill_lv, int32& base_skillratio, int32 mflag) const {
	int32 fak = (mflag > 0 && mflag <= 1000) ? mflag : 1000;   // Unit-Tick-Fallback = voll
	base_skillratio = (base_skillratio + CONCERTO_RATIO_PLUS) * fak / 1000;
}

// Ein Ziel in der Flaeche: Schaden vom Concerto, Quelle der Anzeige ist die Mittelzelle.
static int32 concerto_schaden_sub(block_list* bl, va_list ap) {
	block_list* src = va_arg(ap, block_list*);
	block_list* mitte = va_arg(ap, block_list*);
	s_skill_unit_group* group = va_arg(ap, s_skill_unit_group*);
	int32 fak = va_arg(ap, int32);
	t_tick tick = va_arg(ap, t_tick);

	if (bl->prev == nullptr || status_isdead(*bl))
		return 0;
	if (battle_check_target(mitte, bl, group->target_flag) <= 0)
		return 0;
	// Angriffsart aus der skill_db: Onslaught = Weapon, Magical Rush = Magic
	skill_attack(skill_get_type(group->skill_id), src, mitte, bl, group->skill_id, group->skill_lv, tick, fak);
	return 1;
}

static TIMER_FUNC(concerto_tick_timer) {
	block_list* src = map_id2bl(id);
	if (src == nullptr)
		return 0;
	int32 gid = static_cast<int32>(data / 1024);
	int32 fak = static_cast<int32>(data % 1024);
	std::shared_ptr<s_skill_unit_group> group = skill_id2group(gid);
	if (group == nullptr || group->src_id != id || group->unit_count <= 0)
		return 0;   // Flaeche weg (Tod, Kartenwechsel, abgelaufen) - kein Tick

	skill_unit* mitte = &group->unit[group->unit_count / 2];
	if (!mitte->alive)
		return 0;
	// Reichweite aus der Flaeche selbst: quadratische Layouts haben (2n+1)^2
	// Units; eine einzelne Unit (Layout 0) traegt ihre Reichweite im YAML-Range.
	int32 reichweite = static_cast<int32>((std::sqrt(static_cast<double>(group->unit_count)) - 1) / 2);
	if (reichweite <= 0)
		reichweite = skill_get_unit_range(group->skill_id, group->skill_lv);
	map_foreachinrange(concerto_schaden_sub, mitte, reichweite, BL_CHAR,
		src, mitte, group.get(), fak, tick);
	return 0;
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
	pc_setreg(sd, add_str(REG_GRUPPE), group->group_id);

	// Beat-Map: ein Timer je Tick, relativ zum Start der Musik.
	t_tick start = gettick();
	for (const s_concerto_beat& b : beats_)
		add_timer(start + b.ms, concerto_tick_timer, src->id,
			static_cast<intptr_t>(group->group_id) * 1024 + b.faktor);

	// Hoerweite = Sichtweite des Clients um den Wirkpunkt.
	int32 n = map_foreachinallarea(concerto_musik_sub, src->m,
		x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, BL_PC,
		wav_, jetzt, dauer_s);

	if (battle_config.skill_log)
		ShowInfo("Concerto %d von %s: Musik an %d Spieler, %lld s\n", getSkillId(), sd->status.name, n, (long long)dauer_s);
}
