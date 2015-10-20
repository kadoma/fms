/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    libfmd_msg.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: message mode
 
 * FMD Message Library
 *
 * This library supports a simple set of routines for use in converting FMA
 * events and message codes to localized human-readable message strings.
 *
 * 1. Library API
 *
 * The APIs are as follows:
 *
 * fmd_msg_init - set up the library and return a handle
 * fmd_msg_fini - destroy the handle from fmd_msg_init
 *
 * fmd_msg_locale_set - set the default locale (initially based on environ(5))
 * fmd_msg_locale_get - get the default locale
 *
 * fmd_msg_url_set - set the default URL for knowledge articles
 * fmd_msg_url_get - get the default URL for knowledge articles
 *
 * fmd_msg_gettext_evt - format the entire message for the given event
 * fmd_msg_gettext_id  - format the entire message for the given event code
 *
 * fmd_msg_getitem_evt - format a single message item for the given event
 * fmd_msg_getitem_id  - format a single message item for the given event code
 *
 * Upon success, fmd_msg_gettext_* and fmd_msg_getitem_* return newly-allocated
 * localized strings in multi-byte format.  The caller must call free() on the
 * resulting buffer to deallocate the string after making use of it.  Upon
 * failure, these functions return NULL and set errno as follows:
 *
 * ENOMEM - Memory allocation failure while formatting message
 * ENOENT - No message was found for the specified message identifier
 * EINVAL - Invalid argument (e.g. bad event code, illegal fmd_msg_item_t)
 * EILSEQ - Illegal multi-byte sequence detected in message
 *
 * 2. Variable Expansion
 *
 * The human-readable messages are stored in msgfmt(1) message object files in
 * the corresponding locale directories.  The values for the message items are
 * permitted to contain variable expansions, currently defined as follows:
 *
 * For example, the msgstr value for FMD-8000-2K might be defined as:
 *
 * msgid "FMD-8000-2K.action"
 * msgstr "Use fmdump -v -u %<uuid> to locate the module.  Use fmadm \
 *     reset %<fault-list[0].asru.mod-name> to reset the module."
 *
 * 3. Locking
 *
 * In order to format a human-readable message, libfmd_msg must get and set
 * the process locale and potentially alter text domain bindings.  At present,
 * these facilities in libc are not fully MT-safe.  As such, a library-wide
 * lock is provided: fmd_msg_lock() and fmd_msg_unlock().  These locking calls
 * are made internally as part of the top-level library entry points, but they
 * can also be used by applications that themselves call setlocale() and wish
 * to appropriately synchronize with other threads that are calling libfmd_msg.
 ************************************************************/


#include <libintl.h>
#include <locale.h>
#include <wchar.h>

#include <alloca.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <fmd_msg.h>

struct fmd_msg_hdl {
	int fmh_version;	/* libfmd_msg client abi version number */
	char *fmh_urlbase;	/* base url for all knowledge articles */
	char *fmh_binding;	/* base directory for bindtextdomain() */
	char *fmh_locale;	/* default program locale from environment */
	const char *fmh_template; /* FMD_MSG_TEMPLATE value for fmh_locale */
};

static const char *const fmd_msg_items[] = {
	"class",		/* key for FMD_MSG_ITEM_CLASS */
	"type",			/* key for FMD_MSG_ITEM_TYPE */
	"severity",		/* key for FMD_MSG_ITEM_SEVERITY */
	"description",		/* key for FMD_MSG_ITEM_DESC */
	"response",		/* key for FMD_MSG_ITEM_RESPONSE */
	"impact", 		/* key for FMD_MSG_ITEM_IMPACT */
	"action", 		/* key for FMD_MSG_ITEM_ACTION */
	"url",			/* key for FMD_MSG_ITEM_URL */
};

static pthread_rwlock_t fmd_msg_rwlock = PTHREAD_RWLOCK_INITIALIZER;

static const char FMD_MSG_DOMAIN[] = "FMD";
static const char FMD_MSG_TEMPLATE[] = "syslog-msgs-message-template";
static const char FMD_MSG_URLBASE[] = "http://sun.com/msg/";
static const char FMD_MSG_BINDINGBASE[] = "/usr/lib/fms/dict/";
static const char FMD_MSG_MISSING[] = "-";

