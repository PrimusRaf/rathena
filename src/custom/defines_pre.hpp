// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef CONFIG_CUSTOM_DEFINES_PRE_HPP
#define CONFIG_CUSTOM_DEFINES_PRE_HPP

/**
 * rAthena configuration file (http://rathena.org)
 * For detailed guidance on these check http://rathena.org/wiki/SRC/config/
 **/

// SafaRO: Client-Paketversion fest im Quelltext. Muss zu docker/.env
// (PACKETVER) passen. Grund: Am 09.09.2026 wurde ./configure ohne
// --enable-packetver aufgerufen, der Server lief mit rAthenas
// Standardwert, und alle Spieler erschienen als Novice. Mit diesem
// Fallback ist ein vergessenes configure-Flag folgenlos.
#ifndef PACKETVER
	#define PACKETVER 20250716
#endif

#endif /* CONFIG_CUSTOM_DEFINES_PRE_HPP */
