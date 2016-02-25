

#include "ev_record.h"
#include "logging.h"
#include "fmd_event.h"

// insert

static int ras_store_cpu_event(fmd_event_t *pevt);
static int ras_store_mem_event(fmd_event_t *pevt);
static int ras_store_disk_event(fmd_event_t *pevt);

void time2string(time_t *t_time, char *pTime);

#if 0
static int ras_store_xfs_event(struct cmea_evt_data *edata, void *data);
static int ras_store_ipmi_event(struct cmea_evt_data *edata, void *data);
static int ras_store_mpio_event(struct cmea_evt_data *edata, void *data);
static int ras_store_fuse_event(struct cmea_evt_data *edata, void *data);
static int ras_store_network_event(struct cmea_evt_data *edata, void *data);
#endif 

static struct event_handler {
	char *event_type;
	int (*store_handler)(fmd_event_t *pevent);
}g_store_handler[] = {
	{"cpu", 		ras_store_cpu_event },
    {"mem",         ras_store_mem_event }, 
    {"disk",        ras_store_disk_event }, 
    #if 0
    {"xfs",         ras_store_xfs_event }, 
    {"ipmi",        ras_store_ipmi_event }, 
    {"mpio",        ras_store_mpio_event }, 
    {"fuse",        ras_store_fuse_event }, 
    {"network",     ras_store_network_event },
    #endif
	{NULL, 			NULL}
};

struct sqlite3_priv *priv = NULL;

void time2string(time_t *t_time, char *pTime)
{
    int year=0,mon=0,day=0,hour=0,min=0,sec=0;
    struct tm *m_tm;
    
    //m_tm = gmtime((const time_t *)t_time);localtime
    m_tm = localtime((const time_t *)t_time);
    year = m_tm->tm_year+1900;
    mon = m_tm->tm_mon+1;
    day = m_tm->tm_mday;
    hour = m_tm->tm_hour;
    min = m_tm->tm_min;
    sec = m_tm->tm_sec;
    
    sprintf(pTime, "%04d-%02d-%02d %d:%d:%d", year, mon, day, hour, min, sec);
    return;
}

static int ras_prepare_stmt(sqlite3_stmt **stmt,
                                const struct db_table_descriptor *db_tab)
{
	int i, rc;
	char sql[1024], *p = sql, *end = sql + sizeof(sql);
	const struct db_fields *field;

	p += snprintf(p, end - p, "INSERT INTO %s (",
		      db_tab->name);

	for (i = 0; i < db_tab->num_fields; i++) {
		field = &db_tab->fields[i];
		p += snprintf(p, end - p, "%s", field->name);

		if (i < db_tab->num_fields - 1)
			p += snprintf(p, end - p, ", ");
	}

	p += snprintf(p, end - p, ") VALUES ( NULL, ");

	for (i = 1; i < db_tab->num_fields; i++) {
		if (i <  db_tab->num_fields - 1)
			strcat(sql, "?, ");
		else
			strcat(sql, "?)");
	}

	wr_log("", WR_LOG_DEBUG, "SQL: %s\n", sql);

	rc = sqlite3_prepare_v2(priv->db, sql, -1, stmt, NULL);
	if (rc != SQLITE_OK) {
		wr_log("", WR_LOG_ERROR,
		    "Failed to prepare insert db at table %s (db %s): error = %s\n",
		    db_tab->name, SQLITE_RAS_DB, sqlite3_errmsg(priv->db));
		stmt = NULL;
	} else {
		wr_log("", WR_LOG_DEBUG, "Recording %s events\n", db_tab->name);
	}

	return rc;
}


