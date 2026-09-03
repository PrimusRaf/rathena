// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Atcommands
 * Place the body of custom atcommands in this file.
 * Format:
 *	ACMD_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

// --- SafaRO: @kartenwerte -------------------------------------------
//
//  Zeigt, was von den KARTEN wirklich in den Bonus-Feldern gelandet
//  ist - Groesse, Rasse, Element, jeweils Angriff und Abwehr.
//
//  WARUM ES DIESEN BEFEHL GIBT
//
//  Beim Kartenweiser stand die Frage im Raum, ob eine vervierfachte
//  Karte ihren Bonus ueberhaupt in die Rechnung bringt. Das
//  Statusfenster taugt dafuer NICHT: es wird aus
//  EquipmentPropertiesOrder.lub gefuellt und kennt dort nur
//  Random-Options (Feld rndOpt). Kartenboni schickt rAthena gar nicht
//  an den Client - clif.cpp enthaelt kein einziges Paket dafuer.
//  Wer dort nachsieht, misst also etwas anderes, als er glaubt.
//
//  Hier wird stattdessen genau das ausgelesen, was battle.cpp
//  Zeile 1091 folgende beim Schlag benutzt:
//
//      sd->right_weapon.addsize[groesse]      Angriff nach Groesse
//      sd->indexed_bonus.arrow_addsize[..]    dasselbe fuer Fernkampf
//      sd->right_weapon.addrace[rasse]        Angriff nach Rasse
//      sd->right_weapon.addele[element]       Angriff nach Element
//      sd->indexed_bonus.subele[element]      Abwehr nach Element
//      sd->indexed_bonus.subele_script[..]    dasselbe aus bonus2
//      sd->indexed_bonus.subrace[rasse]       Abwehr nach Rasse
//      sd->indexed_bonus.subsize[groesse]     Abwehr nach Groesse
//
//  Steht hier 360, dann rechnet die Engine auch mit 360.
ACMD_FUNC(kartenwerte)
{
	nullpo_retr(-1, sd);

	char zeile[CHAT_SIZE_MAX];

	clif_displaymessage(fd, "--- Kartenwerte (was die Engine wirklich benutzt) ---");

	// Angriff: Groesse. ELE/RC/SZ-Namen absichtlich ausgeschrieben,
	// damit die Ausgabe ohne Nachschlagen lesbar ist.
	static const char* sz[] = { "Small", "Medium", "Large" };
	for (int32 i = 0; i < SZ_MAX && i < 3; i++) {
		if (sd->right_weapon.addsize[i] || sd->indexed_bonus.arrow_addsize[i]
		    || sd->indexed_bonus.subsize[i]) {
			safesnprintf(zeile, sizeof(zeile),
				"ATK vs %-6s +%d%%  (Pfeil +%d%%)   DEF vs %-6s -%d%%",
				sz[i], sd->right_weapon.addsize[i],
				sd->indexed_bonus.arrow_addsize[i],
				sz[i], sd->indexed_bonus.subsize[i]);
			clif_displaymessage(fd, zeile);
		}
	}
	if (sd->right_weapon.addsize[SZ_ALL] || sd->indexed_bonus.subsize[SZ_ALL]) {
		safesnprintf(zeile, sizeof(zeile),
			"ATK vs alle Groessen +%d%%   DEF vs alle Groessen -%d%%",
			sd->right_weapon.addsize[SZ_ALL],
			sd->indexed_bonus.subsize[SZ_ALL]);
		clif_displaymessage(fd, zeile);
	}

	static const char* ele[] = { "Neutral", "Water", "Earth", "Fire",
		"Wind", "Poison", "Holy", "Shadow", "Ghost", "Undead" };
	for (int32 i = 0; i < ELE_ALL && i < 10; i++) {
		int32 sub = sd->indexed_bonus.subele[i] + sd->indexed_bonus.subele_script[i];
		if (sd->right_weapon.addele[i] || sub) {
			safesnprintf(zeile, sizeof(zeile),
				"ATK vs %-8s +%d%%   Widerstand %-8s %d%%",
				ele[i], sd->right_weapon.addele[i], ele[i], sub);
			clif_displaymessage(fd, zeile);
		}
	}
	if (sd->indexed_bonus.subele[ELE_ALL] || sd->indexed_bonus.subele_script[ELE_ALL]) {
		safesnprintf(zeile, sizeof(zeile), "Widerstand alle Elemente %d%%",
			sd->indexed_bonus.subele[ELE_ALL] + sd->indexed_bonus.subele_script[ELE_ALL]);
		clif_displaymessage(fd, zeile);
	}

	// Player_Human und Player_Doram gehoeren dazu: im PvP sind das
	// die Rassen, die zaehlen (RC_DEMIHUMAN trifft dort NICHT).
	static const char* rc[] = { "Formless", "Undead", "Brute", "Plant",
		"Insect", "Fish", "Demon", "Demihuman", "Angel", "Dragon",
		"Player_Human", "Player_Doram" };
	for (int32 i = 0; i < RC_ALL && i < 12; i++) {
		if (sd->right_weapon.addrace[i] || sd->indexed_bonus.subrace[i]) {
			safesnprintf(zeile, sizeof(zeile),
				"ATK vs %-9s +%d%%   DEF vs %-9s -%d%%",
				rc[i], sd->right_weapon.addrace[i],
				rc[i], sd->indexed_bonus.subrace[i]);
			clif_displaymessage(fd, zeile);
		}
	}
	if (sd->right_weapon.addrace[RC_ALL] || sd->indexed_bonus.subrace[RC_ALL]) {
		safesnprintf(zeile, sizeof(zeile),
			"ATK vs alle Rassen +%d%%   DEF vs alle Rassen -%d%%",
			sd->right_weapon.addrace[RC_ALL],
			sd->indexed_bonus.subrace[RC_ALL]);
		clif_displaymessage(fd, zeile);
	}

	// Der Hinweis gehoert dazu: in Renewal multipliziert die
	// Angriffsseite NUR weaponAtk und equipAtk (battle.cpp:5562),
	// nicht statusAtk. Eine grosse Prozentzahl hier heisst also nicht
	// automatisch viel mehr Schaden.
	clif_displaymessage(fd,
		"Hinweis: ATK-Prozente wirken in Renewal nur auf Waffen- und Ausruestungs-ATK.");
	return 0;
}
