/*
 *   This program is is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or (at
 *   your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * $Id$
 * @file rlm_sql.h
 * @brief Prototypes and functions for the SQL module
 *
 * @copyright 2012-2014  Arran Cudbard-Bell <a.cudbardb@freeradius.org>
 * @copyright 2000,2006  The FreeRADIUS server project
 * @copyright 2000  Mike Machado <mike@innercite.com>
 * @copyright 2000  Alan DeKok <aland@ox.org>
 */
#ifndef _RLM_SQL_H
#define _RLM_SQL_H

RCSIDH(rlm_sql_h, "$Id$")

#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/connection.h>
#include <freeradius-devel/modpriv.h>
#include <freeradius-devel/exfile.h>

#define PW_ITEM_CHECK		0
#define PW_ITEM_REPLY		1


/* SQL Errors */
typedef enum {
	RLM_SQL_QUERY_ERROR = -3,	//!< Query syntax error
	RLM_SQL_ERROR = -2,		//!< General connection/server error
	RLM_SQL_OK = 0,			//!< Success
	RLM_SQL_RECONNECT = 1,		//!< Stale connection, should reconnect
	RLM_SQL_DUPLICATE = 2		//!< Key constraint violation
} sql_rcode_t;

typedef enum {
	FALL_THROUGH_DEFAULT = 0,
	FALL_THROUGH_YES,
	FALL_THROUGH_NO
} sql_fall_through_t;


typedef char **rlm_sql_row_t;

/*
 * Sections where we dynamically resolve the config entry to use,
 * by xlating reference.
 */
typedef struct sql_acct_section {
	CONF_SECTION		*cs;				//!< The CONF_SECTION representing the group
								//!< of queries to process.

	char const		*reference;			//!< Reference string, expanded to point to
								//!< a group of queries.
	bool			reference_cp;

	char const		*logfile;

	char const		*query;	/* for xlat parsing */
} sql_acct_section_t;

typedef struct sql_config {
	char const 		*xlat_name;			//!< Module instance name.

	char const 		*sql_driver_name;		//!< SQL driver module name e.g. rlm_sql_sqlite.
	char const 		*sql_server;			//!< Server to connect to.
	char const 		*sql_port;			//!< Port to connect to.
	char const 		*sql_login;			//!< Login credentials to use.
	char const 		*sql_password;			//!< Login password to use.
	char const 		*sql_db;			//!< Database to run queries against.

	char const		*query_user;			//!< xlat expansion used to specify the user
								//!< to use as the subject of queries.

	char const		*default_profile;		//!< Default profile to use if no other
								//!< profiles were configured.

	char const		*client_query;			//!< Query used to get FreeRADIUS client
								//!< definitions.

	char const		*authorize_check_query;		//!< Query used get check VPs for a user.
	char const 		*authorize_reply_query;		//!< Query used get reply VPs for a user.
	char const		*authorize_group_check_query;	//!< Query used get check VPs for a group.
	char const		*authorize_group_reply_query;	//!< Query used get reply VPs for a group.
	char const		*simul_count_query;		//!< Query used get number of active sessions
								//!< for a user (basic simultaneous use check).
	char const		*simul_verify_query;		//!< Query to get active sessions for a user
								//!< the result is fed to session_zap.
	char const 		*groupmemb_query;		//!< Query to determine group membership.

	bool			do_clients;			//!< Read clients from SQL database.
	bool			read_groups;			//!< Read user groups by default.
								//!< If false, Fall-Through = yes is required
								//!< in the previous reply list to process
								//!< groups.
	bool			read_profiles;			//!< Read user profiles by default.
								//!< If false, Fall-Through = yes is required
								//!< in the previous reply list to process
								//!< profiles.
	char const		*logfile;			//!< Keep a log of all SQL queries executed
								//!< Useful for batch insertion with the
								//!< NULL drivers.

	bool			delete_stale_sessions;		//!< Whether we should use session_zap to create
								//!< a fake stop packet, to terminate any
								//!< stale sessions.

	char const		*allowed_chars;			//!< Chars which done need escaping..
	uint32_t		query_timeout;			//!< How long to allow queries to run for.

	char const		*connect_query;			//!< Query executed after establishing
								//!< new connection.
	struct timeval		connect_timeout_tv;		//!< Connection timeout timeval.
	uint32_t		connect_timeout_ms;		//!< Connection timeout ms.
	uint32_t		connect_timeout_s;		//!< Connection timeout in seconds.

	void			*driver;			//!< Where drivers should write a
								//!< pointer to their configurations.

	/*
	 *	@todo The rest of the queries should also be moved into
	 *	their own sections.
	 */

	/*
	 *	Section configurations
	 */
	sql_acct_section_t	postauth;
	sql_acct_section_t	accounting;
} rlm_sql_config_t;

