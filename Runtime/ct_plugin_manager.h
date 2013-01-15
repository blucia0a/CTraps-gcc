/*
==CTraps -- A GCC Plugin to instrument shared memory accesses==
 
    Copyright (C) 2012 Brandon Lucia

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
*/
void (*plugin_thd_init)(void);
void (*plugin_thd_deinit)(void);
void (*plugin_init)(void);
void (*plugin_deinit)(void);

void (*plugin_read_trap)(void *, void *, unsigned long, void *,unsigned long);
void (*plugin_write_trap)(void *, void *, unsigned long, void *,unsigned long);
