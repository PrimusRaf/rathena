// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the body of custom script commands in this file.
 * Format:
 *	BUILDIN_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

// ==================================================================
//  SafaRO - Selbstfuettern ohne das Haekchen im Tierfenster
// ==================================================================
//
//  WOZU
//
//  Serverseitig ist das Selbstfuettern eingeschaltet und jedes
//  zaehmbare Tier darf es (db/import/pet_db.yml, AllowAutoFeed). Der
//  Schalter dafuer liegt aber allein im Client: er sendet 0x2d8 mit
//  CONFIG_PET_AUTOFEED, und clif_parse_configuration (clif.cpp:17740)
//  setzt daraufhin pd->pet.autofeed.
//
//  Unser Client - 2025-07-16 Ragexe, WARP-gebaut - zeichnet dieses
//  Haekchen nicht. Nachgeprueft: der Server SENDET den Zustand
//  (clif.cpp:8386, Paket 0x2d9, angemeldet seit PACKETVER 20070227 in
//  clif_packetdb.hpp:920), sowohl beim Schluepfen als auch bei jedem
//  Kartenwechsel. Es kommt nur nichts sichtbar an.
//
//  Damit ist das Feld ohne Client-Oberflaeche unerreichbar: es gibt
//  keinen Skriptbefehl und keinen GM-Befehl, der pet.autofeed
//  anfasst - geprueft in script.cpp und atcommand.cpp. Deshalb diese
//  beiden hier.
//
//  WARUM IN src/custom
//
//  Das ist die von rAthena vorgesehene Stelle (script.cpp:27878 und
//  28657 binden die beiden .inc ein). Aenderungen an script.cpp
//  selbst wuerden bei jedem Upstream-Abgleich kollidieren; hier nicht.
//
//  WAS GESPEICHERT WIRD
//
//  pet.autofeed steht in struct s_pet (mmo.hpp:491) und in der Spalte
//  `autofeed` der Tabelle `pet` (sql-files/main.sql:683). Es ueberlebt
//  also Ab- und Anmelden, sobald das Tier gespeichert wird.
// ------------------------------------------------------------------

/**
 * Schaltet das Selbstfuettern des mitgefuehrten Tieres.
 *
 *	setpetautofeed <0 oder 1>{,<char_id>};
 *
 * Rueckgabe: 1 wenn gesetzt, sonst 0.
 *
 * Die drei Bedingungen sind dieselben, die clif_parse_configuration
 * prueft (clif.cpp:17741) - ein Skript soll nicht mehr duerfen als
 * der Client.
 */
BUILDIN_FUNC(setpetautofeed)
{
	map_session_data *sd;

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->pd == nullptr) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!battle_config.feature_pet_autofeed) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (!sd->pd->get_pet_db()->allow_autofeed) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	sd->pd->pet.autofeed = script_getnum(st, 2) != 0;

	// Sofort wegschreiben, nicht erst beim naechsten Speicheranlass.
	// pet.autofeed steht in struct s_pet und geht mit dem ganzen
	// Datensatz an den Char-Server (intif.cpp:147). Ohne diese Zeile
	// haengt es davon ab, ob der Charakter vor dem Abmelden noch
	// gespeichert wird - und der Schalter waere mal dauerhaft, mal
	// nicht.
	intif_save_petdata(sd->status.account_id, &sd->pd->pet);

	// Den Client trotzdem unterrichten. Zeichnet er das Haekchen
	// nicht, schadet es nicht; zeichnet er es doch, stimmt es.
	clif_pet_autofeed_status(sd, true);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Liest den Zustand.
 *
 *	getpetautofeed({<char_id>});
 *
 * Rueckgabe: 1 an, 0 aus oder kein Tier dabei.
 */
BUILDIN_FUNC(getpetautofeed)
{
	map_session_data *sd;

	if (!script_charid2sd(2, sd) || sd->pd == nullptr) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, sd->pd->pet.autofeed ? 1 : 0);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Darf dieses Tier ueberhaupt selbst fressen?
 *
 *	petautofeedallowed({<char_id>});
 *
 * Das ist AllowAutoFeed aus der Tierdatenbank. Ohne diese Auskunft
 * koennte ein Skript nicht zwischen "aus" und "geht nicht"
 * unterscheiden - und der Spieler bekaeme eine irrefuehrende Meldung.
 */
