/*	TimeLib - Time management library for embedded devices
        Copyright (C) 2014 Jesus Ruben Santa Anna Zamudio.

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        Author website: http://www.geekfactory.mx
        Author e-mail: ruben at geekfactory dot mx
 */
#ifndef TIMELIBPORT_H
#define TIMELIBPORT_H

#include "modules.h"

#define TICKS_PER_SECOND m_tick_clk
#define TICK_SECOND ((uint64_t)TICKS_PER_SECOND)
#define TICK_MINUTE ((uint64_t)TICKS_PER_SECOND * 60ull)
#define TICK_HOUR ((uint64_t)TICKS_PER_SECOND * 3600ull)
#define tick_get() ((uint64_t)m_tick())

#endif
// End of header file
