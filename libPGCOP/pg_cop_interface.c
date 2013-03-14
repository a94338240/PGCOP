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

#include "pg_cop_interface.h"
#include "pg_cop_debug.h"
#include "pg_cop_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

int tracker_incoming_port = 0;
pg_cop_module_interface_announcement_t *announced_modules = NULL;

static int _interface_request_send(pg_cop_module_interface_t *intf,
                                   int call_type)
{
	int header_line;
	int retval;

	header_line = call_type;
	retval = send(intf->peer->connection_id, &header_line,
	              sizeof(int), 0);
	if (retval <= 0)
		goto send;

	header_line = strlen(intf->mod_name) + 1;
	retval = send(intf->peer->connection_id, &header_line,
	              sizeof(int), 0);
	if (retval <= 0)
		goto send;
	retval = send(intf->peer->connection_id, intf->mod_name,
	              header_line, 0);
	if (retval <= 0)
		goto send;

	header_line = strlen(intf->peer->mod_name) + 1;
	retval = send(intf->peer->connection_id, &header_line,
	              sizeof(int), 0);
	if (retval <= 0)
		goto send;
	retval = send(intf->peer->connection_id, intf->peer->mod_name,
	              header_line, 0);
	if (retval <= 0)
		goto send;

	header_line = pg_cop_vstack_used_bytes(intf->vstack);
	retval = send(intf->peer->connection_id, &header_line,
	              sizeof(int), 0);
	if (retval <= 0)
		goto send;

	void *stack_dump = pg_cop_vstack_dump(intf->vstack);
	if (!stack_dump)
		goto stack_dump;
	retval = send(intf->peer->connection_id, stack_dump,
	              header_line, 0);
	if (retval <= 0)
		goto send_stack;

	if (pg_cop_vstack_clear(intf->vstack))
		goto clear_vstack;

	free(stack_dump);
	return 0;

clear_vstack:
send_stack:
	free(stack_dump);
stack_dump:
send:
	return -1;
}

static int _interface_request_recv(int sockfd,
                                   int *call_type,
                                   pg_cop_vstack_t **vstack,
                                   char **mod_name,
                                   char **mod_name_from)
{
	int retval;
	int header_line;

retry:
	retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
	if (retval <= 0)
		goto recv_magic;
	if (header_line != MAGIC_START_INVOKE && header_line != MAGIC_START_RETURN)
		goto retry;
	*call_type = header_line;
	DEBUG_INFO("Tracker: Start receiving. type=%d", *call_type);

	retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
	if (retval <= 0)
		goto recv_mnf_size;

	*mod_name_from = malloc(header_line);
	retval = recv(sockfd, *mod_name_from, header_line, MSG_WAITALL);
	if (retval <= 0)
		goto recv_mnf;
	DEBUG_INFO("Tracker: From=%s", *mod_name_from);

	retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
	if (retval <= 0)
		goto recv_mn_size;

	*mod_name = malloc(header_line);
	retval = recv(sockfd, *mod_name, header_line, MSG_WAITALL);
	if (retval <= 0)
		goto recv_mn;
	DEBUG_INFO("Tracker: To=%s", *mod_name);

	retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
	if (retval <= 0)
		goto recv_bufsize;
	void *buffer = malloc(header_line);
	retval = recv(sockfd, buffer, header_line, MSG_WAITALL);
	if (retval <= 0)
		goto recv_buf;
	DEBUG_INFO("Tracker: Vstack size=%d", header_line);

	*vstack = pg_cop_vstack_new(0, header_line);
	if (!vstack)
		goto alloc_vstack;

	if (pg_cop_vstack_import(*vstack, buffer, header_line))
		goto import_vstack;

	free(buffer);
	return 0;

import_vstack:
	pg_cop_vstack_destroy(*vstack);
	*vstack = NULL;
alloc_vstack:
	free(buffer);
	buffer = NULL;
recv_buf:
recv_bufsize:
	free(*mod_name);
	*mod_name = NULL;
recv_mn:
recv_mn_size:
	free(*mod_name_from);
	*mod_name = NULL;
recv_mnf:
recv_mnf_size:
recv_magic:
	return -1;
}

