/****************************************************************************
 *   Aug 11 17:13:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "alarm_clock_config.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#endif

alarm_properties_t::alarm_properties_t() : BaseJsonConfig( ALARM_CLOCK_JSON_CONFIG_FILE ) {}

bool alarm_properties_t::onSave(JsonDocument& doc) {
    doc[VERSION_KEY] = 1;
    doc[BEEP_KEY] = beep;
    doc[FADE_KEY] = fade;
    doc[VIBE_KEY] = vibe;
    doc[SHOW_ON_MAIN_TILE_KEY] = show_on_main_tile;

    return( true );
}

bool alarm_properties_t::onLoad(JsonDocument& doc) {
    beep = doc[BEEP_KEY] | true;
    fade = doc[FADE_KEY] | true;
    vibe = doc[VIBE_KEY] | true;
    show_on_main_tile = doc[SHOW_ON_MAIN_TILE_KEY] | false;

    return( true );
}

bool alarm_properties_t::onDefault( void ) {
    beep = true;
    fade = true;
    vibe = true;
    show_on_main_tile = false;

    return( true );
}