typedef struct sql_inst rlm_sql_t;

typedef struct rlm_sql_handle {
	void			*conn;				//!< Database specific connection handle.
	rlm_sql_row_t		row;				//!< Row data from the last query.
	rlm_sql_t		*inst;				//!< The rlm_sql instance this connection belongs to.
} rlm_sql_handle_t;

typedef struct rlm_sql_module_t {
	char const *name;

	sql_rcode_t (*mod_instantiate)(CONF_SECTION *conf, rlm_sql_config_t *config);
	sql_rcode_t (*sql_socket_init)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);

	sql_rcode_t (*sql_query)(rlm_sql_handle_t *handle, rlm_sql_config_t *config, char const *query);
	sql_rcode_t (*sql_select_query)(rlm_sql_handle_t *handle, rlm_sql_config_t *config, char const *query);
	sql_rcode_t (*sql_store_result)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);

	int (*sql_num_fields)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);
	int (*sql_num_rows)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);
	int (*sql_affected_rows)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);

	sql_rcode_t (*sql_fetch_row)(rlm_sql_row_t *out, rlm_sql_handle_t *handle, rlm_sql_config_t *config);
	sql_rcode_t (*sql_fields)(char const **out[], rlm_sql_handle_t *handle, rlm_sql_config_t *config);
	sql_rcode_t (*sql_free_result)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);

	char const *(*sql_error)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);

	sql_rcode_t (*sql_finish_query)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);
	sql_rcode_t (*sql_finish_select_query)(rlm_sql_handle_t *handle, rlm_sql_config_t *config);
} rlm_sql_module_t;

struct sql_inst {
	rlm_sql_config_t	myconfig; /* HACK */
	fr_connection_pool_t	*pool;
	rlm_sql_config_t	*config;
	CONF_SECTION		*cs;

	DICT_ATTR const		*sql_user;		//!< Cached pointer to SQL-User-Name
							//!< dictionary attribute.
	exfile_t		*ef;

	void *handle;
	rlm_sql_module_t *module;

	int (*sql_set_user)(rlm_sql_t *inst, REQUEST *request, char const *username);
	size_t (*sql_escape_func)(REQUEST *, char *out, size_t outlen, char const *in, void *arg);
	sql_rcode_t (*sql_query)(rlm_sql_handle_t **handle, rlm_sql_t *inst, char const *query);
	sql_rcode_t (*sql_select_query)(rlm_sql_handle_t **handle, rlm_sql_t *inst, char const *query);
	sql_rcode_t (*sql_fetch_row)(rlm_sql_row_t *out, rlm_sql_handle_t **handle, rlm_sql_t *inst);
};

typedef struct sql_grouplist {
	char			*name;
	struct sql_grouplist	*next;
} rlm_sql_grouplist_t;

void		*mod_conn_create(TALLOC_CTX *ctx, void *instance);
int		sql_userparse(TALLOC_CTX *ctx, VALUE_PAIR **first_pair, rlm_sql_row_t row);
int		sql_read_realms(rlm_sql_handle_t *handle);
int		sql_getvpdata(TALLOC_CTX *ctx, rlm_sql_t *inst, REQUEST *request, rlm_sql_handle_t **handle, VALUE_PAIR **pair, char const *query);
int		sql_read_naslist(rlm_sql_handle_t *handle);
int		sql_read_clients(rlm_sql_handle_t *handle);
int		sql_dict_init(rlm_sql_handle_t *handle);
void 		CC_HINT(nonnull (1, 2, 4)) rlm_sql_query_log(rlm_sql_t *inst, REQUEST *request, sql_acct_section_t *section, char const *query);
sql_rcode_t	CC_HINT(nonnull) rlm_sql_select_query(rlm_sql_handle_t **handle, rlm_sql_t *inst, char const *query);
sql_rcode_t	CC_HINT(nonnull) rlm_sql_query(rlm_sql_handle_t **handle, rlm_sql_t *inst, char const *query);
sql_rcode_t 	rlm_sql_fetch_row(rlm_sql_row_t *out, rlm_sql_handle_t **handle, rlm_sql_t *inst);
int		sql_set_user(rlm_sql_t *inst, REQUEST *request, char const *username);
#endif