static int _interface_connect(pg_cop_module_interface_t *intf,
                              pg_cop_module_interface_t *peer)
{
	switch (peer->type) {
	case MODULE_INTERFACE_TYPE_THREAD:
		break;
	case MODULE_INTERFACE_TYPE_SOCKET_TCP:
		if (!strcmp(peer->mod_name, "CALLBACK"))
			break;
		// FIXME Check connection;
		if (peer->connection_id >= 0)
			break;

		peer->connection_id = socket(AF_INET, SOCK_STREAM, 0);
		if (peer->connection_id < 0)
			goto socket;
		struct sockaddr_in remote_addr;
		bzero((char *)&remote_addr, sizeof(remote_addr));
		struct hostent *host_info = gethostbyname(peer->host);
		if (!host_info)
			goto host_info;
		memcpy(&remote_addr.sin_addr.s_addr,
		       host_info->h_addr, host_info->h_length);
		remote_addr.sin_family = AF_INET;
		remote_addr.sin_port = htons(peer->port);
retry:
		if (connect(peer->connection_id, (struct sockaddr *) &remote_addr,
		            sizeof(remote_addr))) {
			DEBUG_INFO("Cannot connect to server, retry...");
			sleep(1);
			goto retry;
		}
		break;
	default:
		goto check_type;
	}

	intf->peer = peer;
	return 0;
host_info:
	close(peer->connection_id);
	peer->connection_id = -1;
socket:
check_type:
	return -1;
}

static int _interface_disconnect(pg_cop_module_interface_t *intf)
{
	switch (intf->peer->type) {
	case MODULE_INTERFACE_TYPE_THREAD:
		break;
	case MODULE_INTERFACE_TYPE_SOCKET_TCP:
		close(intf->peer->connection_id);
		intf->peer->connection_id = -1;
		break;
	default:
		goto check_type;
	}

	intf->peer = NULL;
	return 0;

check_type:
	return -1;
}

static void *_interface_tracker_cli(void *arg)
{

	DEBUG_INFO("A new client connected.");

	pg_cop_module_interface_t *intf_cli = pg_cop_module_interface_new("CALLBACK",
	                                      MODULE_INTERFACE_TYPE_SOCKET_TCP, "CALLBACK", 0);
	intf_cli->connection_id = * ((int *) arg);

	while (1) {
		char *mod_name;
		char *mod_name_from;
		pg_cop_vstack_t *vstack;
		int call_type;

		if (_interface_request_recv(intf_cli->connection_id, &call_type,
		                            &vstack, &mod_name, &mod_name_from) != 0)
			goto request_recv;


		pg_cop_module_interface_announcement_t *announce;
		int found = 0;
		pg_cop_module_interface_t *intf_mod = NULL;

		list_for_each_entry(announce, &announced_modules->list_head,
		                    list_head) {
			if (strcmp(announce->intf->mod_name, mod_name) == 0 &&
			        announce->intf->type == MODULE_INTERFACE_TYPE_THREAD) {
				if (call_type == MAGIC_START_INVOKE) {
					intf_mod = announce->intf;
					if (!_interface_connect(intf_mod, intf_cli)) {
						found = 1;
					} else {
						DEBUG_INFO("Cannot connect to peer, skiped.");
					}
				}
				break;
			}
		}

		if (found) {
			if (!intf_mod->vstack) {
				DEBUG_INFO("Not a vaild module.");
				goto check_module;
			}
			if (pg_cop_vstack_transfer(vstack, intf_mod->vstack))
				goto vstack_transfer;
			sem_post(&intf_mod->recv_sem);
		} else {
			DEBUG_INFO("Cannot find request module. %s", mod_name);
		}

		pg_cop_vstack_destroy(vstack);
		free(mod_name);
		free(mod_name_from);
		continue;

vstack_transfer:
check_module:
		pg_cop_vstack_destroy(vstack);
		free(mod_name);
		free(mod_name_from);
		continue;
request_recv:
		break;
	}

	intf_cli->connection_id = -1;
	pg_cop_module_interface_destroy(intf_cli);
	DEBUG_INFO("Client disconnected.");
	pthread_exit(0);
	return NULL;
}

