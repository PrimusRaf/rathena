// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom battle config structure
 * Place the definition of custom battle configuration structure members in this file.
 *
 * Make sure that the name you want is not already taken by looking into battle.hpp
 * Do not forget to add initilization logic to battle_config_init.inc
 *
 * Format:
 *	 <datatype> name;
 **/

// --- SafaRO ---------------------------------------------------
// Darf der Client das Selbstfuettern des Tieres umschalten?
// Begruendung in conf/import/battle_conf.txt und in clif.cpp an der
// Stelle CONFIG_PET_AUTOFEED.
int32 pet_autofeed_client_toggle;

// Hoechste Schadensminderung der Abwehrkarten, in Prozent. Begruendung
// steht bei CAP_DEFENSE_CARDFIX in src/map/battle.cpp; eingestellt wird
// sie in conf/import/battle_conf.txt.
int32 max_damage_reduction;
