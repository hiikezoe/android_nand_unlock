/*
 * Copyright (C) 2013 Hiroyuki Ikezoe
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef SHLCDC_MMAP_H
#define SHLCDC_MMAP_H

#include <stdbool.h>

int shlcdc_mmap_device_open(void);
void *shlcdc_mmap(void *address, size_t length, int fd);
bool shlcdc_munmap(void *address, size_t length);

#endif /* SHLCDC_MMAP_H */
/*
vi:ts=2:nowrap:ai:expandtab:sw=2
*/
