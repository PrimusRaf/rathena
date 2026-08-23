// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom battle config initializations
 * Place the definition of custom battle configuration initializations in this file.
 *
 * Do not forget to add a comma at the end of the line, if you want to add another config
 *
 * Format:
 *	 { "name", &battle_config.<variable name>, <default value>, <minimum value>, <maximum value> },
 **/

// --- SafaRO ---------------------------------------------------
// Vorgabe 1: rAthenas gewohntes Verhalten. Abgeschaltet wird es in
// conf/import/battle_conf.txt, nicht hier - dann steht der Grund an
// der Stelle, an der jemand ihn sucht.
{ "pet_autofeed_client_toggle", &battle_config.pet_autofeed_client_toggle, 1, 0, 1, },
