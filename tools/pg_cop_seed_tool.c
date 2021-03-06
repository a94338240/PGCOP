/*
    PGCOP - PocoGraph Component Oriented Platform.
    Copyright (C) 2013  David Wu <david@pocograph.com>
                        Steve Ma <steve@pocograph.com>

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

#include "pg_cop_debug.h"
#include "pg_cop_seed_file_parser.h"
#include "pg_cop_util.h"
#include "pg_cop_seed_tool_opts_ag.h"

#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

enum _state_txt {
	en_ready,
	en_div,
	en_note,
	en_note_mult,
};

int g_pg_cop_debug_level = 0;
const char* g_output_folder = NULL;
char* g_d_name = NULL;

/*
 *┌───────────────────────────────────────────┐
 *│            Seed info                      │
 *├───────┬───────────────────────────────────┤
 *│ Hash  │ cd6c5e902a7e251fc12c09d6844fedf7  │
 *├───────┼───────────────────────────────────┤
 *│ Modu  │ mod_service                       │
 *├───────┼─────┬─────────────────────────────┤
 *│       │Type │ Address                     │
 *│ Track ├─────┼─────────────────────────────┤
 *│       │ 0   │ 10.1.3.117:1080             │
 *│       │ 0   │ 10.1.1.1:1090               │
 *├───────┼─────┴─────────────────────────────┤
 *│ Func  │ ping                              |
 *│       │ get_four_number                   │
 *┕───────┴───────────────────────────────────┘
 *
 */
static int _debug_seed(const char* path)
{
	char* hash;
	struct pg_cop_seed_file_tracker_info_list track_h;
	struct pg_cop_seed_file_func_info_list func_h;
	char* module_name = 0;
	char fmtbuf[32] = {0};
	int tmpl = 0;
	int i = 0;

	struct pg_cop_seed_file_tracker_info_list* it = 0;
	struct pg_cop_seed_file_func_info_list* itl = 0;

	if (0 != pg_cop_seed_file_parser_all_info(path, &module_name, &hash, &track_h, &func_h)) {
		DEBUG_ERROR("Cannot parse seed info from %s.", path);
		return -1;
	}
	printf("┌───────────────────────────────────────────┐\n");
	printf("│               Seed info                   │\n");
	printf("├───────┬───────────────────────────────────┤\n");
	printf("│ Hash  │ %s  │\n", hash);
	printf("├───────┼───────────────────────────────────┤\n");
	printf("│ Modu  │ %s", module_name);
	tmpl = strlen(module_name);
	if (tmpl <= 34) {
		for (i = 0; i < 34 - tmpl; ++i) printf(" ");
		printf("│");
	}
	printf("\n");


	printf("├───────┼─────┬─────────────────────────────┤\n");
	printf("│       │Type │      Address                │\n");
	printf("│ Track ├─────┼─────────────────────────────┤\n");

	list_for_each_entry(it, &track_h.list_head, list_head) {
		memset(fmtbuf, 0, sizeof(fmtbuf));
		sprintf(fmtbuf, "%s:%d", inet_ntoa(it->info.address), it->info.port);
		tmpl = strlen(fmtbuf);
		printf("│       │ %d   │ %s", it->info.tracker_type, fmtbuf);
		for (i = 0; i < 28 - tmpl; ++i) printf(" ");
		printf("│\n");
	}
	printf("├───────┼─────┴─────────────────────────────┤\n");


	list_for_each_entry(itl, &func_h.list_head, list_head) {
		tmpl = strlen(itl->name);
		static int flg = 0;
		if (0 == flg) {
			flg = 1;
			printf("│ Func  │ ");
		} else {
			printf("│       │ ");
		}
		printf("%s", itl->name);
		tmpl = strlen(itl->name);
		if (tmpl <= 34) {
			for (i = 0; i < 34 - tmpl; ++i) printf(" ");
			printf("│");
		}
		printf("\n");
	}
	printf("┕───────┴───────────────────────────────────┘\n");

	if (0 != pg_cop_seed_file_parser_release_buf(&module_name, &hash, &track_h, &func_h)) {
		DEBUG_ERROR("release buff error");
		return -1;
	}
	return 0;
}