static void *_interface_tracker(void *arg)
{
	int retval;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		goto socket;

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(tracker_incoming_port);

	DEBUG_INFO("Tracker started.");

retry_bind:
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
	         sizeof(serv_addr)) < 0) {
		DEBUG_ERROR("Failed to bind on port %d. retrying...", ntohs(serv_addr.sin_port));
		sleep(1);
		goto retry_bind;
	}

retry_listen:
	if (listen(sockfd, 5)) {
		DEBUG_ERROR("Failed to listen on port. %d, retrying...", ntohs(serv_addr.sin_port));
		sleep(1);
		goto retry_listen;
	}
	DEBUG_INFO("Listen on port %d", ntohs(serv_addr.sin_port));

	int sockfd_cli;
	while ((sockfd_cli = accept(sockfd, NULL, 0))) {
		if (sockfd_cli < 0)
			goto accept;
		pthread_attr_t attr;
		retval = pthread_attr_init(&attr);
		if (retval != 0) {
			DEBUG_ERROR("Cannot create thread attributes.");
			goto create_attr;
		}

		pthread_t thread;
		if (pthread_create(&thread, &attr, _interface_tracker_cli, &sockfd_cli)) {
			DEBUG_ERROR("Cannot create thread.");
			goto create_thread;
		}
		pthread_attr_destroy(&attr);
		continue;

create_thread:
		pthread_attr_destroy(&attr);
create_attr:
		;
	}

	DEBUG_INFO("Server down");
	pthread_exit(0);
	return NULL;

accept:
	close(sockfd);
socket:
	pthread_exit(0);
	return NULL;
}

int pg_cop_module_interface_daemon_init()
{
	announced_modules = (pg_cop_module_interface_announcement_t*)
	                    malloc(sizeof(pg_cop_module_interface_announcement_t));
	if (!announced_modules)
		goto alloc_ann;
	INIT_LIST_HEAD(&announced_modules->list_head);

	return 0;

alloc_ann:
	return -1;
}

int pg_cop_module_interface_daemon_start()
{
	pthread_attr_t attr;

	int retval = pthread_attr_init(&attr);
	if (retval != 0) {
		DEBUG_ERROR("Cannot create thread attributes.");
		goto init_attr;
	}

	pthread_t thread;
	if ((thread = pthread_create(&thread, &attr, _interface_tracker, NULL))) {
		DEBUG_ERROR("Cannot create thread.");
		goto create_thread;
	}

	pthread_attr_destroy(&attr);

	return 0;

create_thread:
	pthread_attr_destroy(&attr);
init_attr:
	return -1;
}

int pg_cop_module_interface_wait(pg_cop_module_interface_t *intf, char **method)
{
	sem_wait(&intf->recv_sem);

	if (pg_cop_vstack_pop(intf->vstack, VSTACK_TYPE_STRING, method))
		goto pop;

	return 0;
pop:
	return -1;
}

pg_cop_module_interface_t *pg_cop_module_interface_new(const char *name,
        pg_cop_module_interface_type_t type,
        ...)
{
	pg_cop_module_interface_t *intf = (pg_cop_module_interface_t *) malloc
	                                  (sizeof(pg_cop_module_interface_t));
	if (!intf)
		goto intf_alloc;

	va_list va;
	switch (type) {
	case MODULE_INTERFACE_TYPE_THREAD:
		intf->host = NULL;
		intf->port = 0;
		break;
	case MODULE_INTERFACE_TYPE_SOCKET_TCP:
		va_start(va, type);
		intf->host = strdup(va_arg(va, char *));
		intf->port = va_arg(va, int);
		va_end(va);
		break;
	default:
		goto check_type;
	}

	intf->vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024);
	if (!intf->vstack)
		goto vstack_alloc;

	intf->mod_name = strdup(name);
	intf->type = type;
	intf->peer = NULL;
	intf->connection_id = -1;
	sem_init(&intf->recv_sem, 0, 0);

	return intf;

