/*
    PGCOP - PocoGraph Component Oriented Platform.
    Copyright (C) 2013  David Wu <david@pocograph.com>

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

#include "pg_cop_seeds.h"
#include "pg_cop_debug.h"
#include "pg_cop_util.h"

#include <string.h>
#include <dirent.h>

pg_cop_seed_t *pg_cop_seeds_list = NULL;

char *pg_cop_seeds_path = NULL;

int pg_cop_init_seeds_table()
{
  pg_cop_seeds_list = 
    (pg_cop_seed_t *)malloc(sizeof(pg_cop_seed_t)); // INFO No free needed
  if (!pg_cop_seeds_list)
    return -1;
  INIT_LIST_HEAD(&pg_cop_seeds_list->list_head);
  return 0;
}

int pg_cop_load_seeds(int argc, char *argv[])
{
  DIR *seed_dir;
  struct dirent *seed_dir_entry;
  char file_ext[20] = {};
  char seed_path[255] = {};
  pg_cop_seed_t *seed = NULL;
  int invres;
  FILE *fp;

  if (!pg_cop_seeds_list)
    return -1;

  seed_dir = opendir(pg_cop_seeds_path);

  if (!seed_dir)
    DEBUG_CRITICAL("Cannot found seed directory.");

  while ((seed_dir_entry = readdir(seed_dir))) {
    if (!seed_dir_entry->d_name)
      continue;
    if (pg_cop_get_file_extension(seed_dir_entry->d_name, 
                                  file_ext, sizeof(file_ext)) != 0)
      continue;
    if (strncmp(file_ext, ".seed", 5))
      continue;

    strncpy(seed_path, pg_cop_seeds_path, sizeof(seed_path) - 1);
    strncat(seed_path, "/", sizeof(seed_path) - 1);
    strncat(seed_path, seed_dir_entry->d_name, sizeof(seed_path) - 1);
    fp = fopen(seed_path, "r");

    if (!fp) {
      DEBUG_ERROR("Seed %s cannot be loaded.", 
                  seed_dir_entry->d_name);
      continue;
    }

    // TODO fill structure of seeds.
    seed = pg_cop_seed_new("202cb962ac59075b964b07152d234b70",
                           "127.0.0.1",
                           12728);

    seed->tracker_intf = pg_cop_module_interface_announce("mod_pgcop_tracker", 
                                                    MODULE_INTERFACE_TYPE_SOCKET_TCP,
                                                    seed->host, seed->port);
    if (seed->tracker_intf == NULL)
      goto out;

    seed->seed_intf = pg_cop_module_interface_connect("mod_pgcop_tracker");
    if (seed->seed_intf == NULL)
      goto out;

    if (pg_cop_module_interface_invoke(seed->seed_intf, "announce_seed", 1,
                                       VSTACK_TYPE_STRING, seed->infohash))
      goto out;
    if (pg_cop_module_interface_pop(seed->seed_intf, VSTACK_TYPE_U8, &invres))
      goto out;

    list_add_tail(&seed->list_head, &pg_cop_seeds_list->list_head);

    DEBUG_INFO("Seed %s be loaded.",
               seed_dir_entry->d_name);
    fclose(fp);

    continue;
  out:
    pg_cop_seed_destroy(seed);
    fclose(fp);
  }

  closedir(seed_dir);
  return 0;
}

pg_cop_seed_t *pg_cop_seed_new(char *infohash,
                               char *host,
                               int port)
{
  pg_cop_seed_t *seed = malloc(sizeof(pg_cop_seed_t));
  seed->infohash = strdup(infohash);
  seed->host = strdup(host);
  seed->port = port;

  return seed;
}

int pg_cop_seed_destroy(pg_cop_seed_t *seed)
{
  free(seed->infohash);
  seed->infohash = NULL;
  free(seed->host);
  seed->host = NULL;
  free(seed);

  return 0;
}