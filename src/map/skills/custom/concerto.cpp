// SafaRO - Concerto-Skills, siehe concerto.hpp und CONCERTO.md
#include "concerto.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <common/core.hpp>      // db_path
#include <common/random.hpp>
#include <common/showmsg.hpp>
#include <common/timer.hpp>

#include "map/battle.hpp"
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

static const uint16 CONCERTO_IDS[] = {
	SAFA_CONCERTO_HERO, SAFA_CONCERTO_RUSH, SAFA_CONCERTO_ELEPHANT,
	SAFA_CONCERTO_EAGLE, SAFA_CONCERTO_RHINO, SAFA_CONCERTO_NIGHT,
	SAFA_TALE_ALBERTA, SAFA_TALE_ALDEBARAN, SAFA_TALE_AMATSU, SAFA_TALE_COMODO,
	SAFA_TALE_EINBROCH, SAFA_TALE_GEFFEN, SAFA_TALE_HUGEL, SAFA_TALE_JUNO,
	SAFA_TALE_LASAGNA, SAFA_TALE_MORROC, SAFA_TALE_PAYON, SAFA_TALE_PRONTERA,
};

// Instanzen der Factory, damit der Tick-Timer Art/Status/Effekt kennt.
static std::unordered_map<uint16, const SkillConcerto*> concerto_instanzen;

const SkillConcerto* concerto_finden(uint16 skill_id) {
	auto it = concerto_instanzen.find(skill_id);
	return it == concerto_instanzen.end() ? nullptr : it->second;
}

bool concerto_ist_skill(uint16 skill_id) {
	for (uint16 id : CONCERTO_IDS)
		if (id == skill_id)
			return true;
	return false;
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
	for (uint16 id : CONCERTO_IDS) {
		if (skill_db.find(id) == nullptr)
			continue;   // Concerto noch nicht in der skill_db - kein Fehlerspam bei jedem Login
		uint16 idx = skill_get_index(id);
		if (idx == 0 || sd.status.skill[idx].id != id || sd.status.skill[idx].lv == 0)
			continue;
		clif_addskill(sd, id);
		clif_skillinfo(sd, id);
	}
}

