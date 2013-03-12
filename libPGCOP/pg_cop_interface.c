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
  void *stack_dump;

  header_line = call_type;
  retval = send(intf->peer->connection_id, &header_line,
                sizeof(int), 0);
  if (retval <= 0)
    goto out;

  header_line = strlen(intf->mod_name) + 1;
  retval = send(intf->peer->connection_id, &header_line,
                sizeof(int), 0);
  if (retval <= 0)
    goto out;
  retval = send(intf->peer->connection_id, intf->mod_name,
                header_line, 0);
  if (retval <= 0)
    goto out;

  header_line = strlen(intf->peer->mod_name) + 1;
  retval = send(intf->peer->connection_id, &header_line,
                sizeof(int), 0);
  if (retval <= 0)
    goto out;
  retval = send(intf->peer->connection_id, intf->peer->mod_name,
                header_line, 0);
  if (retval <= 0)
    goto out;

  header_line = pg_cop_vstack_used_bytes(intf->vstack);
  retval = send(intf->peer->connection_id, &header_line,
                sizeof(int), 0);
  if (retval <= 0)
    goto out;

  stack_dump = pg_cop_vstack_dump(intf->vstack);
  retval = send(intf->peer->connection_id, stack_dump, 
                header_line, 0);
  if (retval <= 0)
    goto out;
  pg_cop_vstack_clear(intf->vstack);
  free(stack_dump);
  return 0;

 out:
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
  char *buffer;

 retry:
  retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
  if (retval <= 0)
    goto out;
  if (header_line != MAGIC_START_INVOKE && header_line != MAGIC_START_RETURN)
    goto retry;
  *call_type = header_line;
  DEBUG_INFO("Tracker: Start receiving. type=%d", *call_type);

  retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
  if (retval <= 0)
    goto out;

  *mod_name_from = malloc(header_line);
  retval = recv(sockfd, *mod_name_from, header_line, MSG_WAITALL);
  if (retval <= 0)
    goto out;
  DEBUG_INFO("Tracker: From=%s", *mod_name_from);

  retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
  if (retval <= 0)
    goto out;

  *mod_name = malloc(header_line);
  retval = recv(sockfd, *mod_name, header_line, MSG_WAITALL); 
  if (retval <= 0)
    goto out;
  DEBUG_INFO("Tracker: To=%s", *mod_name);

  retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
  if (retval <= 0)
    goto out;
  buffer = malloc(header_line);
  retval = recv(sockfd, buffer, header_line, MSG_WAITALL);
  if (retval <= 0)
    goto out;
  DEBUG_INFO("Tracker: Vstack size=%d", header_line);

  *vstack = pg_cop_vstack_new(0, header_line);
  pg_cop_vstack_import(*vstack, buffer, header_line);

  free(buffer);
  return 0;

 out:
  return -1;
}

static int _interface_connect(pg_cop_module_interface_t *intf, 
                              pg_cop_module_interface_t *peer) 
{
  struct sockaddr_in remote_addr;
  struct hostent *host_info;

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
    bzero((char *)&remote_addr, sizeof(remote_addr));
    host_info = gethostbyname(peer->host);
    memcpy(&remote_addr.sin_addr.s_addr, 
           host_info->h_addr, host_info->h_length);
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(peer->port);
  retry:
    if (connect(peer->connection_id, (struct sockaddr *)&remote_addr, 
                sizeof(remote_addr))) {
      DEBUG_INFO("Cannot connect to server, retry...");
      sleep(1);
      goto retry;
    }
    break;
  default:
    return -1;
  }

  intf->peer = peer;
  return 0;
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
    return -1;
  }

  intf->peer = NULL;
  return 0;
}

