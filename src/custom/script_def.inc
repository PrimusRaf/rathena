// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the definition of custom script commands in this file.
 * Format:
 *	BUILDIN_DEF(command_name,"parameters"),
 **/

//BUILDIN_DEF(example,""),

// --- SafaRO: Selbstfuettern der Tiere, siehe src/custom/script.inc ---
//
// "i?" - Schalter, dahinter wahlweise eine char_id.
// "?"  - nur wahlweise eine char_id.
//
// Die Rumpffunktionen stehen in script.inc; script.cpp bindet beide
// Dateien ein (Zeile 27878 und 28657).
BUILDIN_DEF(setpetautofeed,"i?"),
BUILDIN_DEF(getpetautofeed,"?"),
BUILDIN_DEF(petautofeedallowed,"?"),