vstack_alloc:
check_type:
	free(intf);
	intf = NULL;
intf_alloc:
	return NULL;
}

int pg_cop_module_interface_destroy(pg_cop_module_interface_t *intf)
{
	if (!intf)
		goto check_arg;
	pg_cop_vstack_destroy(intf->vstack);
	sem_destroy(&intf->recv_sem);
	free(intf->host);
	intf->host = NULL;
	free(intf->mod_name);
	intf->mod_name = NULL;
	free(intf);
	return 0;

check_arg:
	return -1;
}

pg_cop_module_interface_t *pg_cop_module_interface_connect(const char *name)
{
	pg_cop_module_interface_t *intf = pg_cop_module_interface_new("PEER", MODULE_INTERFACE_TYPE_THREAD, 0);
	if (!intf)
		goto intf_alloc;

	for (;;) {
		pg_cop_module_interface_announcement_t *announce;
		int found = 0;
		list_for_each_entry(announce, &announced_modules->list_head,
		                    list_head) {
			if (strcmp(announce->intf->mod_name, name) == 0) {
				if (!_interface_connect(intf, announce->intf)) {
					found = 1;
					break;
				}
			}
		}

		if (found)
			break;
		sleep(1);
	}

	return intf;

intf_alloc:
	return NULL;
}

int pg_cop_module_interface_disconnect(pg_cop_module_interface_t *intf)
{
	if (!intf)
		goto check_arg;
	if (_interface_disconnect(intf))
		goto disconnect;
	pg_cop_module_interface_destroy(intf);
	return 0;

disconnect:
check_arg:
	return -1;
}

int pg_cop_module_interface_invoke(pg_cop_module_interface_t *intf,
                                   const char *method, int num, ...)
{
	if (intf == NULL || intf->peer == NULL || num < 0)
		goto check_args;

	if (pg_cop_vstack_push(intf->vstack, VSTACK_TYPE_STRING, method) != 0)
		goto push_method;

	va_list va;
	va_start(va, num);
	for (int i = 0; i < num; i++) {
		pg_cop_vstack_type_t type = va_arg(va, pg_cop_vstack_type_t);
		switch (type) {
		case VSTACK_TYPE_DATASIZE:
			if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*),
			                       va_arg(va, void*)) != 0)
				goto push_datas;
			break;
		case VSTACK_TYPE_STRING:
			if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*)) != 0)
				goto push_datas;
			break;
		default:
			if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, int)) != 0)
				goto push_datas;
		}
	}
	va_end(va);

	switch (intf->peer->type) {
	case MODULE_INTERFACE_TYPE_THREAD:
		if (pg_cop_vstack_transfer(intf->vstack, intf->peer->vstack))
			goto thread_transfer;
		intf->peer->peer = intf;
		sem_post(&intf->peer->recv_sem);
		sem_wait(&intf->recv_sem);
		break;
	case MODULE_INTERFACE_TYPE_SOCKET_TCP:
		if (pg_cop_vstack_has_more(intf->vstack)) {
			char *mod_name;
			char *mod_name_from;
			pg_cop_vstack_t *vstack;
			if (_interface_request_send(intf,
			                            MAGIC_START_INVOKE) != 0)
				goto request_send;
			pg_cop_vstack_clear(intf->vstack);

			int call_type;
			if (_interface_request_recv(intf->peer->connection_id, &call_type,
			                            &vstack, &mod_name, &mod_name_from) != 0)
				goto recv_return;
			if (pg_cop_vstack_transfer(vstack, intf->vstack))
				goto socket_transfer_return;
			free(mod_name_from);
			free(mod_name);
			pg_cop_vstack_destroy(vstack);
			break;

socket_transfer_return:
			free(mod_name_from);
			free(mod_name);
			pg_cop_vstack_destroy(vstack);
		} else {
			DEBUG_ERROR("Invalid invoke.");
			goto check_vstack;
		}
		break;
	default:
		goto check_type;
	}

	return 0;

