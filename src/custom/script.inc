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
