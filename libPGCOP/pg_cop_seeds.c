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
#include "pg_cop_config.h"

#include <string.h>
#include <dirent.h>

pg_cop_seed_t *pg_cop_seeds_list = NULL;

char *pg_cop_seeds_path = NULL;

int pg_cop_init_seeds_table()
{
	pg_cop_seeds_list =
	    (pg_cop_seed_t *)malloc(sizeof(pg_cop_seed_t));         // INFO No free needed
	if (!pg_cop_seeds_list)
		goto alloc_seeds_list;
	INIT_LIST_HEAD(&pg_cop_seeds_list->list_head);
	return 0;

alloc_seeds_list:
	return -1;
}

int pg_cop_load_seeds(int argc, char *argv[])
{
	if (!pg_cop_seeds_list)
		goto check_seeds_list;

	DIR *seed_dir = opendir(pg_cop_seeds_path);

	if (!seed_dir) {
		goto seed_dir_open;
		DEBUG_CRITICAL("Cannot found seed directory.");
	}

	struct dirent *seed_dir_entry;
	while ((seed_dir_entry = readdir(seed_dir))) {
		if (!seed_dir_entry->d_name)
			goto read_dir_cont;

		char file_ext[20] = {0};
		if (pg_cop_get_file_extension(seed_dir_entry->d_name,
		                              file_ext, sizeof(file_ext)) != 0)
			goto get_file_ext_cont;

		if (strncmp(file_ext, ".seed", 5))
			goto cmp_ext_cont;

		char seed_path[255] = {0};
		strncpy(seed_path, pg_cop_seeds_path, sizeof(seed_path) - 1);
		strncat(seed_path, "/", sizeof(seed_path) - 1);
		strncat(seed_path, seed_dir_entry->d_name, sizeof(seed_path) - 1);

		FILE *fp = fopen(seed_path, "r");
		if (!fp) {
			DEBUG_ERROR("Seed %s cannot be loaded.",
			            seed_dir_entry->d_name);
			goto seed_open_cont;
		}

		// TODO fill structure of seeds.
		pg_cop_tracker_info_t *tracker_info_head = malloc(sizeof(pg_cop_tracker_info_t));
		if (!tracker_info_head)
			goto tracker_info_head_cont;
		INIT_LIST_HEAD(&tracker_info_head->list_head);

		pg_cop_tracker_info_t *tracker_info = pg_cop_tracker_info_new("127.0.0.1", 12728);
		if (!tracker_info)
			goto alloc_tracker_info;
		INIT_LIST_HEAD(&tracker_info->list_head);

		list_add_tail(&tracker_info->list_head,
		              &tracker_info_head->list_head);

		pg_cop_seed_t *seed = pg_cop_seed_new("202cb962ac59075b964b07152d234b70",
		                                      "mod_tester_service", tracker_info_head);
		if (!seed)
			goto new_seed_cont;

		if (pg_cop_seed_connect_tracker(seed))
			goto connect_tracker_cont;

		pg_cop_module_t * module;
		list_for_each_entry(module, &pg_cop_modules_list->list_head, list_head) {
			DEBUG_INFO("Loaded module=%s, seed=%s", module->info->name, seed->mod_name);
			if (strcmp(module->info->name, seed->mod_name) == 0) {
				DEBUG_INFO("Announce %s", seed->mod_name);
				if (pg_cop_seed_announce(seed)) {
					DEBUG_ERROR("%s cannot be announced to tracker.", module->info->name);
					goto seed_announce;
				}
				DEBUG_INFO("Service in seed %s available on this computer.", seed_dir_entry->d_name);
			}
		}
		if (pg_cop_seed_get_announced_peers(seed)) {
			DEBUG_ERROR("Cannot get peers");
			goto get_announced_peers;
		}

		INIT_LIST_HEAD(&seed->list_head);
		list_add_tail(&seed->list_head, &pg_cop_seeds_list->list_head);

		DEBUG_INFO("Seed %s be loaded.", seed_dir_entry->d_name);
		fclose(fp);
		continue;

get_announced_peers:
		pg_cop_seed_revoke(seed);
seed_announce:
		pg_cop_seed_disconnect_tracker(seed);
connect_tracker_cont:
		pg_cop_seed_destroy(seed);
new_seed_cont:
alloc_tracker_info:
		pg_cop_tracker_info_list_destroy(tracker_info_head);
tracker_info_head_cont:
		fclose(fp);
seed_open_cont:
cmp_ext_cont:
get_file_ext_cont:
read_dir_cont:
		continue;
	}

	closedir(seed_dir);
	return 0;

seed_dir_open:
check_seeds_list:
	return -1;
}

pg_cop_seed_t *pg_cop_seed_new(char *infohash,
                               char *mod_name,
                               pg_cop_tracker_info_t *tracker_info_list)
{
	pg_cop_seed_t *seed = malloc(sizeof(pg_cop_seed_t));
	seed->infohash = strdup(infohash);
	seed->mod_name = strdup(mod_name);
	seed->tracker_info_list = tracker_info_list;
	seed->intf = NULL;

	return seed;
}