BUILDIN_FUNC(petautofeedallowed)
{
	map_session_data *sd;

	if (!script_charid2sd(2, sd) || sd->pd == nullptr) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushint(st, sd->pd->get_pet_db()->allow_autofeed ? 1 : 0);
	return SCRIPT_CMD_SUCCESS;
}


// ==================================================================
//  SafaRO - Sonne, Mond und Stern einzeln zuruecksetzen
// ==================================================================
//
//  WOZU
//
//  rAthena kennt resetfeel, aber das loescht alle drei auf einmal
//  (pc_resetfeel, pc.cpp:9550 - eine Schleife ueber MAX_PC_FEELHATE).
//
//  Fuer den Spieler ist das teuer. Eine Zuweisung neu zu setzen heisst:
//  auf die gewuenschte Karte laufen und SG_FEEL wirken, 100 SP und drei
//  Sekunden feste Zauberzeit. Wer nur seinen Stern verschieben will,
//  duerfte nicht Sonne und Mond mit verlieren.
//
//  WIE DAS ZURUECKSETZEN WIRKT
//
//  skills/taekwon/feelingthesunmoonandstars.cpp:17 entscheidet allein
//  an feel_map[stufe-1].index:
//
//      if(!sd->feel_map[skill_lv-1].index)
//          clif_feel_req(...)          <- neue Karte waehlen
//      else
//          clif_feel_info(...)         <- nur die alte anzeigen
//
//  Auf 0 gesetzt fragt die Fertigkeit also wieder nach einem Ort. Die
//  drei Felder werden ausserdem als dauerhafte Charaktervariablen
//  gefuehrt (PC_FEEL_SUN, PC_FEEL_MOON, PC_FEEL_STAR, siehe sg_info in
//  pc.cpp:718) - die muessen mit, sonst steht beim naechsten Anmelden
//  der alte Wert wieder da (pc.cpp:2396 liest sie zurueck).

/**
 * Setzt EINE Zuweisung zurueck.
 *
 *	resetfeelone <0 Sonne, 1 Mond, 2 Stern>{,<char_id>};
 *
 * Rueckgabe: 1 wenn etwas geloescht wurde, 0 wenn dort nichts stand.
 */
BUILDIN_FUNC(resetfeelone)
{
	map_session_data *sd;
	int32 i = script_getnum(st, 2);

	if (i < 0 || i >= MAX_PC_FEELHATE) {
		ShowError("buildin_resetfeelone: index %d out of range (0-%d).\n",
			i, MAX_PC_FEELHATE - 1);
		script_pushint(st, 0);
		return SCRIPT_CMD_FAILURE;
	}

	if (!script_charid2sd(3, sd)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	if (sd->feel_map[i].index == 0) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	sd->feel_map[i].m = -1;
	sd->feel_map[i].index = 0;
	pc_setglobalreg(sd, add_str(sg_info[i].feel_var), 0);

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}

/**
 * Liefert den Kartennamen einer Zuweisung.
 *
 *	getfeelmap(<0 Sonne, 1 Mond, 2 Stern>{,<char_id>});
 *
 * Leerer Text, wenn dort nichts steht. Damit kann der NPC zeigen, was
 * gerade gilt - sonst loescht man blind.
 *
 * Gelesen wird der Kartenindex, nicht feel_map[i].m: der Index steht
 * auch dann richtig da, wenn die Karte gerade nicht geladen ist.
 */
BUILDIN_FUNC(getfeelmap)
{
	map_session_data *sd;
	int32 i = script_getnum(st, 2);

	if (i < 0 || i >= MAX_PC_FEELHATE) {
		ShowError("buildin_getfeelmap: index %d out of range (0-%d).\n",
			i, MAX_PC_FEELHATE - 1);
		script_pushconststr(st, "");
		return SCRIPT_CMD_FAILURE;
	}

	if (!script_charid2sd(3, sd) || sd->feel_map[i].index == 0) {
		script_pushconststr(st, "");
		return SCRIPT_CMD_SUCCESS;
	}

	script_pushstrcopy(st, mapindex_id2name(sd->feel_map[i].index));
	return SCRIPT_CMD_SUCCESS;
}
