#ifndef PG_COP_CONFIG_H
#define PG_COP_CONFIG_H

extern const char *pg_cop_lua_config_file;

void pg_cop_read_config();
int pg_cop_get_module_config_number(const char *conf_key, int *num);
int pg_cop_get_module_config_strdup(const char *conf_key, char **str);
int pg_cop_get_config_number(const char *conf_key, int *num);
int pg_cop_get_config_strdup(const char *conf_key, char **str);

#endif /* PG_COP_CONFIG_H */
