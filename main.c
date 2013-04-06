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
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#include "shlcdc_mmap.h"

typedef enum {
  MIBIB_PARTITION    = 2,
  APPSBL_PARTITION   = 6,
  BOOT_PARTITION     = 8,
  RECOVERY_PARTITION = 9,
  SYSTEM_PARTITION   = 11,
} partition;

typedef struct{
  partition number;
  const char *name;
} partition_info;

static partition_info partitions[] = {
  { BOOT_PARTITION,     "boot"     },
  { RECOVERY_PARTITION, "recovery" },
  { SYSTEM_PARTITION,   "system"   }
};

static size_t partitions_length = sizeof(partitions) / sizeof(partitions[0]);

enum {
  MMC_PROTECT_READ  = 0x01,
  MMC_PROTECT_WRITE = 0x02
};

struct mmc_protect_inf {
  uint32_t partition;
  uint32_t protect;
};

static struct mmc_protect_inf *mmc_protect_part;

static struct mmc_protect_inf original_mmc_protect_part[] = {
  { 0,                  MMC_PROTECT_WRITE                    },
  { 1,                  MMC_PROTECT_READ | MMC_PROTECT_WRITE },
  { MIBIB_PARTITION,    MMC_PROTECT_READ | MMC_PROTECT_WRITE },
  { 3,                  MMC_PROTECT_READ | MMC_PROTECT_WRITE },
  { 4,                  MMC_PROTECT_WRITE                    },
  { APPSBL_PARTITION,   MMC_PROTECT_READ | MMC_PROTECT_WRITE },
  { 7,                  MMC_PROTECT_READ | MMC_PROTECT_WRITE },
  { BOOT_PARTITION,     MMC_PROTECT_WRITE                    },
  { RECOVERY_PARTITION, MMC_PROTECT_WRITE                    },
  { 10,                 MMC_PROTECT_READ | MMC_PROTECT_WRITE },
  { SYSTEM_PARTITION,   MMC_PROTECT_WRITE                    }
};

static size_t original_mmc_protect_part_size =
  sizeof(original_mmc_protect_part) / sizeof(original_mmc_protect_part[0]);

static size_t original_mmc_protect_part_length =
  sizeof(original_mmc_protect_part) / sizeof(original_mmc_protect_part[0])
    * sizeof(struct mmc_protect_inf);

static void *
find_mmc_protect_part(const void *address, int length)
{
  int position;
  uint32_t mmc_protect_part_index;

  for (position = 0; position < length; position++) {
    for (mmc_protect_part_index = 0;
         mmc_protect_part_index < original_mmc_protect_part_size;
         mmc_protect_part_index++) {
      uint32_t *partition = (uint32_t *)((unsigned long)(address) + position);
      if (partition[mmc_protect_part_index * 2] != original_mmc_protect_part[mmc_protect_part_index].partition) {
        break;
      }
    }
    if (mmc_protect_part_index == original_mmc_protect_part_size) {
      return (void*)((unsigned long)(address) + position);
    }
  }

  return NULL;
}

static bool
unlock_protection(int32_t mmc_protect_part_index)
{
  int fd;
  void *address = NULL;
  void *mmc_protect_part_address;
  int page_size = sysconf(_SC_PAGE_SIZE);
  int length = page_size * page_size;
  int index;

  fd = shlcdc_mmap_device_open();
  if (fd < 0) {
    return false;
  }

  address = shlcdc_mmap(NULL, length, fd);
  if (address == MAP_FAILED) {
    close(fd);
    return false;
  }

  mmc_protect_part_address = find_mmc_protect_part(address, length);
  if (!mmc_protect_part_address) {
    printf("Couldn't find mmc_part_protect address\n");
    shlcdc_munmap(address, length);
    close(fd);
    return false;
  }

#ifdef DEBUG
  printf("Found mmc_part_protect at %p\n",
         (void*)((unsigned long)(mmc_protect_part_address) - (unsigned long)(address) + PAGE_OFFSET));
#endif

  mmc_protect_part = mmc_protect_part_address;
  for (index = 0; index < original_mmc_protect_part_size; index++) {
    mmc_protect_part[index].protect = 0;
  }

  shlcdc_munmap(address, length);
  close(fd);

  return true;
}

static bool
restore_protection(void)
{
  memcpy(mmc_protect_part, &original_mmc_protect_part, original_mmc_protect_part_length);
  return true;
}

static void
usage(void)
{
  printf("Usage:\n");
  printf("\tnand_unlock [partition name]\n");
}

static partition
get_partition_number_for_name(const char *name)
{
  uint32_t i;
  for (i = 0; i < partitions_length; i++) {
    if (!strcmp(name, partitions[i].name)) {
      return partitions[i].number;
    }
  }
  return -1;
}

static uint32_t
get_mmc_protect_part_index_for_name(const char *name)
{
  int32_t partition_number;
  uint32_t mmc_protect_part_index;

  partition_number = get_partition_number_for_name(name);
  if (partition_number < 0) {
    return UINT32_MAX;
  }

  for (mmc_protect_part_index = 0;
       mmc_protect_part_index < original_mmc_protect_part_size;
       mmc_protect_part_index++) {
    if (original_mmc_protect_part[mmc_protect_part_index].partition == (uint32_t)partition_number) {
      return mmc_protect_part_index;
    }
  }

  printf("partition number should be ");
  for (mmc_protect_part_index = 0;
       mmc_protect_part_index < original_mmc_protect_part_size;
       mmc_protect_part_index++) {
    printf("%u ", original_mmc_protect_part[mmc_protect_part_index].partition);
  }
  printf(".\n");

  return UINT32_MAX;
}

int
main(int argc, char **argv)
{
  bool ret;
  uint32_t mmc_protect_part_index;

  if (argc != 2) {
    usage();
    exit(EXIT_FAILURE);
  }

  if (strcmp("boot", argv[1]) &&
      strcmp("recovery", argv[1]) &&
      strcmp("system", argv[1])) {
    printf("The %s partition is not supported.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  mmc_protect_part_index = get_mmc_protect_part_index_for_name(argv[1]);
  if (mmc_protect_part_index == UINT32_MAX) {
    exit(EXIT_FAILURE);
  }

  ret = unlock_protection(mmc_protect_part_index);

  if (!ret) {
    exit(EXIT_FAILURE);
  }

  printf("Now %s partition is unlocked.\n", argv[1]);

  exit(EXIT_SUCCESS);
}
/*
vi:ts=2:nowrap:ai:expandtab:sw=2
*/