recv_return:
request_send:
thread_transfer:
check_vstack:
check_type:
push_datas:
	pg_cop_vstack_clear(intf->vstack);
push_method:
check_args:
	DEBUG_INFO("Peer disconnected.");
	return -1;
}

int pg_cop_module_interface_return(pg_cop_module_interface_t *intf, int num, ...)
{
	if (intf == NULL || intf->peer == NULL || num < 0) {
		DEBUG_ERROR("No interface.");
		goto check_args;
	}

	va_list va;
	va_start(va, num);
	for (int i = 0; i < num; i++) {
		pg_cop_vstack_type_t type = va_arg(va, pg_cop_vstack_type_t);
		switch (type) {
		case VSTACK_TYPE_DATASIZE:
			if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*),
			                       va_arg(va, void*)) != 0) {
				goto push_datas;
			}
			break;
		case VSTACK_TYPE_STRING:
			if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*)) != 0)
				goto push_datas;
			break;
		default:
			if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, int)) != 0)
				goto push_datas;
		}
	}
	va_end(va);

	switch (intf->peer->type) {
	case MODULE_INTERFACE_TYPE_THREAD:
		pg_cop_vstack_transfer(intf->vstack, intf->peer->vstack);
		sem_post(&intf->peer->recv_sem);
		break;
	case MODULE_INTERFACE_TYPE_SOCKET_TCP:
		if (pg_cop_vstack_has_more(intf->vstack)) {
			if (_interface_request_send(intf,
			                            MAGIC_START_RETURN) != 0)
				goto request_send;
			pg_cop_vstack_clear(intf->vstack);
		}
		break;
	default:
		goto check_type;
	}

	return 0;

request_send:
	pg_cop_vstack_clear(intf->vstack);
check_type:
push_datas:
check_args:
	DEBUG_INFO("Peer disconnected.");
	return -1;
}

pg_cop_module_interface_t *pg_cop_module_interface_announce(const char *name,
        pg_cop_module_interface_type_t type,
        ...)
{
	char *host;
	int port;

	if (type == MODULE_INTERFACE_TYPE_SOCKET_TCP) {
		va_list va;
		va_start(va, type);
		host = strdup(va_arg(va, char *));
		port = va_arg(va, int);
		va_end(va);
	} else {
		host = strdup("dummy");
		port = 0;
	}

	pg_cop_module_interface_announcement_t * announce;
	list_for_each_entry(announce, &announced_modules->list_head,
	                    list_head) {
		if (strcmp(announce->intf->mod_name, name) == 0 &&
		        announce->intf->type == type &&
		        strcmp(announce->intf->host, host) == 0 &&
		        announce->intf->port == port) {
			free(host);
			return announce->intf;
		}
	}

	pg_cop_module_interface_t *intf = pg_cop_module_interface_new(name, type, host, port);
	if (!intf)
		goto intf_alloc;
	free(host);

	announce = (pg_cop_module_interface_announcement_t *) malloc(sizeof(pg_cop_module_interface_announcement_t));
	if (!announce)
		goto announce_alloc;
	announce->intf = intf;

	list_add_tail(&announce->list_head, &announced_modules->list_head);
	DEBUG_INFO("Module %s announced.", name);

	return intf;

announce_alloc:
	pg_cop_module_interface_destroy(intf);
intf_alloc:
	return NULL;
}

int pg_cop_module_interface_revoke(pg_cop_module_interface_t *intf)
{
	pg_cop_module_interface_announcement_t *announce, *announce_tmp;

	list_for_each_entry_safe(announce, announce_tmp,
	                         &announced_modules->list_head, list_head) {
		if (announce->intf == intf) {
			DEBUG_INFO("Module %s revoked with type %d.", announce->intf->mod_name,
			           announce->intf->type);
			pg_cop_module_interface_destroy(announce->intf);
			list_del(&announce->list_head);
			free(announce);
		}
	}

	return 0;
}
