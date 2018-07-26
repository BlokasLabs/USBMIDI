/* 
 * Copyright (C) 2015-2018 UAB Vilniaus Blokas
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef MIDI_MESSAGES_H
#define MIDI_MESSAGES_H

#include <stdint.h>

inline bool midi_is_real_time(uint8_t byte)
{
	return byte == 0xf8 || (byte >= 0xfa && byte != 0xfd);
}

inline bool midi_is_sysex_start(uint8_t byte)
{
	return byte == 0xf0;
}

inline bool midi_is_sysex_end(uint8_t byte)
{
	return byte == 0xf7;
}

inline bool midi_is_single_byte_system_common(uint8_t byte)
{
	return byte >= 0xf4 && byte <= 0xf6;
}

#endif // MIDI_MESSAGES_H