//specially only match one sub string
static int _get_sub_str(char* s, char* pattern, char** pout, int* end_offset)
{
	regmatch_t m[8];
	regex_t reg;
	int r = 0;

	memset(m, 0, sizeof(m));
	if (0 != regcomp(&reg, pattern, REG_EXTENDED | REG_NEWLINE)) {
		DEBUG_ERROR("Error occured when initialize...");
		return -1;
	}

	r = regexec(&reg, s, sizeof(m) / sizeof(regmatch_t), m, 0);
	if (REG_NOMATCH == r) {
		return -1;
	} else if (0 == r) {
		int slen = m[1].rm_eo - m[1].rm_so;
		*end_offset = m[1].rm_eo;
		if (pout != 0) {
			*pout = (char*)malloc(slen + 1);
			memset(*pout, 0, slen + 1);
			strncpy(*pout, s + m[1].rm_so, slen);
		}
	}
	regfree(&reg);
	return 0;
}

static int _match_parttern(char* s)
{
	char* pattern = "@MODULE:\\s+(\\w+)\\s+";
	char* p_trck = 0;
	char* md_name = 0;
	char* dummy = s;
	int endoff = 0;
	char* tmp = 0;
	struct pg_cop_seed_file_tracker_info_list t0;
	struct pg_cop_seed_file_tracker_info_list* new_track = 0;
	struct pg_cop_seed_file_func_info_list f0;
	unsigned int iarr[5] = {0};
	char* osf = 0;

	if (0 != _get_sub_str(s, pattern, &md_name, &endoff)) return -1;
	DEBUG_LOG_BY_LEVEL(1, "module name=%s\n", md_name);
	INIT_LIST_HEAD(&t0.list_head);
	INIT_LIST_HEAD(&f0.list_head);

	pattern = "([0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}:[0-9]{1,5})\\s+";
	while (0 == _get_sub_str(dummy, pattern, &p_trck, &endoff)) {
	  DEBUG_LOG_BY_LEVEL(1, "Tracker=%s\n", p_trck);
		new_track = (struct pg_cop_seed_file_tracker_info_list*)malloc(sizeof(struct pg_cop_seed_file_tracker_info_list));
		memset(new_track, 0, sizeof(struct pg_cop_seed_file_tracker_info_list));
		new_track->info.tracker_type = 0;
		memset(iarr, 0, sizeof(iarr));
		sscanf(p_trck, "%d.%d.%d.%d:%d", &iarr[0], &iarr[1], &iarr[2], &iarr[3], &iarr[4]);
		char addr_array[32] = {0};
		sprintf(addr_array, "%d.%d.%d.%d", iarr[0], iarr[1], iarr[2], iarr[3]);
		inet_aton(addr_array, &new_track->info.address);
		new_track->info.port = iarr[4];
		list_add_tail(&new_track->list_head, &t0.list_head);
		dummy += endoff;
	}

	pattern = "(@METHODS:)";
	if (0 != _get_sub_str(s, pattern, 0, &endoff)) return -1;
	dummy = s + endoff;
	DEBUG_LOG_BY_LEVEL(1, "Functions list:\n");
	while ((tmp = strsep(&dummy, " ")) != NULL) {
		if (*tmp != '\0') {
			struct pg_cop_seed_file_func_info_list* new_f = (struct pg_cop_seed_file_func_info_list*)malloc(sizeof(struct pg_cop_seed_file_func_info_list));
			new_f->name = tmp;
			list_add_tail(&new_f->list_head, &f0.list_head);
			DEBUG_LOG_BY_LEVEL(1, "%s\t", tmp);
		}
	}
	DEBUG_LOG_BY_LEVEL(1, "\n");
	int l = strlen(g_output_folder) + strlen(g_d_name) + 5;
	osf = (char*)malloc(l);
	memset(osf, 0, l);
	sprintf(osf, "%s%s%s", g_output_folder, "/", g_d_name);
	sprintf(osf + l - 5, "%s", "seed");
	DEBUG_LOG_BY_LEVEL(1, "Output:%s\n", osf);
	if (0 != pg_cop_seed_file_create(md_name, osf, &t0, &f0)) {
		DEBUG_ERROR("Error occured when create seed file.");
		free(osf);
		return -1;
	} else {
		printf("Seed file:\"%s\" generated successfully.\n", osf);
	}
	free(osf);
	return 0;
}