static const char *const fmd_msg_etype_items[] = {
    "cpu",                 /* key for FMD_MSG_ITEM_ECLASS_GMCA */
    ".io.scsi.disk.",    /* key for FMD_MSG_ITEM_ECLASS_SCSI */
    ".io.ata.disk.",    /* key for FMD_MSG_ITEM_ECLASS_ATA */
    ".io.sata.disk.",    /* key for FMD_MSG_ITEM_ECLASS_SATA */
    ".ipmi.",        /* key for FMD_MSG_ITEM_ECLASS_IPMI */
    ".io.mpio.",        /* key for FMD_MSG_ITEM_ECLASS_MPIO */
    ".io.network.",        /* key for FMD_MSG_ITEM_ECLASS_NETWORK */
    ".io.pcie.",        /* key for FMD_MSG_ITEM_ECLASS_PCIE */
    ".service.",        /* key for FMD_MSG_ITEM_ECLASS_SERVICE */
    ".topo.",        /* key for FMD_MSG_ITEM_ECLASS_TOPO */
};

void
format_msg_date(char *buf, time_t secs)
{
	struct tm *tm = NULL;
	int year = 0, month = 0, day = 0;
	int hour = 0, min = 0, sec = 0;

	time(&secs);
	tm = gmtime(&secs);
	year = tm->tm_year + 1900;
	month = tm->tm_mon + 1;
	day = tm->tm_mday;
	hour = tm->tm_hour;
	min = tm->tm_min;
	sec = tm->tm_sec;
	sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
}

void
fmd_msg_lock(void)
{
	if (pthread_rwlock_wrlock(&fmd_msg_rwlock) != 0)
		abort();
}

void
fmd_msg_unlock(void)
{
	if (pthread_rwlock_unlock(&fmd_msg_rwlock) != 0)
		abort();
}

static fmd_msg_hdl_t *
fmd_msg_init_err(fmd_msg_hdl_t *h, int err)
{
	fmd_msg_fini(h);
	errno = err;
	return (NULL);
}

fmd_msg_hdl_t *
fmd_msg_init(int version)
{
	fmd_msg_hdl_t *h = NULL;
	const char *s;
	//size_t len;

	if (version != FMD_MSG_VERSION)
		return (fmd_msg_init_err(h, EINVAL));

	if ((h = malloc(sizeof (fmd_msg_hdl_t))) == NULL)
		return (fmd_msg_init_err(h, ENOMEM));

	bzero(h, sizeof (fmd_msg_hdl_t));
	h->fmh_version = version;

	if ((h->fmh_urlbase = strdup(FMD_MSG_URLBASE)) == NULL)
		return (fmd_msg_init_err(h, ENOMEM));

	/*
	 * Initialize the program's locale from the environment if it hasn't
	 * already been initialized, and then retrieve the default setting.
	 */
	(void) setlocale(LC_ALL, "");
	s = setlocale(LC_ALL, NULL);
	h->fmh_locale = strdup(s ? s : "C");

	if (h->fmh_locale == NULL)
		return (fmd_msg_init_err(h, ENOMEM));

	/*
	 * Use the default root directory /usr/lib/fm/dict, and set fmh_binding
	 * as the same directory prefixed with the root directory.
	 */
	h->fmh_binding = malloc(sizeof(char) * sizeof(FMD_MSG_BINDINGBASE));
	(void) sprintf(h->fmh_binding, "%s", FMD_MSG_BINDINGBASE);

	if (h->fmh_binding == NULL)
		return (fmd_msg_init_err(h, ENOMEM));

	bindtextdomain(FMD_MSG_DOMAIN, FMD_MSG_BINDINGBASE);

	/*
	 * Cache the message template for the current locale.  This is the
	 * snprintf(3C) format string for the final human-readable message.
	 */
	h->fmh_template = dgettext(FMD_MSG_DOMAIN, FMD_MSG_TEMPLATE);

	return (h);
}

