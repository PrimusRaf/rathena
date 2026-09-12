// SafaRO - Concerto-Skills der Barden/Taenzer (CONCERTO.md)
//
// Quest-freigeschaltete Skills (Etc-Tab), die ein ganzes Musikstueck
// per ZC_SOUND an alle Spieler im Umkreis schicken und eine Bodenflaeche
// legen. Ids ab 8100, eine Stufe. Stand 12.09.2026:
//   8100 Onslaught of the Hero   Schaden (Waffe)      Beat-Map-Ticks
//   8101 Magical Rush            Schaden (Magie)      Beat-Map-Ticks
//   8102 Elephant Hymn           Buff SC_SAFA_ELEPHANT je Beat
//   8103 Eagles Whisper          Buff SC_SAFA_EAGLE    je Beat
//   8104 Great Rhino Trial       Buff SC_SAFA_RHINO    je Beat
//   8105 Nightfall in Midgard    zufaelliger Debuff je Beat, Bosse inklusive
//
// Warum eine Hoersperre: der 2025er Client kennt keinen Stopp fuer
// ZC_SOUND und mischt jedes Paket als neue Instanz (Test 12.09.). Damit
// niemand zwei Concertos gleichzeitig hoert, merkt sich der Server je
// Spieler, bis wann er zuhoert, und schickt ihm solange kein weiteres
// Musikpaket. Der Kartenwechsel beendet die Musik im Client, deshalb
// gilt die Sperre nur auf der Karte, auf der sie gesetzt wurde.
//
// Buff-Modell (Raffael 12.09.): auf jedem Beat der Beat-Map bekommen
// alle Gruppenmitglieder in der Flaeche den Status neu, Laufzeit =
// Abstand zum naechsten Beat + 3 s Gnadenfrist. Wer reinlaeuft, hat ihn
// beim naechsten Beat; wer rausgeht, verliert ihn kurz danach; stirbt
// der Barde oder wechselt die Karte, laeuft er aus. Dazu kreist auf
// jedem Beat das Tier des Concertos um den Barden (eigener STR-Effekt,
// tools/concerto/str_bauen.py).
#pragma once

#include <vector>

#include "../skill_impl.hpp"
#include "map/status.hpp"   // sc_type, SC_NONE

constexpr uint16 SAFA_CONCERTO_HERO = 8100;      // Concerto: Onslaught of the Hero
constexpr uint16 SAFA_CONCERTO_RUSH = 8101;      // Concerto: Magical Rush
constexpr uint16 SAFA_CONCERTO_ELEPHANT = 8102;  // Concerto: Elephant Hymn
constexpr uint16 SAFA_CONCERTO_EAGLE = 8103;     // Concerto: Eagles Whisper
constexpr uint16 SAFA_CONCERTO_RHINO = 8104;     // Concerto: Great Rhino Trial
constexpr uint16 SAFA_CONCERTO_NIGHT = 8105;     // Concerto: Nightfall in Midgard

// Client-Effekte (STR-Slots, siehe CONCERTO.md 4): Mammut, Falke, Fulgor, Nightmare
constexpr int32 SAFA_EFFEKT_ELEPHANT = 705;      // mobile_ef01.str
constexpr int32 SAFA_EFFEKT_EAGLE = 987;         // rwc2011.str
constexpr int32 SAFA_EFFEKT_RHINO = 1031;        // invincibleoff2.str
constexpr int32 SAFA_EFFEKT_NIGHT = 669;         // wideb.str

bool concerto_ist_skill(uint16 skill_id);

// Vor dem Cast (skill_check_condition_castbegin): false, wenn das eigene
// Concerto des Barden noch spielt - dann weder SP noch Cooldown.
bool concerto_darf_wirken(map_session_data& sd, uint16 skill_id);

// Ganz am Anfang von skill_check_condition_castbegin: true = Skill ist
// gesperrt, weil das eigene Concerto laeuft. Erlaubt bleiben nur Musical
// Strike und Throw Arrow (Raffael 12.09.: "gebunden wie bei einem Song").
bool concerto_blockiert_skill(map_session_data& sd, uint16 skill_id);

// Aus map_moveblock: laeuft der Barde, zieht seine Concerto-Flaeche mit
// (wie SC_DANCING bei den alten Songs). Fuer alles ausser Spielern
// mit laufendem Concerto ein No-Op.
void concerto_mitziehen(block_list* bl, int16 dx, int16 dy);

// Nach dem Login (clif_parse_LoadEndAck): gelernte Concertos einzeln per
// ZC_ADD_SKILL nachschicken. Der 2025er Client zeigt sie aus der
// Skill-Liste (ZC_SKILLINFO_LIST) nicht an, aus ZC_ADD_SKILL schon
// (12.09.2026: Server hatte den Skill, Etc-Tab blieb leer).
void concerto_nachschicken(map_session_data& sd);

// Ein Tick aus der Beat-Map (db/import/concerto/<id>.beats)
struct s_concerto_beat {
	int32 ms;        // Offset ab Wiedergabestart
	int32 faktor;    // Schadensfaktor x1000 (1000 = volle 35000 %); bei Buffs unbenutzt
};

enum e_concerto_art : uint8 {
	CONCERTO_SCHADEN,   // skill_attack je Tick
	CONCERTO_BUFF,      // sc_start auf die Gruppe je Beat
	CONCERTO_DEBUFF,    // zufaelliger Status auf Gegner je Beat
};

class SkillConcerto : public SkillImpl {
public:
	// art/status/effekt: siehe Tabelle oben; status nur bei CONCERTO_BUFF
	SkillConcerto(uint16 skill_id, const char* wav, e_concerto_art art = CONCERTO_SCHADEN, sc_type status = SC_NONE, int32 effekt = 0);

	// TargetType Self: Flaeche um den Barden, wie bei den Originalsongs.
	void castendNoDamageId(block_list* src, block_list* target, uint16 skill_lv, t_tick tick, int32& flag) const override;
	// Falls die skill_db doch einmal auf Ground steht.
	void castendPos2(block_list* src, int32 x, int32 y, uint16 skill_lv, t_tick tick, int32& flag) const override;

	// 35000 % x Tick-Faktor (der Faktor kommt als mflag aus dem Timer).
	void calculateSkillRatio(const Damage* wd, const block_list* src, const block_list* target, uint16 skill_lv, int32& base_skillratio, int32 mflag) const override;

	e_concerto_art art() const { return art_; }
	sc_type status() const { return status_; }
	int32 effekt() const { return effekt_; }

private:
	void anstimmen(block_list* src, int32 x, int32 y, uint16 skill_lv) const;

	void beats_laden();

	const char* wav_;                      // Dateiname relativ zu data\wav\, max. 23 Zeichen
	std::vector<s_concerto_beat> beats_;   // leer = Fallback auf Unit.Interval
	e_concerto_art art_;
	sc_type status_;
	int32 effekt_;
};

// Die Factory-Instanz eines Concertos (fuer den Tick-Timer), nullptr wenn keins.
const SkillConcerto* concerto_finden(uint16 skill_id);
