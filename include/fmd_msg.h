#ifndef	_FMD_MSG_H
#define	_FMD_MSG_H 1

#include <sys/types.h>

/*
 * Fault Management Daemon msg File Interfaces
 *
 * Note: The contents of this file are private to the implementation of the
 * Solaris system and FMD subsystem and are subject to change at any time
 * without notice.  Applications and drivers using these interfaces will fail
 * to run on future releases.  These interfaces should not be used for any
 * purpose until they are publicly documented for use outside of Sun.
 */

#define	FMD_MSG_VERSION	1	/* libary ABI interface version */

typedef struct fmd_msg_hdl fmd_msg_hdl_t;

typedef enum {
	FMD_MSG_ITEM_CLASS,
	FMD_MSG_ITEM_TYPE,
	FMD_MSG_ITEM_SEVERITY,
	FMD_MSG_ITEM_DESC,
	FMD_MSG_ITEM_RESPONSE,
	FMD_MSG_ITEM_IMPACT,
	FMD_MSG_ITEM_ACTION,
	FMD_MSG_ITEM_URL,
	FMD_MSG_ITEM_MAX
} fmd_msg_item_t;

typedef enum {
	FMD_MSG_ITEM_ECLASS_GMCA,
	FMD_MSG_ITEM_ECLASS_SCSI,
	FMD_MSG_ITEM_ECLASS_ATA,
	FMD_MSG_ITEM_ECLASS_SATA,
	FMD_MSG_ITEM_ECLASS_IPMI,
	FMD_MSG_ITEM_ECLASS_MPIO,
	FMD_MSG_ITEM_ECLASS_NETWORK,
	FMD_MSG_ITEM_ECLASS_PCIE,
	FMD_MSG_ITEM_ECLASS_SERVICE,
	FMD_MSG_ITEM_ECLASS_TOPO,
	FMD_MSG_ITEM_ECLASS_MAX
} fmd_msg_etype_item_t;

extern void fmd_msg_lock(void);
extern void fmd_msg_unlock(void);

fmd_msg_hdl_t *fmd_msg_init(int);
void fmd_msg_fini(fmd_msg_hdl_t *);

extern int fmd_msg_locale_set(fmd_msg_hdl_t *, const char *);
extern const char *fmd_msg_locale_get(fmd_msg_hdl_t *);

extern int fmd_msg_url_set(fmd_msg_hdl_t *, const char *);
extern const char *fmd_msg_url_get(fmd_msg_hdl_t *);

extern char *fmd_msg_gettext_evt(fmd_msg_hdl_t *, const char *);
extern char *fmd_msg_gettext_id(fmd_msg_hdl_t *, const char *);

extern char *fmd_msg_getitem_evt(fmd_msg_hdl_t *,
    const char *, fmd_msg_item_t);

extern char *fmd_msg_getitem_id(fmd_msg_hdl_t *,
    const char *, fmd_msg_item_t);

extern char *fmd_msg_getid_evt(fmd_msg_hdl_t *, const char *);

#endif	/* _FMD_MSG_H */