void
fmd_msg_fini(fmd_msg_hdl_t *h)
{
	if (h == NULL)
		return; /* simplify caller code */

	free(h->fmh_binding);
	free(h->fmh_urlbase);
	free(h->fmh_locale);
	free(h);
}

int
fmd_msg_locale_set(fmd_msg_hdl_t *h, const char *locale)
{
	char *l;

	if (locale == NULL) {
		errno = EINVAL;
		return (-1);
	}

	if ((l = strdup(locale)) == NULL) {
		errno = ENOMEM;
		return (-1);
	}

	fmd_msg_lock();

	if (setlocale(LC_ALL, l) == NULL) {
		free(l);
		errno = EINVAL;
		fmd_msg_unlock();
		return (-1);
	}

	h->fmh_template = dgettext(FMD_MSG_DOMAIN, FMD_MSG_TEMPLATE);
	free(h->fmh_locale);
	h->fmh_locale = l;

	fmd_msg_unlock();
	return (0);
}

const char *
fmd_msg_locale_get(fmd_msg_hdl_t *h)
{
	return (h->fmh_locale);
}

int
fmd_msg_url_set(fmd_msg_hdl_t *h, const char *url)
{
	char *u;

	if (url == NULL) {
		errno = EINVAL;
		return (-1);
	}

	if ((u = strdup(url)) == NULL) {
		errno = ENOMEM;
		return (-1);
	}

	fmd_msg_lock();

	free(h->fmh_urlbase);
	h->fmh_urlbase = u;

	fmd_msg_unlock();
	return (0);
}

const char *
fmd_msg_url_get(fmd_msg_hdl_t *h)
{
	return (h->fmh_urlbase);
}

static char *
fmd_msg_etype_lookup(const char *eclass)
{
	int i;

	for (i = 0; i < FMD_MSG_ITEM_ECLASS_MAX; i++) {
		if (strstr(eclass, fmd_msg_etype_items[i]) != NULL) {
			if (i == FMD_MSG_ITEM_ECLASS_GMCA)
				return ("GMCA");
			else if (i == FMD_MSG_ITEM_ECLASS_SCSI ||
				 i == FMD_MSG_ITEM_ECLASS_ATA  ||
				 i == FMD_MSG_ITEM_ECLASS_SATA)
				return ("DISK");
			else if (i == FMD_MSG_ITEM_ECLASS_IPMI)
				return ("IPMI");
			else if (i == FMD_MSG_ITEM_ECLASS_MPIO)
				return ("MPIO");
			else if (i == FMD_MSG_ITEM_ECLASS_NETWORK)
				return ("NETWORK");
			else if (i == FMD_MSG_ITEM_ECLASS_PCIE)
				return ("PCIE");
			else if (i == FMD_MSG_ITEM_ECLASS_SERVICE)
				return ("SERVICE");
			else if (i == FMD_MSG_ITEM_ECLASS_TOPO)
				return ("TOPO");
			else
				return (NULL);
		}
	}

	return (NULL);
}

/*
 * This function is the main engine for formatting an event message item, such
 * as the Description field.  It loads the item text from a message object,
 * expands any variables defined in the item text, and then returns a newly-
 * allocated multi-byte string with the localized message text, or NULL with
 * errno set if an error occurred.
 */
static char *
fmd_msg_getitem_locked(fmd_msg_hdl_t *h,
    const char *dict, const char *code, fmd_msg_item_t item)
{
	const char *istr = fmd_msg_items[item];
	char *txt;
	const char *url = h->fmh_urlbase;
	size_t len = strlen(code) + 1 + strlen(istr) + 1;
	char *key = alloca(len);

//	assert(fmd_msg_lock_held(h));

	/*
	 * If the item is FMD_MSG_ITEM_URL, then its value is directly computed
	 * as the URL base concatenated with the code.  Otherwise the item text
	 * is derived by looking up the key <code>.<istr> in the dict object.
	 * Once we're done, convert the 'txt' multi-byte to wide-character.
	 */
	if (item == FMD_MSG_ITEM_URL) {
		len = strlen(url) + strlen(code) + 1;
		key = alloca(len);
		(void) snprintf(key, len, "%s%s", url, code);
		txt = key;
	} else {
		len = strlen(code) + 1 + strlen(istr) + 1;
		key = alloca(len);
		(void) snprintf(key, len, "%s.%s", code, istr);
		txt = dgettext(dict, key);
	}

	return (txt);
}

