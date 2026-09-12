// SafaRO - Concerto-Skills der Barden/Taenzer (CONCERTO.md)
//
// Quest-freigeschaltete Skills (Etc-Tab), die ein ganzes Musikstueck
// per ZC_SOUND an alle Spieler im Umkreis schicken und eine Bodenflaeche
// legen. Ids ab 8100, eine Stufe. Stand 12.09.2026: Flaeche + Musik +
// Hoersperre; Schaden (Beat-Map, Abschnitt 3.3) kommt im naechsten Schritt.
//
// Warum eine Hoersperre: der 2025er Client kennt keinen Stopp fuer
// ZC_SOUND und mischt jedes Paket als neue Instanz (Test 12.09.). Damit
// niemand zwei Concertos gleichzeitig hoert, merkt sich der Server je
// Spieler, bis wann er zuhoert, und schickt ihm solange kein weiteres
// Musikpaket. Der Kartenwechsel beendet die Musik im Client, deshalb
// gilt die Sperre nur auf der Karte, auf der sie gesetzt wurde.
#pragma once

#include "../skill_impl.hpp"

constexpr uint16 SAFA_CONCERTO_HERO = 8100;   // Concerto: Onslaught of the Hero
constexpr uint16 SAFA_CONCERTO_RUSH = 8101;   // Concerto: Magical Rush

bool concerto_ist_skill(uint16 skill_id);

// Vor dem Cast (skill_check_condition_castbegin): false, wenn das eigene
// Concerto des Barden noch spielt - dann weder SP noch Cooldown.
bool concerto_darf_wirken(map_session_data& sd, uint16 skill_id);

// Aus map_moveblock: laeuft der Barde, zieht seine Concerto-Flaeche mit
// (wie SC_DANCING bei den alten Songs). Fuer alles ausser Spielern
// mit laufendem Concerto ein No-Op.
void concerto_mitziehen(block_list* bl, int16 dx, int16 dy);

class SkillConcerto : public SkillImpl {
public:
	SkillConcerto(uint16 skill_id, const char* wav);

	// TargetType Self: Flaeche um den Barden, wie bei den Originalsongs.
	void castendNoDamageId(block_list* src, block_list* target, uint16 skill_lv, t_tick tick, int32& flag) const override;
	// Falls die skill_db doch einmal auf Ground steht.
	void castendPos2(block_list* src, int32 x, int32 y, uint16 skill_lv, t_tick tick, int32& flag) const override;

private:
	void anstimmen(block_list* src, int32 x, int32 y, uint16 skill_lv) const;

	const char* wav_;   // Dateiname relativ zu data\wav\, max. 23 Zeichen
};