int pg_cop_seed_destroy(pg_cop_seed_t *seed)
{
	free(seed->infohash);
	seed->infohash = NULL;
	free(seed->mod_name);
	seed->mod_name = NULL;
	seed->intf = NULL;
	free(seed);

	return 0;
}

pg_cop_tracker_info_t *pg_cop_tracker_info_new(char *host, int port)
{
	pg_cop_tracker_info_t *tracker = malloc(sizeof(pg_cop_tracker_info_t));
	if (!tracker)
		goto alloc_tracker;
	tracker->intf = NULL;
	tracker->host = strdup(host);
	tracker->port = port;
	INIT_LIST_HEAD(&tracker->list_head);

	return tracker;

alloc_tracker:
	return NULL;
}

int pg_cop_tracker_info_destroy(pg_cop_tracker_info_t *tracker)
{
	free(tracker->host);
	return 0;
}

int pg_cop_tracker_info_list_destroy(pg_cop_tracker_info_t *tracker_list)
{
	pg_cop_tracker_info_t *tracker_info;
	pg_cop_tracker_info_t *tracker_info_tmp;
	list_for_each_entry_safe(tracker_info, tracker_info_tmp,
	                         &tracker_list->list_head,
	                         list_head) {
		pg_cop_tracker_info_destroy(tracker_info);
	}
	free(tracker_list);

	return 0;
}

int pg_cop_seed_connect_tracker(pg_cop_seed_t *seed)
{
	pg_cop_tracker_info_t *tracker;
	list_for_each_entry(tracker,
	                    &seed->tracker_info_list->list_head,
	                    list_head) {
		tracker->intf = pg_cop_module_interface_announce("mod_pgcop_tracker",
		                MODULE_INTERFACE_TYPE_SOCKET_TCP,
		                tracker->host,
		                tracker->port);
		if (tracker->intf == NULL)
			goto tracker_announce_cont;

		seed->intf = pg_cop_module_interface_connect("mod_pgcop_tracker");
		if (seed->intf == NULL)
			goto seed_connect_cont;

		continue;
seed_connect_cont:
		pg_cop_module_interface_revoke(tracker->intf);
		tracker->intf = NULL;
tracker_announce_cont:
		;
	}
	return 0;
}

int pg_cop_seed_disconnect_tracker(pg_cop_seed_t *seed)
{
	if (seed->intf) {
		pg_cop_module_interface_disconnect(seed->intf);
		seed->intf = NULL;
	}

	return 0;
}

int pg_cop_seed_announce(pg_cop_seed_t *seed)
{
	if (pg_cop_module_interface_invoke(seed->intf, "announce_seed", 2,
	                                   VSTACK_TYPE_STRING, seed->infohash,
	                                   VSTACK_TYPE_I32, tracker_incoming_port))
		goto announce_invoke_cont;

	int invres;
	if (pg_cop_module_interface_pop(seed->intf, VSTACK_TYPE_I32, &invres))
		goto announce_return_cont;

	if (invres)
		goto remote_return;
	return 0;

remote_return:
announce_return_cont:
announce_invoke_cont:
	return -1;
}

int pg_cop_seed_revoke(pg_cop_seed_t *seed)
{
	if (pg_cop_module_interface_invoke(seed->intf, "revoke_seed", 2,
	                                   VSTACK_TYPE_STRING, seed->infohash,
	                                   VSTACK_TYPE_I32, tracker_incoming_port))
		goto announce_invoke_cont;

	int invres;
	if (pg_cop_module_interface_pop(seed->intf, VSTACK_TYPE_I32, &invres))
		goto announce_return_cont;

	return 0;

announce_return_cont:
announce_invoke_cont:
	return -1;
}

int pg_cop_seed_get_announced_peers(pg_cop_seed_t *seed)
{
	if (pg_cop_module_interface_invoke(seed->intf, "get_announced_peers", 1,
	                                   VSTACK_TYPE_STRING, seed->infohash))
		goto get_peers_invoke;

	int found = 0;
	int invres = 0;
	if (pg_cop_module_interface_pop(seed->intf, VSTACK_TYPE_I32, &invres))
		goto pop_invres;
	if (invres)
		goto invres_check;

	while (pg_cop_module_interface_has_more(seed->intf)) {
		char *peer_host;
		int peer_port;
		if (pg_cop_module_interface_pop(seed->intf, VSTACK_TYPE_STRING, &peer_host))
			goto get_peers_return_host_cont;
		if (pg_cop_module_interface_pop(seed->intf, VSTACK_TYPE_I32, &peer_port))
			goto get_peers_return_port_cont;

		// TODO announce remote peers.
		DEBUG_INFO("INFOHASH=%s, peer=%s:%d", seed->infohash, peer_host, peer_port);
		free(peer_host);

		continue;
get_peers_return_port_cont:
		free(peer_host);
get_peers_return_host_cont:
		;
	}

	if (!found)
		goto find_peers;

	return 0;

find_peers:
invres_check:
pop_invres:
get_peers_invoke:
	return -1;
}