/*
 * This function is the main engine for formatting an entire event message.
 * It retrieves the master format string for an event, formats the individual
 * items, and then produces the final string composing all of the items.  The
 * result is a newly-allocated multi-byte string of the localized message
 * text, or NULL with errno set if an error occurred.
 */
static char *
fmd_msg_gettext_locked(fmd_msg_hdl_t *h, const char *dict, const char *code)
{
	char *items[FMD_MSG_ITEM_MAX];
	const char *format = h->fmh_template;
	char *buf = NULL;
	char *uuid = NULL;
	size_t len;
	int i;

	time_t sec = 0;
	char date[64];

//	assert(fmd_msg_lock_held(h));
	bzero(items, sizeof (items));

	for (i = 0; i < FMD_MSG_ITEM_MAX; i++) {
		items[i] = fmd_msg_getitem_locked(h, dict, code, i);
		if (items[i] == NULL)
			goto out;
	}

//	if (nvlist_lookup_string(nvl, FM_SUSPECT_UUID, &uuid) != 0)
	uuid = (char *)FMD_MSG_MISSING;		/* FIXME uuid= "-" */

	/* Get the times */
	format_msg_date(date, sec);

	/*
	 * Format the message once to get its length, allocate a buffer, and
	 * then format the message again into the buffer to return it.
	 */
	len = snprintf(NULL, 0, format, date, uuid, code,
	    items[FMD_MSG_ITEM_CLASS], items[FMD_MSG_ITEM_TYPE],
	    items[FMD_MSG_ITEM_SEVERITY], items[FMD_MSG_ITEM_DESC],
	    items[FMD_MSG_ITEM_RESPONSE], items[FMD_MSG_ITEM_IMPACT],
	    items[FMD_MSG_ITEM_ACTION]);

	if ((buf = malloc(len + 1)) == NULL) {
		errno = ENOMEM;
		goto out;
	}

	(void) snprintf(buf, len + 1, format, date, uuid, code,
	    items[FMD_MSG_ITEM_CLASS], items[FMD_MSG_ITEM_TYPE],
	    items[FMD_MSG_ITEM_SEVERITY], items[FMD_MSG_ITEM_DESC],
	    items[FMD_MSG_ITEM_RESPONSE], items[FMD_MSG_ITEM_IMPACT],
	    items[FMD_MSG_ITEM_ACTION]);
out:
	for (i = 0; i < FMD_MSG_ITEM_MAX; i++)
		free(items[i]);

	return (buf);
}

/*
 * Common code for fmd_msg_getitem_evt() and fmd_msg_getitem_id(): this function
 * handles locking, changing locales and domains, and restoring i18n state.
 */
static char *
fmd_msg_getitem(fmd_msg_hdl_t *h,
    const char *eclass, const char *code, fmd_msg_item_t item)
{
	char *dict = NULL, *ecode = NULL;
	char *p = NULL, *s = NULL;
	int err;

	if (code != NULL) {
		if ((p = strchr(code, '-')) == NULL || p == code) {
			errno = EINVAL;
			return (NULL);
		}

		dict = alloca((size_t)(p - code) + 1);
		(void) strncpy(dict, code, (size_t)(p - code));
		dict[(size_t)(p - code)] = '\0';	/* eg: GMCA */
	} else if (eclass != NULL)
		dict = fmd_msg_etype_lookup(eclass);

	fmd_msg_lock();

	/*
	 * If a non-default text domain binding was requested, save the old
	 * binding perform the re-bind now that fmd_msg_lock() is held.
	 */
	if (h->fmh_binding != NULL) {
		(void) bindtextdomain(dict, h->fmh_binding);
	}

	if (code != NULL) {
		s = fmd_msg_getitem_locked(h, dict, code, item);
		err = errno;
	} else if (eclass != NULL && code == NULL) {
		ecode = dgettext(dict, eclass);
		s = fmd_msg_getitem_locked(h, dict, ecode, item);
		err = errno;
	}

	fmd_msg_unlock();

	if (s == NULL)
		errno = err;

	return (s);
}