bool concerto_blockiert_skill(map_session_data& sd, uint16 skill_id) {
	int64 jetzt = static_cast<int64>(time(nullptr));
	if (!concerto_hoert_noch(sd, REG_BIS, REG_MAP, jetzt))
		return false;
	switch (skill_id) {
		case BA_MUSICALSTRIKE:
		case DC_THROWARROW:
			return false;
	}
	if (concerto_ist_skill(skill_id))
		clif_displaymessage(sd.fd, "Your concerto is still playing.");
	else
		clif_displaymessage(sd.fd, "You are conducting a concerto - only Musical Strike and Throw Arrow are possible.");
	clif_skill_fail(sd, skill_id);
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

// Schadensfaktor je Tick ist als Basis x1000 gedacht; 35000 % = 100 % + 34900.
constexpr int32 CONCERTO_RATIO_PLUS = 34900;

// Buff-Laufzeit = Abstand zum naechsten Beat + Gnadenfrist
constexpr int32 CONCERTO_BUFF_GNADE_MS = 3000;
// Debuff (Nightfall): Laufzeit je Anwendung; Wiederziehen verlaengert
constexpr int32 CONCERTO_DEBUFF_MS = 10000;

// Das Tier (STR-Effekt, 2 s je Umlauf) wird nicht auf jedem Beat neu
// ausgeloest - bei 115 bpm liefen sonst vier Mammuts gleichzeitig -,
// sondern hoechstens alle CONCERTO_EFFEKT_MS je Flaeche.
constexpr t_tick CONCERTO_EFFEKT_MS = 1950;
static std::unordered_map<int32, t_tick> concerto_effekt_zuletzt;   // group_id -> tick

static void concerto_effekt(block_list* src, int32 gid, int32 effekt, t_tick tick) {
	if (effekt <= 0)
		return;
	auto it = concerto_effekt_zuletzt.find(gid);
	if (it != concerto_effekt_zuletzt.end() && DIFF_TICK(tick, it->second) < CONCERTO_EFFEKT_MS)
		return;
	concerto_effekt_zuletzt[gid] = tick;
	clif_specialeffect(src, effekt, AREA);
}

// Timer-Daten: group_id * 1024 + wert. Bei Schaden ist wert der Faktor
// (<= 1000, passt in die 12 Bit, die skill_attack als flag weiterreicht);
// bei Buff/Debuff der Abstand zum naechsten Beat in 10-ms-Schritten (<= 1023).
static TIMER_FUNC(concerto_tick_timer);
static TIMER_FUNC(concerto_zeile_timer);

static void concerto_timer_registrieren() {
	static bool registriert = false;
	if (!registriert) {
		add_timer_func_list(concerto_tick_timer, "concerto_tick_timer");
		add_timer_func_list(concerto_zeile_timer, "concerto_zeile_timer");
		registriert = true;
	}
}

SkillConcerto::SkillConcerto(uint16 skill_id, const char* wav, e_concerto_art art, sc_type status, int32 effekt)
	: SkillImpl(static_cast<e_skill>(skill_id)), art_(art), status_(status), effekt_(effekt) {
	concerto_timer_registrieren();
	concerto_instanzen[skill_id] = this;
	fassungen_[0].wav = wav;
	fassung_laden(fassungen_[0], std::to_string(skill_id));
}

SkillConcerto::SkillConcerto(uint16 skill_id, const char* wav_m, const char* wav_f, sc_type status, std::vector<s_tale_klasse> klassen)
	: SkillImpl(static_cast<e_skill>(skill_id)), art_(CONCERTO_TALE), status_(status), effekt_(0), klassen_(std::move(klassen)) {
	concerto_timer_registrieren();
	concerto_instanzen[skill_id] = this;
	fassungen_[0].wav = wav_m;
	fassungen_[1].wav = wav_f;
	fassung_laden(fassungen_[0], std::to_string(skill_id) + "_m");
	fassung_laden(fassungen_[1], std::to_string(skill_id) + "_f");
}

// Tab-getrennte Spalte n (0-basiert) einer Zeile
static std::string spalte(const std::string& zeile, size_t n) {
	size_t a = 0;
	for (size_t i = 0; i < n; ++i) {
		a = zeile.find('\t', a);
		if (a == std::string::npos)
			return "";
		++a;
	}
	size_t e = zeile.find('\t', a);
	return zeile.substr(a, e == std::string::npos ? std::string::npos : e - a);
}

void SkillConcerto::fassung_laden(s_concerto_fassung& f, const std::string& kurz) {
	std::string basis = std::string(db_path) + "/import/concerto/" + kurz;
	std::ifstream in(basis + ".beats");
	if (!in) {
		ShowWarning("Concerto %d: keine Beat-Map %s.beats - Fallback auf Unit.Interval\n", getSkillId(), basis.c_str());
		return;
	}
	std::string zeile;
	while (std::getline(in, zeile)) {
		if (zeile.empty())
			continue;
		if (zeile[0] == '#') {
			size_t d = zeile.find("dauer_ms=");
			if (d != std::string::npos)
				f.dauer_ms = atoi(zeile.c_str() + d + 9);
			continue;
		}
		std::istringstream z(zeile);
		int32 ms = 0; double fak_d = 0;
		if (!(z >> ms >> fak_d))
			continue;
		int32 fak = static_cast<int32>(std::lround(fak_d * 1000));
		if (ms < 0 || fak <= 0)
			continue;
		f.beats.push_back({ ms, std::min(fak, 1000) });
	}
	if (art_ != CONCERTO_TALE) {
		ShowStatus("Concerto %d: %zu Ticks aus %s.beats\n", getSkillId(), f.beats.size(), basis.c_str());
		return;
	}
	// Liedzeilen: ms<TAB>original<TAB>english<TAB>ascii - nur Zeilen mit Blase
	std::ifstream lin(basis + ".lyrics");
	if (lin) {
		while (std::getline(lin, zeile)) {
			if (zeile.empty() || zeile[0] == '#')
				continue;
			if (!zeile.empty() && zeile.back() == '\r')
				zeile.pop_back();
			std::string blase = spalte(zeile, 2), chat = spalte(zeile, 3);
			int32 ms = atoi(spalte(zeile, 0).c_str());
			if (blase.empty() || ms < 0)
				continue;
			f.zeilen.push_back({ ms, blase, chat });
		}
	} else {
		ShowWarning("Concerto %d: keine Liedzeilen %s.lyrics\n", getSkillId(), basis.c_str());
	}
	ShowStatus("Concerto %d: %zu Beats, %zu Liedzeilen, %d ms aus %s\n", getSkillId(), f.beats.size(), f.zeilen.size(), f.dauer_ms, basis.c_str());
}

const s_concerto_fassung& SkillConcerto::fassung(const map_session_data& sd) const {
	if (art_ == CONCERTO_TALE && sd.status.sex == SEX_FEMALE && !fassungen_[1].beats.empty())
		return fassungen_[1];
	return fassungen_[0];
}

bool SkillConcerto::klasse_passt(const map_session_data& sd) const {
	for (const s_tale_klasse& k : klassen_)
		if ((sd.class_ & k.maske) == k.wert)
			return true;
	return false;
}

// Soul Link nach Klasse; status.cpp prueft bei SC_SPIRIT selbst noch einmal
// gegen MAPID_SECONDMASK, deshalb reicht hier die Vorauswahl.
uint16 concerto_soullink_fuer(const map_session_data& sd) {
	switch (sd.class_ & MAPID_SECONDMASK) {
		case MAPID_KNIGHT:         return SL_KNIGHT;
		case MAPID_CRUSADER:       return SL_CRUSADER;
		case MAPID_WIZARD:         return SL_WIZARD;
		case MAPID_SAGE:           return SL_SAGE;
		case MAPID_HUNTER:         return SL_HUNTER;
		case MAPID_BARDDANCER:     return SL_BARDDANCER;
		case MAPID_PRIEST:         return SL_PRIEST;
		case MAPID_MONK:           return SL_MONK;
		case MAPID_BLACKSMITH:     return SL_BLACKSMITH;
		case MAPID_ALCHEMIST:      return SL_ALCHEMIST;
		case MAPID_ASSASSIN:       return SL_ASSASIN;
		case MAPID_ROGUE:          return SL_ROGUE;
		case MAPID_STAR_GLADIATOR: return SL_STAR;
		case MAPID_SOUL_LINKER:    return SL_SOULLINKER;
		case MAPID_SUPER_NOVICE:   return SL_SUPERNOVICE;
	}
	// Rebirth-Erstklassen unter Level 70: Spirit of the Rebirth
	if ((sd.class_ & JOBL_UPPER) && !(sd.class_ & JOBL_2) && sd.status.base_level < 70 && (sd.class_ & MAPID_FIRSTMASK) != MAPID_NOVICE)
		return SL_HIGH;
	return 0;
}

void SkillConcerto::calculateSkillRatio(const Damage* wd, const block_list* src, const block_list* target, uint16 skill_lv, int32& base_skillratio, int32 mflag) const {
	int32 fak = (mflag > 0 && mflag <= 1000) ? mflag : 1000;   // Unit-Tick-Fallback = voll
	base_skillratio = (base_skillratio + CONCERTO_RATIO_PLUS) * fak / 1000;
}

// ---------------------------------------------------------------- Schaden

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

// ---------------------------------------------------------------- Buff

// Ein Spieler in der Flaeche: Gruppenmitglied (oder der Barde selbst) bekommt den Status.
static int32 concerto_buff_sub(block_list* bl, va_list ap) {
	block_list* src = va_arg(ap, block_list*);
	int32 status = va_arg(ap, int32);
	int32 dauer = va_arg(ap, int32);

	map_session_data* tsd = BL_CAST(BL_PC, bl);
	map_session_data* ssd = BL_CAST(BL_PC, src);
	if (tsd == nullptr || ssd == nullptr || bl->prev == nullptr || status_isdead(*bl))
		return 0;
	if (bl->id != src->id && (ssd->status.party_id == 0 || tsd->status.party_id != ssd->status.party_id))
		return 0;
	// Neu setzen verlaengert; val1 = 1 (eine Stufe)
	status_change_start(src, bl, static_cast<sc_type>(status), 10000, 1, 0, 0, 0, dauer, SCSTART_NOAVOID | SCSTART_NOTICKDEF);
	return 1;
}

// ---------------------------------------------------------------- Debuff (Nightfall)

struct s_nacht_debuff {
	sc_type sc;
	int32 gewicht;     // relative Haeufigkeit
	int32 val1;        // meist Skillstufe
	bool nur_spieler;  // Strip & Co. wirken nur auf Spieler
};

// Leichte Gewichtung (Raffael 12.09.): Stat-Brecher und DoTs haeufig, harte
// Kontrollen selten, Stone/Freeze am seltensten (Elementwechsel des Ziels).
static const s_nacht_debuff NACHT_POOL[] = {
	{ SC_POISON,        6, 5, false }, { SC_CURSE,         6, 5, false }, { SC_SILENCE,       6, 5, false },
	{ SC_BLIND,         6, 5, false }, { SC_BLEEDING,      6, 5, false }, { SC_CONFUSION,     5, 5, false },
	{ SC_BURNING,       5, 5, false }, { SC_FREEZING,      4, 5, false }, { SC_HALLUCINATION, 4, 5, false },
	{ SC_FEAR,          3, 5, false }, { SC_CRYSTALIZE,    3, 5, false }, { SC_DEEPSLEEP,     2, 5, false },
	{ SC_STUN,          2, 5, false }, { SC_SLEEP,         2, 5, false }, { SC_FREEZE,        1, 5, false },
	{ SC_STONE,         1, 5, false },
	{ SC_DECREASEAGI,   6, 10, false }, { SC_QUAGMIRE,      6, 5, false }, { SC_SLOWCAST,      6, 5, false },
	{ SC_SIGNUMCRUCIS,  6, 10, false }, { SC_PROVOKE,       6, 10, false }, { SC_ORATIO,        6, 10, false },
	{ SC_AETERNA,       4, 1, false }, { SC_MARSHOFABYSS,  5, 5, false }, { SC_MANDRAGORA,    5, 5, false },
	{ SC_ADORAMUS,      5, 10, false }, { SC_THORNSTRAP,    3, 5, false }, { SC_CLOUD_KILL,    5, 5, false },
	{ SC__IGNORANCE,    5, 3, false }, { SC__WEAKNESS,     5, 3, false }, { SC__ENERVATION,   5, 3, false },
	{ SC__GROOMY,       5, 3, false }, { SC__LAZINESS,     5, 3, false }, { SC__UNLUCKY,      5, 3, false },
	{ SC_STRIPWEAPON,   4, 5, true },  { SC_STRIPSHIELD,   4, 5, true },  { SC_STRIPARMOR,    4, 5, true },
	{ SC_STRIPHELM,     4, 5, true },  { SC_ANKLE,         3, 5, true },  { SC_SPIDERWEB,     3, 1, true },
};

static const s_nacht_debuff* nacht_ziehen(bool spieler) {
	int32 summe = 0;
	for (const s_nacht_debuff& d : NACHT_POOL)
		if (spieler || !d.nur_spieler)
			summe += d.gewicht;
	int32 los = rnd() % summe;
	for (const s_nacht_debuff& d : NACHT_POOL) {
		if (!spieler && d.nur_spieler)
			continue;
		if (los < d.gewicht)
			return &d;
		los -= d.gewicht;
	}
	return &NACHT_POOL[0];
}

static int32 concerto_debuff_sub(block_list* bl, va_list ap) {
	block_list* src = va_arg(ap, block_list*);
	block_list* mitte = va_arg(ap, block_list*);

	if (bl->prev == nullptr || status_isdead(*bl))
		return 0;
	if (battle_check_target(mitte, bl, BCT_ENEMY) <= 0)
		return 0;
	const s_nacht_debuff* d = nacht_ziehen(bl->type == BL_PC);
	// NOAVOID: Bossimmunitaet, Resistenzen und VIT/INT-Kuerzung gelten nicht (Raffael 12.09.)
	status_change_start(src, bl, d->sc, 10000, d->val1, 0, 0, 0, CONCERTO_DEBUFF_MS, SCSTART_NOAVOID | SCSTART_NOTICKDEF);
	return 1;
}

// ---------------------------------------------------------------- Tale (30-min-Buff + Soul Link)

// Jeder Spieler in der Flaeche: Klassenlinie der Region -> Regionsstatus 30 min;
// jeder mit passendem Soul Link -> Link 30 min. Kein Party-Zwang.
static int32 concerto_tale_sub(block_list* bl, va_list ap) {
	block_list* src = va_arg(ap, block_list*);
	const SkillConcerto* con = va_arg(ap, const SkillConcerto*);

	map_session_data* tsd = BL_CAST(BL_PC, bl);
	if (tsd == nullptr || bl->prev == nullptr || status_isdead(*bl))
		return 0;
	if (battle_check_target(src, bl, BCT_NOENEMY) <= 0)
		return 0;
	// Der Saenger selbst bekommt den Regionsbuff immer, egal welche Klasse (Raffael 13.09.)
	if (con->status() != SC_NONE && (bl->id == src->id || con->klasse_passt(*tsd)))
		status_change_start(src, bl, con->status(), 10000, 1, 0, 0, 0, SAFA_TALE_BUFF_MS, SCSTART_NOAVOID | SCSTART_NOTICKDEF);
	uint16 link = concerto_soullink_fuer(*tsd);
	if (link != 0)
		sc_start2(src, bl, SC_SPIRIT, 100, 5, link, SAFA_TALE_BUFF_MS);
	return 1;
}

// Liedzeile: Sprechblase (Englisch) wie beim Frost Joker, dazu die
// Originalzeile in ASCII-Umschrift als gefaerbte Chatzeile.
static TIMER_FUNC(concerto_zeile_timer) {
	map_session_data* sd = map_id2sd(id);
	int32 gid = static_cast<int32>(data / 1024);
	size_t idx = static_cast<size_t>(data % 1024);
	if (sd == nullptr || sd->prev == nullptr || status_isdead(*sd)) {
		ShowDebug("Concerto-Zeile %zu: Spieler %d weg\n", idx, id);
		return 0;
	}
	std::shared_ptr<s_skill_unit_group> group = skill_id2group(gid);
	if (group == nullptr || group->src_id != id || group->unit_count <= 0) {
		ShowDebug("Concerto-Zeile %zu: Flaeche gid=%d %s\n", idx, gid, group == nullptr ? "weg" : "fremd/leer");
		return 0;   // Concerto vorbei (Tod, Kartenwechsel) - kein Gesang mehr
	}
	const SkillConcerto* con = concerto_finden(group->skill_id);
	if (con == nullptr)
		return 0;
	const s_concerto_fassung& f = con->fassung(*sd);
	if (idx >= f.zeilen.size()) {
		ShowDebug("Concerto-Zeile %zu: ausserhalb (%zu Zeilen)\n", idx, f.zeilen.size());
		return 0;
	}
	if (battle_config.skill_log)
		ShowInfo("Concerto-Zeile %zu @%d ms: %s\n", idx, f.zeilen[idx].ms, f.zeilen[idx].blase.c_str());
	const s_concerto_zeile& z = f.zeilen[idx];
	char buf[CHAT_SIZE_MAX];
	snprintf(buf, sizeof(buf), "%s : %s", sd->status.name, z.blase.c_str());
	clif_disp_overhead(sd, buf);
	if (!z.chat.empty()) {
		snprintf(buf, sizeof(buf), "~ %s", z.chat.c_str());
		clif_messagecolor(sd, color_table[COLOR_LIGHT_GREEN], buf, false, AREA);
	}
	return 0;
}

// ---------------------------------------------------------------- Tick

static TIMER_FUNC(concerto_tick_timer) {
	block_list* src = map_id2bl(id);
	if (src == nullptr)
		return 0;
	int32 gid = static_cast<int32>(data / 1024);
	int32 wert = static_cast<int32>(data % 1024);
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

	const SkillConcerto* con = concerto_finden(group->skill_id);
	e_concerto_art art = con != nullptr ? con->art() : CONCERTO_SCHADEN;

	switch (art) {
		case CONCERTO_SCHADEN:
			map_foreachinrange(concerto_schaden_sub, mitte, reichweite, BL_CHAR,
				src, mitte, group.get(), wert, tick);
			break;
		case CONCERTO_BUFF: {
			int32 dauer = wert * 10 + CONCERTO_BUFF_GNADE_MS;
			concerto_effekt(src, gid, con->effekt(), tick);
			map_foreachinrange(concerto_buff_sub, mitte, reichweite, BL_PC,
				src, static_cast<int32>(con->status()), dauer);
			break;
		}
		case CONCERTO_DEBUFF:
			concerto_effekt(src, gid, con->effekt(), tick);
			map_foreachinrange(concerto_debuff_sub, mitte, reichweite, BL_CHAR,
				src, mitte);
			break;
		case CONCERTO_TALE:
			map_foreachinrange(concerto_tale_sub, mitte, reichweite, BL_PC, src, con);
			break;
	}
	return 0;
}

// ---------------------------------------------------------------- Wirken

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
	// Sanctuary-Kachel (Elephant, Rhino, Tales): skill_unit_timer_sub loescht die
	// Flaeche, sobald val1 (= Rest-Heilungen des echten Sanctuary) <= 0 ist. Bei
	// uns wird val1 nie gesetzt - Flaeche flackerte nur auf (13.09.2026).
	if (group->unit_id == UNT_SANCTUARY)
		group->val1 = 1000000;

	const s_concerto_fassung& f = fassung(*sd);
	int64 jetzt = static_cast<int64>(time(nullptr));
	int64 dauer_s = skill_get_time(getSkillId(), skill_lv) / 1000;   // Duration1 = Liedlaenge
	if (f.dauer_ms > 0) {
		// Tales: Barden- und Taenzerinnen-Fassung sind verschieden lang -
		// die Flaeche lebt genau so lang wie das gewaehlte Stueck.
		dauer_s = f.dauer_ms / 1000;
		group->limit = f.dauer_ms;
	}

	pc_setreg(sd, add_str(REG_BIS), jetzt + dauer_s);
	pc_setreg(sd, add_str(REG_MAP), src->m);
	pc_setreg(sd, add_str(REG_GRUPPE), group->group_id);

	// Beat-Map: ein Timer je Tick, relativ zum Start der Musik.
	t_tick start = gettick();
	for (size_t i = 0; i < f.beats.size(); ++i) {
		const s_concerto_beat& b = f.beats[i];
		int32 wert;
		if (art_ == CONCERTO_SCHADEN) {
			wert = b.faktor;
		} else {
			// Abstand zum naechsten Beat (letzter: bis zum Liedende) in 10 ms, max. 10,23 s
			int32 naechster = (i + 1 < f.beats.size()) ? f.beats[i + 1].ms : static_cast<int32>(dauer_s * 1000);
			wert = std::min(1023, std::max(0, (naechster - b.ms) / 10));
		}
		add_timer(start + b.ms, concerto_tick_timer, src->id,
			static_cast<intptr_t>(group->group_id) * 1024 + wert);
	}
	// Liedzeilen (Tales): ein Timer je Zeile mit Blase
	for (size_t i = 0; i < f.zeilen.size() && i < 1024; ++i)
		add_timer(start + f.zeilen[i].ms, concerto_zeile_timer, src->id,
			static_cast<intptr_t>(group->group_id) * 1024 + static_cast<intptr_t>(i));

	// Hoerweite = Sichtweite des Clients um den Wirkpunkt.
	int32 n = map_foreachinallarea(concerto_musik_sub, src->m,
		x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, BL_PC,
		f.wav.c_str(), jetzt, dauer_s);

	ShowInfo("Concerto %d von %s: Musik an %d Spieler, %lld s, Flaeche gid=%d unit_id=%d units=%d, %zu Beat-Timer, %zu Zeilen-Timer (%s)\n",
		getSkillId(), sd->status.name, n, (long long)dauer_s, group->group_id, group->unit_id, group->unit_count,
		f.beats.size(), f.zeilen.size(), f.wav.c_str());
}
