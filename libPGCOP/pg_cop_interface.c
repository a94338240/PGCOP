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
#include <unistd.h>

pg_cop_module_interface_announcement_t *announced_modules = NULL;

int pg_cop_module_interface_tracker_init()
{
  announced_modules = (pg_cop_module_interface_announcement_t*)
    malloc(sizeof(pg_cop_module_interface_announcement_t));
  INIT_LIST_HEAD(&announced_modules->list_head);
}

int pg_cop_module_interface_wait(pg_cop_module_interface_t *intf, char **method)
{
  sem_wait(&intf->recv_sem);
  
  if (!pg_cop_vstack_pop(intf->vstack, VSTACK_TYPE_STRING, method))
  return 0;
}

pg_cop_module_interface_t *pg_cop_module_interface_connect(const char *name)
{
  pg_cop_module_interface_t *intf = (pg_cop_module_interface_t *)malloc
    (sizeof(pg_cop_module_interface_t));
  pg_cop_module_interface_announcement_t *announce;
  int found = 0;

  intf->vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024, VSTACK_SCOPE_THREAD);
  intf->mod_name = name;
  sem_init(&intf->recv_sem, 0, 0);

  for (;;) {
    list_for_each_entry(announce, &announced_modules->list_head, 
                        list_head)
      if (strcmp(announce->intf->mod_name, intf->mod_name) == 0) {
        intf->peer = announce->intf;
        found = 1;
        break;
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

  if (intf == NULL || num < 0)
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

  pg_cop_vstack_transfer(intf->vstack, intf->peer->vstack);
  intf->peer->peer = intf;
  sem_post(&intf->peer->recv_sem);
  sem_wait(&intf->recv_sem);

  return 0;
}

int pg_cop_module_interface_return(pg_cop_module_interface_t *intf, int num, ...)
{
  va_list va;
  pg_cop_vstack_type_t type;
  int size;
  int retval = 0;
  int i, found = 0;

  if (intf == NULL || num < 0)
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

  pg_cop_vstack_transfer(intf->vstack, intf->peer->vstack);
  sem_post(&intf->peer->recv_sem);

  return 0;
}

pg_cop_module_interface_t * pg_cop_module_interface_announce(pg_cop_module_t *module, 
                                                             pg_cop_vstack_scope_t scope)
{
  int i = 0;
  pg_cop_module_interface_t * intf = 
    (pg_cop_module_interface_t *)malloc
    (sizeof(pg_cop_module_interface_t));
  pg_cop_module_interface_announcement_t * announce = 
    (pg_cop_module_interface_announcement_t *)malloc
    (sizeof(pg_cop_module_interface_announcement_t));
  
  intf->vstack = pg_cop_vstack_new(0, 8 * 1024 * 1024, VSTACK_SCOPE_THREAD);
  intf->mod_name = module->info->name;
  sem_init(&intf->recv_sem, 0, 0);
  announce->intf = intf;
  list_add_tail(&announce->list_head, &announced_modules->list_head);

  return intf;
}