static void *_interface_tracker_cli(void *arg)
{
  int sockfd = *((int *)arg);
  char *mod_name;
  char *mod_name_from;
  pg_cop_module_interface_t *intf_cli = NULL;
  pg_cop_module_interface_t *intf_mod = NULL;
  pg_cop_vstack_t *vstack;
  pg_cop_module_interface_announcement_t *announce;
  int found = 0;
  int call_type;

  DEBUG_INFO("A new client connected.");

  intf_cli = pg_cop_module_interface_new("CALLBACK", 
                                         MODULE_INTERFACE_TYPE_SOCKET_TCP, 0);
  intf_cli->connection_id = sockfd;

  while (1) {
    if (_interface_request_recv(sockfd, &call_type, 
                                &vstack, &mod_name, &mod_name_from) != 0)
      goto out;

    list_for_each_entry(announce, &announced_modules->list_head, 
                        list_head) {
      if (strcmp(announce->intf->mod_name, mod_name) == 0 &&
          announce->intf->type == MODULE_INTERFACE_TYPE_THREAD) {
        intf_mod = announce->intf;
        if (call_type == MAGIC_START_INVOKE) {
          if (!_interface_connect(intf_mod, intf_cli)) {
            found = 1;
          }
        }
        break;
      }
    }

    if (found) {
      if (!intf_mod->vstack) {
        DEBUG_INFO("Not a vaild module.");
        goto out;
      }
      pg_cop_vstack_transfer(vstack, intf_mod->vstack);
      sem_post(&intf_mod->recv_sem);
    }
    else {
      DEBUG_INFO("Cannot find request module. %s", mod_name);
    }
    pg_cop_vstack_destroy(vstack);
    free(mod_name);
    free(mod_name_from);
  }

 out:
  close(intf_cli->connection_id);
  pg_cop_module_interface_destroy(intf_cli);
  DEBUG_INFO("Client disconnected.");
  return NULL;
}

static void *_interface_tracker(void *arg)
{
  int sockfd;
  int sockfd_cli;
  struct sockaddr_in serv_addr;
  pthread_t thread;
  pthread_attr_t attr;
  int retval;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(tracker_incoming_port);

  DEBUG_INFO("Tracker started.");

 retry_bind:
  if (bind(sockfd, (struct sockaddr *)&serv_addr,
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

  while ((sockfd_cli = accept(sockfd, NULL, 0))) {
    retval = pthread_attr_init(&attr);
    if (retval != 0) {
      DEBUG_ERROR("Cannot create thread attributes.");
      return NULL;
    }
  
    if (pthread_create(&thread, &attr, _interface_tracker_cli, &sockfd_cli)) {
      DEBUG_ERROR("Cannot create thread.");
      return NULL;
    }
  }

  DEBUG_INFO("Server down");
  close(sockfd);

  return NULL;
}

int pg_cop_module_interface_daemon_init()
{
  announced_modules = (pg_cop_module_interface_announcement_t*)
    malloc(sizeof(pg_cop_module_interface_announcement_t));
  INIT_LIST_HEAD(&announced_modules->list_head);

  return 0;
}

int pg_cop_module_interface_daemon_start()
{
  int retval;
  pthread_t thread;
  pthread_attr_t attr;

  retval = pthread_attr_init(&attr);
  if (retval != 0) {
    DEBUG_ERROR("Cannot create thread attributes.");
    return -1;
  }
  
  if (pthread_create(&thread, &attr, _interface_tracker, NULL)) {
    DEBUG_ERROR("Cannot create thread.");
    return -1;
  }

  return 0;
}

int pg_cop_module_interface_wait(pg_cop_module_interface_t *intf, char **method)
{
  sem_wait(&intf->recv_sem);
  
  if (!pg_cop_vstack_pop(intf->vstack, VSTACK_TYPE_STRING, method))
    return 0;
  return -1;
}

pg_cop_module_interface_t *pg_cop_module_interface_new(const char *name, 
                                                       pg_cop_module_interface_type_t type,
                                                       ...)
{
  pg_cop_module_interface_t *intf = (pg_cop_module_interface_t *)malloc
    (sizeof(pg_cop_module_interface_t));
  va_list va;

  intf->vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024);
  intf->mod_name = strdup(name);
  intf->type = type;
  intf->peer = NULL;
  intf->connection_id = -1;
  intf->host = NULL;
  sem_init(&intf->recv_sem, 0, 0);

  switch (type) {
  case MODULE_INTERFACE_TYPE_THREAD:
    break;
  case MODULE_INTERFACE_TYPE_SOCKET_TCP:
    if (strcmp(name, "CALLBACK")) {
      va_start(va, type);
      intf->host = strdup(va_arg(va, char *));
      intf->port = va_arg(va, int);
      va_end(va);
    }
    break;
  }

  return intf;
}

int pg_cop_module_interface_destroy(pg_cop_module_interface_t *intf)
{
  if (intf) {
    pg_cop_vstack_destroy(intf->vstack);
    sem_destroy(&intf->recv_sem);
    if (intf->host)
      free(intf->host);
    intf->host = NULL;
    if (intf->mod_name)
      free(intf->mod_name);
    intf->mod_name = NULL;
    free(intf);
    return 0;
  }
  return -1;
}

pg_cop_module_interface_t *pg_cop_module_interface_connect(const char *name)
{
  pg_cop_module_interface_announcement_t *announce;
  int found = 0;
  pg_cop_module_interface_t *intf = NULL;
  intf = pg_cop_module_interface_new("PEER", MODULE_INTERFACE_TYPE_THREAD, 
                                     0);

  for (;;) {
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
}

int pg_cop_module_interface_disconnect(pg_cop_module_interface_t *intf)
{
  if (!intf)
    return -1;
  _interface_disconnect(intf);
  pg_cop_module_interface_destroy(intf);
  return 0;
}

int pg_cop_module_interface_invoke(pg_cop_module_interface_t *intf, 
                                   const char *method, int num, ...)
{
  va_list va;
  pg_cop_vstack_type_t type;
  int i = 0;
  char *mod_name;
  char *mod_name_from;
  pg_cop_vstack_t *vstack;
  int call_type;

  if (intf == NULL || intf->peer == NULL || num < 0)
    return -1;

  if (pg_cop_vstack_push(intf->vstack, VSTACK_TYPE_STRING, method) != 0)
    return -1;

  va_start(va, num);
  for (i = 0; i < num; i++) {
    type = va_arg(va, pg_cop_vstack_type_t);
    switch (type) {
    case VSTACK_TYPE_DATASIZE:
      if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*), 
                             va_arg(va, void*)) != 0)
        return -1;
      break;
    case VSTACK_TYPE_STRING:
      if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*)) != 0)
        return -1;
      break;
    default:
      if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, int)) != 0)
        return -1;
    }
  }
  va_end(va);

  switch (intf->peer->type) {
  case MODULE_INTERFACE_TYPE_THREAD:
    pg_cop_vstack_transfer(intf->vstack, intf->peer->vstack);
    intf->peer->peer = intf;
    sem_post(&intf->peer->recv_sem);
    sem_wait(&intf->recv_sem);
    break;
  case MODULE_INTERFACE_TYPE_SOCKET_TCP:
    if (pg_cop_vstack_has_more(intf->vstack)) {
      if (_interface_request_send(intf,
                                  MAGIC_START_INVOKE) != 0)
        goto out;
      pg_cop_vstack_clear(intf->vstack);
      if (_interface_request_recv(intf->peer->connection_id, &call_type, 
                                  &vstack, &mod_name, &mod_name_from) != 0)
        goto out;
      pg_cop_vstack_transfer(vstack, intf->vstack);
    }
    free(mod_name_from);
    free(mod_name);
    pg_cop_vstack_destroy(vstack);
    break;
  default:
    return -1;
  }

  return 0;

 out:
  intf->peer->connection_id = -1;
  DEBUG_INFO("Peer disconnected.");
  return -1;
}