char *
fmd_msg_getid_evt(fmd_msg_hdl_t *h, const char *eclass)
{
	char *dict, *s;
	int err;
	dict = fmd_msg_etype_lookup(eclass);

	fmd_msg_lock();

	/*
	 * If a non-default text domain binding was requested, save the old
	 * binding perform the re-bind now that fmd_msg_lock() is held.
	 */
	if (h->fmh_binding != NULL) {
		(void) bindtextdomain(dict, h->fmh_binding);
	}

	s = dgettext(dict, eclass);
	err = errno;

	fmd_msg_unlock();

	if (s == NULL)
		errno = err;

	return (s);
}

char *
fmd_msg_getitem_evt(fmd_msg_hdl_t *h,
    const char *eclass, fmd_msg_item_t item)
{
	if (item >= FMD_MSG_ITEM_MAX) {
		errno = EINVAL;
		return (NULL);
	}

	return (fmd_msg_getitem(h, eclass, NULL, item));
}

char *
fmd_msg_getitem_id(fmd_msg_hdl_t *h,
    const char *code, fmd_msg_item_t item)
{
	if (item >= FMD_MSG_ITEM_MAX) {
		errno = EINVAL;
		return (NULL);
	}

	return (fmd_msg_getitem(h, NULL, code, item));
}

/*
 * Common code for fmd_msg_gettext_evt() and fmd_msg_gettext_id(): this function
 * handles locking and restoring i18n state.
 */
static char *
fmd_msg_gettext(fmd_msg_hdl_t *h,
    const char *eclass, const char *code)
{
	char *dict, *ecode, *key, *p, *s;
	size_t len;
	int err;

	if (code != NULL) {
		if ((p = strchr(code, '-')) == NULL || p == code) {
			errno = EINVAL;
			return (NULL);
		}

		dict = alloca((size_t)(p - code) + 1);
		(void) strncpy(dict, code, (size_t)(p - code));
		dict[(size_t)(p - code)] = '\0';	/* eg: GMCA */
	} else if (eclass != NULL)
		dict = fmd_msg_etype_lookup(eclass);

	fmd_msg_lock();

	/*
	 * If a non-default text domain binding was requested, save the old
	 * binding perform the re-bind now that fmd_msg_lock() is held.
	 */
	if (h->fmh_binding != NULL) {
		(void) bindtextdomain(dict, h->fmh_binding);
	}

	/*
	 * Compute the lookup code for FMD_MSG_ITEM_CLASS: we'll use this to
	 * determine if the dictionary contains any data for this code at all.
	 */
	len = strlen(code) + 1 + strlen(fmd_msg_items[FMD_MSG_ITEM_CLASS]) + 1;
	key = alloca(len);

	(void) snprintf(key, len, "%s.%s",
	    code, fmd_msg_items[FMD_MSG_ITEM_CLASS]);

	if (dgettext(dict, key) == key) { /* Check out if translation is successfull */
		s = NULL;
		err = ENOENT;
	} else if (code != NULL) {
		s = fmd_msg_gettext_locked(h, dict, code);
		err = errno;
	} else if (eclass != NULL && code == NULL) {
		ecode = dgettext(dict, eclass);
		s = fmd_msg_gettext_locked(h, dict, ecode);
		err = errno;
	}

	fmd_msg_unlock();

	if (s == NULL)
		errno = err;

	return (s);
}

char *
fmd_msg_gettext_evt(fmd_msg_hdl_t *h, const char *eclass)
{
	return (fmd_msg_gettext(h, eclass, NULL));
}

char *
fmd_msg_gettext_id(fmd_msg_hdl_t *h, const char *code)
{
	return (fmd_msg_gettext(h, NULL, code));
}

