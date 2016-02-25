#ifndef __EV_RECORD__
#define __EV_RECORD__ 1

#include "wrap.h"
#include <unistd.h>
#include <sqlite3.h>
#include "logging.h"
#include "fmd_event.h"

#define SQLITE_RAS_DB "/lib/fms/db/faults.db"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

struct db_fields {
	char *name;
	char *type;
};

struct db_table_descriptor {
	char			*name;
	const struct db_fields	*fields;
	size_t			num_fields;
};

// 8 counts
static const struct db_fields dev_event_fields[] = {
		{ .name="id",			.type="INTEGER PRIMARY KEY" },
		{ .name="timestamp",	.type="TEXT" },
		{ .name="err_msg",		.type="TEXT" },
		{ .name="err_type",		.type="TEXT" },
		{ .name="dev_id",		.type="INTEGER" },
		{ .name="evt_id",		.type="INTEGER" },
		{ .name="detail",		.type="TEXT" },
		{ .name="evt_level",	.type="TEXT" },
};

static const struct db_table_descriptor cpu_event_tab = {
	.name = "cpu_event",
	.fields = dev_event_fields,
	.num_fields = ARRAY_SIZE(dev_event_fields),
};

static const struct db_table_descriptor mem_event_tab = {
	.name = "mem_event",
	.fields = dev_event_fields,
	.num_fields = ARRAY_SIZE(dev_event_fields),
};

static const struct db_table_descriptor disk_event_tab = {
	.name = "disk_event",
	.fields = dev_event_fields,
	.num_fields = ARRAY_SIZE(dev_event_fields),
};

// rep event table
static const struct db_fields rep_record_fields[] = {
		{ .name="id",			.type="INTEGER PRIMARY KEY" },
		{ .name="device_name",	.type="TEXT" },            
		{ .name="timestamp_rep",	.type="TEXT" },
		{ .name="timestamp_1",	.type="TEXT" },
		{ .name="timestamp_2",	.type="TEXT" },
		{ .name="err_type",		.type="TEXT" },
		{ .name="err_msg",		.type="TEXT" },
		{ .name="action",		.type="TEXT" },
		{ .name="err_count",	.type="INTEGER" },
		{ .name="dev_id",		.type="INTEGER" },
		{ .name="evt_id",		.type="INTEGER" },
};

static const struct db_table_descriptor rep_record_tab = {
	.name = "rep_event",
	.fields = rep_record_fields,
	.num_fields = ARRAY_SIZE(rep_record_fields),
};

static const struct db_table_descriptor norep_record_tab = {
	.name = "norep_event",
	.fields = rep_record_fields,
	.num_fields = ARRAY_SIZE(rep_record_fields),
};
    
struct sqlite3_priv {
	sqlite3		*db;
	sqlite3_stmt	*stmt_cpu_event;
	sqlite3_stmt	*stmt_mem_event;
    sqlite3_stmt	*stmt_disk_event;
	sqlite3_stmt	*stmt_rep_record;
	sqlite3_stmt	*stmt_norep_record;
};

int ras_event_opendb();
int ras_events_handle(fmd_event_t *event);

int ras_store_norep_event(fmd_event_t *pevt);
int ras_store_rep_event(fmd_event_t *pevt);

#endif  // ev_record.h