static int rasdb_create_table(const struct db_table_descriptor *db_tab)
{
	const struct db_fields *field;
 	char sql[1024] = {0};
    char *p = sql, *end = sql + sizeof(sql);
 	int i,rc;

    memset(sql, 0, sizeof(sql));
    
	p += snprintf(p, end - p, "CREATE TABLE IF NOT EXISTS %s (",
		      db_tab->name);
    
	for (i = 0; i < db_tab->num_fields; i++) {
		field = &db_tab->fields[i];
		p += snprintf(p, end - p, "%s %s", field->name, field->type);

		if (i < db_tab->num_fields - 1)
			p += snprintf(p, end - p, ", ");
	}
	p += snprintf(p, end - p, ")");

	wr_log("", WR_LOG_DEBUG, "SQL: %s\n", sql);

	rc = sqlite3_exec(priv->db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		wr_log("", WR_LOG_ERROR,
		    "Failed to create table %s on %s: error = %d\n", db_tab->name, SQLITE_RAS_DB, rc);
	}
	return rc;
}


// open db and create tables.
int ras_event_opendb()
{
	int rc;
	sqlite3 *db;

	printf("Calling %s()\n", __FUNCTION__);

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		return -1;
    
	rc = sqlite3_initialize();
	if (rc != SQLITE_OK) {
		wr_log("", WR_LOG_ERROR,
		    "cpu: Failed to initialize sqlite: error = %d\n",
		     rc);
		return -1;
	}

	do {
		rc = sqlite3_open_v2(SQLITE_RAS_DB, &db,
				     SQLITE_OPEN_FULLMUTEX |
				     SQLITE_OPEN_READWRITE |
				     SQLITE_OPEN_CREATE, NULL);
		if (rc == SQLITE_BUSY)
			usleep(10000);
	} while (rc == SQLITE_BUSY);

	priv->db = db;
    
	if (rc != SQLITE_OK) {
		wr_log("", WR_LOG_ERROR,
		    "cpu: Failed to connect to %s: error = %d\n",
		    SQLITE_RAS_DB, rc);
		return -1;
	}
    
	rc = rasdb_create_table(&cpu_event_tab);
	if (rc == SQLITE_OK)
		rc = ras_prepare_stmt(&priv->stmt_cpu_event, &cpu_event_tab);

	rc = rasdb_create_table(&mem_event_tab);
	if (rc == SQLITE_OK)
		rc = ras_prepare_stmt(&priv->stmt_mem_event, &mem_event_tab);

	rc = rasdb_create_table(&disk_event_tab);
	if (rc == SQLITE_OK)
		rc = ras_prepare_stmt(&priv->stmt_disk_event, &disk_event_tab);

    /* create repaired and not repaired events */
	rc = rasdb_create_table(&rep_record_tab);
	if (rc == SQLITE_OK)
		rc = ras_prepare_stmt(&priv->stmt_rep_record, &rep_record_tab);

	rc = rasdb_create_table(&norep_record_tab);
	if (rc == SQLITE_OK)
		rc = ras_prepare_stmt(&priv->stmt_norep_record, &norep_record_tab);

	return 0;
}

