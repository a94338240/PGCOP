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

pg_cop_module_interface_announcement_t *announced_modules = NULL;

static int _interface_connect(pg_cop_module_interface_t *intf, 
                              pg_cop_module_interface_t *peer) 
{
  struct sockaddr_in remote_addr;
  struct hostent *host_info;

  switch (peer->type) {
  case MODULE_INTERFACE_TYPE_THREAD:
  case MODULE_INTERFACE_TYPE_SOCKET_TCP_BACK:
    break;
  case MODULE_INTERFACE_TYPE_SOCKET_TCP:
    peer->connection_id = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&remote_addr, sizeof(remote_addr));
    host_info = gethostbyname("127.0.0.1");
    memcpy(&remote_addr.sin_addr.s_addr, 
           host_info->h_addr, host_info->h_length);
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(12728);
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

static void *_interface_tracker_cli(void *arg)
{
  int sockfd = *((int *)arg);
  int retval = 0;
  int header_line;
  int call_type = 0;
  char *mod_name;
  char *mod_name_from;
  char *buffer;
  pg_cop_module_interface_t *intf_cli;
  pg_cop_module_interface_t *intf_mod;
  pg_cop_vstack_t *vstack;
  pg_cop_module_interface_announcement_t *announce;
  int found = 0;

  while (1) {
  retry:
    retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
    if (retval <= 0)
      goto out;
    if (header_line != MAGIC_START_INVOKE && header_line != MAGIC_START_RETURN)
      goto retry;
    call_type = header_line;
    DEBUG_INFO("Tracker: Start receiving. type=%d", call_type);

    retval = retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
    if (retval <= 0)
      goto out;

    mod_name_from = malloc(header_line);
    retval = recv(sockfd, mod_name_from, header_line, MSG_WAITALL);
    if (retval <= 0)
      goto out;
    DEBUG_INFO("Tracker: From=%s", mod_name_from);

    retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
    if (retval <= 0)
      goto out;

    mod_name = malloc(header_line);
    retval = recv(sockfd, mod_name, header_line, MSG_WAITALL); 
    if (retval <= 0)
      goto out;
    DEBUG_INFO("Tracker: To=%s", mod_name);

    retval = recv(sockfd, &header_line, sizeof(int), MSG_WAITALL);
    if (retval <= 0)
      goto out;
    buffer = malloc(header_line);
    retval = recv(sockfd, buffer, header_line, MSG_WAITALL);
    if (retval <= 0)
      goto out;
    DEBUG_INFO("Tracker: Vstack size=%d", header_line);

    vstack = pg_cop_vstack_new(0, header_line);
    pg_cop_vstack_import(vstack, buffer, header_line);

    if (call_type == MAGIC_START_INVOKE) {
      intf_cli = pg_cop_module_interface_new(mod_name_from, 
                                             MODULE_INTERFACE_TYPE_SOCKET_TCP_BACK);
      intf_cli->connection_id = sockfd;
    }
    list_for_each_entry(announce, &announced_modules->list_head, 
                        list_head) {
      if (strcmp(announce->intf->mod_name, mod_name) == 0 &&
          announce->intf->type == MODULE_INTERFACE_TYPE_THREAD) {
        intf_mod = announce->intf;
        if (call_type == MAGIC_START_INVOKE) {
          if (!_interface_connect(intf_mod, intf_cli))
            found = 1;
        } else {
          found = 1;
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
    free(buffer);
    free(mod_name);
  }

 out:
  free(mod_name_from);
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
  serv_addr.sin_port = htons(12728);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&serv_addr,
           sizeof(serv_addr)) < 0) {
    DEBUG_ERROR("Failed to bind on port %d.", ntohs(serv_addr.sin_port));
    return NULL;
  }

  if (listen(sockfd, 5)) {
    DEBUG_ERROR("Failed to listen on port. %d", ntohs(serv_addr.sin_port));
    return NULL;
  }
  DEBUG_INFO("Listen on port %d", ntohs(serv_addr.sin_port));

  while (sockfd_cli = accept(sockfd, NULL, 0)) {
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

  return NULL;
}

int pg_cop_module_interface_tracker_init()
{
  int retval;
  pthread_t thread;
  pthread_attr_t attr;

  announced_modules = (pg_cop_module_interface_announcement_t*)
    malloc(sizeof(pg_cop_module_interface_announcement_t));
  INIT_LIST_HEAD(&announced_modules->list_head);

  retval = pthread_attr_init(&attr);
  if (retval != 0) {
    DEBUG_ERROR("Cannot create thread attributes.");
    return -1;
  }
  
  if (pthread_create(&thread, &attr, _interface_tracker, NULL)) {
    DEBUG_ERROR("Cannot create thread.");
    return -1;
  }
}

int pg_cop_module_interface_wait(pg_cop_module_interface_t *intf, char **method)
{
  sem_wait(&intf->recv_sem);
  
  if (!pg_cop_vstack_pop(intf->vstack, VSTACK_TYPE_STRING, method))
  return 0;
}

pg_cop_module_interface_t *pg_cop_module_interface_new(const char *name, 
                                                       pg_cop_module_interface_type_t type)
{
  pg_cop_module_interface_t *intf = (pg_cop_module_interface_t *)malloc
    (sizeof(pg_cop_module_interface_t));
  intf->vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024);
  intf->mod_name = name;
  intf->type = type;
  intf->peer = NULL;
  sem_init(&intf->recv_sem, 0, 0);

  return intf;
}

int pg_cop_module_interface_connect(pg_cop_module_interface_t *intf, 
                                    const char *name)
{
  pg_cop_module_interface_announcement_t *announce;
  int found = 0;

  for (;;) {
    list_for_each_entry(announce, &announced_modules->list_head, 
                        list_head) {
      if (strcmp(announce->intf->mod_name, name) == 0) {
        if (!_interface_connect(intf, announce->intf))
          found = 1;
      }
    }
    if (found)
      break;
    sleep(1);
  }
  return 0;
}

int pg_cop_module_interface_disconnect(pg_cop_module_interface_t *intf)
{
  if (!intf)
    return -1;
  if (intf->vstack)
    free(intf->vstack);
  sem_destroy(&intf->recv_sem);
  if (intf)
    free(intf);
}

int pg_cop_module_interface_invoke(pg_cop_module_interface_t *intf, 
                                   const char *method, int num, ...)
{
  va_list va;
  pg_cop_vstack_type_t type;
  int size;
  int retval = 0;
  int i, found = 0;
  void *stack_dump;
  int header_line = 0;
  char *mod_name;
  char *mod_name_from;
  char *buffer;
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
      header_line = MAGIC_START_INVOKE;
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

    retry:
      retval = recv(intf->peer->connection_id, &header_line, sizeof(int), MSG_WAITALL);
      if (retval <= 0)
        goto out;
      if (header_line != MAGIC_START_INVOKE && header_line != MAGIC_START_RETURN)
        goto retry;
      call_type = header_line;
      DEBUG_INFO("Tracker: Start receiving. type=%d", call_type);

      retval = recv(intf->peer->connection_id, &header_line, sizeof(int), MSG_WAITALL);
      if (retval <= 0)
        goto out;

      mod_name_from = malloc(header_line);
      retval = recv(intf->peer->connection_id, mod_name_from, header_line, MSG_WAITALL);
      if (retval <= 0)
        goto out;
      DEBUG_INFO("Tracker: From=%s", mod_name_from);

      retval = recv(intf->peer->connection_id, &header_line, sizeof(int), MSG_WAITALL);
      if (retval <= 0)
        goto out;

      mod_name = malloc(header_line);
      retval = recv(intf->peer->connection_id, mod_name, header_line, MSG_WAITALL);
      if (retval <= 0)
        goto out;
      DEBUG_INFO("Tracker: To=%s", mod_name);

      retval = recv(intf->peer->connection_id, &header_line, sizeof(int), MSG_WAITALL);
      if (retval <= 0)
        goto out;
      buffer = malloc(header_line);
      retval = recv(intf->peer->connection_id, buffer, header_line, MSG_WAITALL);
      if (retval <= 0)
        goto out;
      DEBUG_INFO("Tracker: Vstack size=%d", header_line);

      vstack = pg_cop_vstack_new(0, header_line);
      pg_cop_vstack_import(vstack, buffer, header_line);
      pg_cop_vstack_transfer(vstack, intf->vstack);
    }
    break;
  default:
    return -1;
  }

  return 0;

 out:
  DEBUG_INFO("Peer disconnected.");
  return -1;
}

int pg_cop_module_interface_return(pg_cop_module_interface_t *intf, int num, ...)
{
  va_list va;
  pg_cop_vstack_type_t type;
  int size;
  int retval = 0;
  int i, found = 0;
  void *stack_dump;
  int header_line;

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
  case MODULE_INTERFACE_TYPE_SOCKET_TCP_BACK:
    if (pg_cop_vstack_has_more(intf->vstack)) {
      header_line = MAGIC_START_RETURN;
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
    }
    break;
  default:
    return -1;
  }

  return 0;

 out:
  DEBUG_INFO("Peer disconnected.");
  return -1;
}

int pg_cop_module_interface_announce(pg_cop_module_interface_t *intf)
{
  pg_cop_module_interface_announcement_t * announce = 
    (pg_cop_module_interface_announcement_t *)malloc
    (sizeof(pg_cop_module_interface_announcement_t));

  announce->intf = intf;
  list_add_tail(&announce->list_head, &announced_modules->list_head);

  return 0;
}

