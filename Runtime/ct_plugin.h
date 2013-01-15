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

#ifdef __cplusplus
extern "C"{
#endif
void global_init();
void thread_init();
void global_deinit();
void thread_deinit();
void read_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid);
void write_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid);
#ifdef __cplusplus
}
#endif