int ras_store_norep_event(fmd_event_t *event)
{
	int rc;
    char time1[64] = {0};
    char time2[64] = {0};
    char time3[64] = {0};

	if (!priv || !priv->stmt_disk_event)
		return 0;
    
	wr_log("", WR_LOG_DEBUG, "disk event store: %p\n", priv->stmt_disk_event);
    
    time2string(&event->repaired_T, time1);
    time2string(&event->ev_create, time2);
    time2string(&event->ev_last_occur, time3);

	sqlite3_bind_text(priv->stmt_rep_record,  1, event->dev_name, -1, NULL);
        
	sqlite3_bind_text(priv->stmt_rep_record,  2, time1, -1, NULL);
    sqlite3_bind_text(priv->stmt_rep_record,  3, time2, -1, NULL);
    sqlite3_bind_text(priv->stmt_rep_record,  4, time3, -1, NULL);
    
	sqlite3_bind_int(priv->stmt_rep_record,  5, event->event_type);
	sqlite3_bind_text (priv->stmt_rep_record,  6, event->ev_class, -1, NULL);
	sqlite3_bind_text (priv->stmt_rep_record,  7, "offline", -1, NULL);
	sqlite3_bind_int(priv->stmt_rep_record,  8, event->ev_count);

	sqlite3_bind_int(priv->stmt_rep_record,  9, event->dev_id);
    sqlite3_bind_int(priv->stmt_rep_record,  10, event->evt_id);

	rc = sqlite3_step(priv->stmt_rep_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed to do rep event step on sqlite: error = %d\n", rc);
    
	rc = sqlite3_reset(priv->stmt_rep_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed reset rep event on sqlite: error = %d\n", rc);
    
	wr_log("", WR_LOG_DEBUG, "register inserted at db\n");

	return rc;
}


int ras_store_rep_event(fmd_event_t *event)
{
	int rc;
    char time1[64] = {0};
    char time2[64] = {0};
    char time3[64] = {0};

	if (!priv || !priv->stmt_rep_record)
		return 0;
    
	wr_log("", WR_LOG_DEBUG, "disk event store: %p\n", priv->stmt_rep_record);
    
    time2string(&event->repaired_T, time1);
    time2string(&event->ev_create, time2);
    time2string(&event->ev_last_occur, time3);

	sqlite3_bind_text(priv->stmt_rep_record,  1, event->dev_name, -1, NULL);
        
	sqlite3_bind_text(priv->stmt_rep_record,  2, time1, -1, NULL);
    sqlite3_bind_text(priv->stmt_rep_record,  3, time2, -1, NULL);
    sqlite3_bind_text(priv->stmt_rep_record,  4, time3, -1, NULL);
    
	sqlite3_bind_int(priv->stmt_rep_record,  5, event->event_type);
	sqlite3_bind_text (priv->stmt_rep_record,  6, event->ev_class, -1, NULL);
	sqlite3_bind_text (priv->stmt_rep_record,  7, "offline", -1, NULL);
	sqlite3_bind_int(priv->stmt_rep_record,  8, event->ev_count);

	sqlite3_bind_int(priv->stmt_rep_record,  9, event->dev_id);
    sqlite3_bind_int(priv->stmt_rep_record,  10, event->evt_id);

	rc = sqlite3_step(priv->stmt_rep_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed to do rep event step on sqlite: error = %d\n", rc);
    
	rc = sqlite3_reset(priv->stmt_rep_record);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed reset rep event on sqlite: error = %d\n", rc);
    
	wr_log("", WR_LOG_DEBUG, "register inserted at db\n");

	return rc;
}

static int ras_store_disk_event(fmd_event_t *event)
{
	int rc;
    char time[64] = {0};
    
	if (!priv || !priv->stmt_disk_event)
		return 0;
    
	wr_log("", WR_LOG_DEBUG, "disk event store: %p\n", priv->stmt_disk_event);
    
    time2string(&event->ev_create, time);

	sqlite3_bind_text(priv->stmt_disk_event,  1, time, -1, NULL);
	sqlite3_bind_text (priv->stmt_disk_event,  2, event->ev_class, -1, NULL);

	sqlite3_bind_int(priv->stmt_disk_event,  3, event->event_type);
	sqlite3_bind_int(priv->stmt_disk_event,  4, event->dev_id);
    sqlite3_bind_int(priv->stmt_disk_event,  5, event->evt_id);
    
	sqlite3_bind_text (priv->stmt_disk_event,  6, event->data, -1, NULL);

    if(event->event_type == EVENT_SERD)
	    sqlite3_bind_text(priv->stmt_disk_event,  7, "Minor", -1, NULL);
    else if(event->event_type == EVENT_FAULT)
	    sqlite3_bind_text(priv->stmt_disk_event,  7, "Major", -1, NULL);

	rc = sqlite3_step(priv->stmt_disk_event);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed to do disk event step on sqlite: error = %d\n", rc);
    
	rc = sqlite3_reset(priv->stmt_disk_event);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed reset disk event on sqlite: error = %d\n", rc);
    
	wr_log("", WR_LOG_DEBUG, "register inserted at db\n");

	return rc;
}


// ras events store to sqlite
int ras_store_mem_event(fmd_event_t *event)
{
	int rc;
    char time[64] = {0};
    
	if (!priv || !priv->stmt_mem_event)
		return 0;
    
	wr_log("", WR_LOG_DEBUG, "mem event store: %p\n", priv->stmt_mem_event);
    
    time2string(&event->ev_create, time);

	sqlite3_bind_text(priv->stmt_mem_event,  1, time, -1, NULL);
	sqlite3_bind_text (priv->stmt_mem_event,  2, event->ev_class, -1, NULL);

	sqlite3_bind_int(priv->stmt_mem_event,  3, event->event_type);
	sqlite3_bind_int(priv->stmt_mem_event,  4, event->dev_id);
    sqlite3_bind_int(priv->stmt_mem_event,  5, event->evt_id);
    
	sqlite3_bind_text (priv->stmt_mem_event,  6, event->data, -1, NULL);

    if(event->event_type == EVENT_SERD)
	    sqlite3_bind_text(priv->stmt_mem_event,  7, "Minor", -1, NULL);
    else if(event->event_type == EVENT_FAULT)
	    sqlite3_bind_text(priv->stmt_mem_event,  7, "Major", -1, NULL);

	rc = sqlite3_step(priv->stmt_mem_event);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed to do aer_event step on sqlite: error = %d\n", rc);
    
	rc = sqlite3_reset(priv->stmt_mem_event);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed reset aer_event on sqlite: error = %d\n", rc);
    
	wr_log("", WR_LOG_DEBUG, "register inserted at db\n");

	return rc;
}


//mc memcache 
int ras_store_cpu_event(fmd_event_t *event)
{
	int rc;
    char time[64] = {0};

	if (!priv || !priv->stmt_cpu_event)
		return 0;
    
	wr_log("", WR_LOG_DEBUG, "mc_event store: %p\n", priv->stmt_cpu_event);

    time2string(&event->ev_create, time);

	sqlite3_bind_text(priv->stmt_cpu_event,  1, time, -1, NULL);
	sqlite3_bind_text (priv->stmt_cpu_event,  2, event->ev_class, -1, NULL);
    
	sqlite3_bind_int(priv->stmt_cpu_event,  3, event->event_type);
	sqlite3_bind_int(priv->stmt_cpu_event,  4, event->dev_id);
    sqlite3_bind_int(priv->stmt_cpu_event,  5, event->evt_id);

	sqlite3_bind_text(priv->stmt_cpu_event,  6, event->data, -1, NULL);

    if(event->event_type == EVENT_SERD)
	    sqlite3_bind_text(priv->stmt_cpu_event,  7, "Minor", -1, NULL);
    else if(event->event_type == EVENT_FAULT)
	    sqlite3_bind_text(priv->stmt_cpu_event,  7, "Major", -1, NULL);

	rc = sqlite3_step(priv->stmt_cpu_event);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed to do mc_event step on sqlite: error = %d\n", rc);
    
	rc = sqlite3_reset(priv->stmt_cpu_event);
	if (rc != SQLITE_OK && rc != SQLITE_DONE)
		wr_log("", WR_LOG_ERROR,
		    "Failed reset mc_event on sqlite: error = %d\n", rc);
    
	wr_log("", WR_LOG_DEBUG, "register inserted at db\n");

	return rc;
    
}


// there is a bug. wanghuan
int ras_events_handle(fmd_event_t *event)
{
    int i;
	for (i = 0; g_store_handler[i].event_type; i++) {
		char *name = g_store_handler[i].event_type;
		if (strncmp(name, event->dev_name, strlen(name)) == 0) {
			wr_log("", WR_LOG_DEBUG, "handle dev store to DB.[%s]", name);
			return g_store_handler[i].store_handler(event);
		}
	}
    wr_log("", WR_LOG_ERROR, "failed to find event store handler.");
    return -1;
}