int pg_cop_module_interface_return(pg_cop_module_interface_t *intf, int num, ...)
{
  va_list va;
  pg_cop_vstack_type_t type;
  int i;

  if (intf == NULL || intf->peer == NULL || num < 0)
    return -1;

  va_start(va, num);
  for (i = 0; i < num; i++) {
    type = va_arg(va, pg_cop_vstack_type_t);
    switch (type) {
    case VSTACK_TYPE_DATASIZE:
      if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*), 
                             va_arg(va, void*)) != 0)
        return -1;
      break;
    case VSTACK_TYPE_STRING:
      if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, void*)) != 0)
        return -1;
      break;
    default:
      if (pg_cop_vstack_push(intf->vstack, type, va_arg(va, int)) != 0)
        return -1;
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
        goto out;
    }
    pg_cop_vstack_clear(intf->vstack);
    break;
  default:
    return -1;
  }

  return 0;

 out:
  intf->peer->connection_id = -1;
  DEBUG_INFO("Peer disconnected.");
  return -1;
}

pg_cop_module_interface_t *pg_cop_module_interface_announce(const char *name,
                                                            pg_cop_module_interface_type_t type,
                                                            ...)
{
  pg_cop_module_interface_announcement_t * announce;
  pg_cop_module_interface_t *intf;
  const char *host;
  int port;
  va_list va;

  va_start(va, type);
  host = va_arg(va, char *);
  port = va_arg(va, int);

  list_for_each_entry(announce, &announced_modules->list_head,
                      list_head) {
    if (strcmp(announce->intf->mod_name, name) == 0 &&
        announce->intf->type == type &&
        strcmp(announce->intf->host, host) == 0 && 
        announce->intf->port == port) {
      return announce->intf;
    }
  }

  intf = pg_cop_module_interface_new(name, type, host, port);
  announce = 
    (pg_cop_module_interface_announcement_t *)malloc
    (sizeof(pg_cop_module_interface_announcement_t));
  announce->intf = intf;
                      
  list_add_tail(&announce->list_head, &announced_modules->list_head);
  DEBUG_INFO("Module %s announced.", name);

  return intf;
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