static int _g_single_file(char* filename)
{
	FILE* f = fopen(filename, "r");
	char c = 0;
	enum _state_txt st = en_ready;
	int idx = 0;
	char buff[2048] = {0};

	if (NULL == f) {
		DEBUG_ERROR("File %s cannot be open.", filename);
		return -1;
	}

	while (!feof(f)) {
		if (fread(&c, 1, 1, f)) {
			switch (st) {
			case en_ready:
				if ('/' == c) st = en_div;
				break;
			case en_div:
				if ('*' == c) st = en_note;
				break;
			case en_note:
				if ('*' == c) st = en_note_mult;
				else {
					if (('\n' == c) || ('\t' == c)) c = ' ';
					if (idx < sizeof(buff) - 1) buff[idx++] = c;
					else {
						DEBUG_INFO("Buffer overflow!");
						memset(buff, 0, sizeof(buff));
						idx = 0;
					}
				}
				break;
			case en_note_mult:
				if ('/' == c)  {
					if (0 == _match_parttern(buff)) {
						return 0;
					}
					idx = 0;
					memset(buff, 0, sizeof(buff));
					st = en_ready;
				} else {
					if ('\t' == c) c = ' ';
					buff[idx++] = c;
					st = en_note;
				}
				break;
			default:
				break;
			}
		}
	}

	return -1;
}

static int _generate(const char* path)
{
	struct dirent *module_dir_entry;
	char module_path[255] = {};

	DIR *module_dir = opendir(path);
	if (!module_dir) {
		DEBUG_ERROR("Cannot found module directory.");
		goto check_modules_dir;
	}

	while ((module_dir_entry = readdir(module_dir))) {
		if (!module_dir_entry->d_name) {
			DEBUG_ERROR("Cannot open eth input directory.");
			continue;
		}

		char file_ext[20] = {};
		g_d_name = module_dir_entry->d_name;
		if (pg_cop_get_file_extension(module_dir_entry->d_name,
		                              file_ext, sizeof(file_ext)) != 0) {
			DEBUG_INFO("Error occured when loading module %s", module_dir_entry->d_name);
			goto get_file_extension_cont;
		}

		if (0 != strncmp(file_ext, ".c", 3))
			goto skip;

		strncpy(module_path, path, sizeof(module_path) - 1);
		strncat(module_path, "/", sizeof(module_path) - 1);
		strncat(module_path, module_dir_entry->d_name, sizeof(module_path) - 1);
		DEBUG_LOG_BY_LEVEL(1, "\nFile=%s, in progress...\n", module_path);
		if (0 != _g_single_file(module_path)) {
			DEBUG_ERROR("Error occured in \'%s\', invalid seed declaration comments found.", module_path);
		}
		continue;

get_file_extension_cont:
skip:
		continue;
	}

	closedir(module_dir);
	return 0;

check_modules_dir:
	return -1;
}

int main(int argc, char** argv)
{
	int optct = optionProcess(&toolsOptions, argc, argv);
	argc -= optct;
	argv += optct;

	if (!HAVE_OPT(INPUT) && !HAVE_OPT(VIEW)) {
		DEBUG_ERROR("You need to specify an input, or view in option.");
		goto check_opts;
	}

	const char *path = NULL;
	if (HAVE_OPT(VIEW)) {
		path = OPT_ARG(VIEW);
		if (!path) {
			DEBUG_ERROR("You need to specify a valid path for input.");
			goto opt_get_input;
		}

		_debug_seed(path);
		goto view_seed;
	}

	if (HAVE_OPT(INPUT)) {
		path = OPT_ARG(INPUT);
		if (!path) {
			DEBUG_ERROR("You need to specify a valid path for input.");
			goto opt_get_input;
		}
	}

  if (HAVE_OPT(DEBUG_LEVEL)) {
    g_pg_cop_debug_level = atoi(OPT_ARG(DEBUG_LEVEL));
    DEBUG_LOG_BY_LEVEL(1, "debug level=%d\n", g_pg_cop_debug_level);
    if (!path) {
      DEBUG_ERROR("You need to specify a valid path for input.");
      goto opt_get_input;
    }
  }

	if (HAVE_OPT(OUTPUT)) {
		g_output_folder = OPT_ARG(OUTPUT);
	} else {
	  printf("no given output dir.using default as \".\"\n");
		g_output_folder = ".";
	}

	_generate(path);

view_seed:
	return 0;

opt_get_input:
check_opts:
	return -1;
}
