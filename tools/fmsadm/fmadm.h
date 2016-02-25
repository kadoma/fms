/*
 * fmadm.h
 *
 */

#ifndef    _FMADM_H
#define    _FMADM_H

#include <fmd_adm.h>
#include <stdio.h>

#define    FMADM_EXIT_SUCCESS    0
#define    FMADM_EXIT_ERROR    1
#define    FMADM_EXIT_USAGE    2

extern void note(const char *format, ...);
extern void warn(const char *format, ...);
extern void die(const char *format, ...);

extern int cmd_config(fmd_adm_t *, int, char *[]);
extern int cmd_modinfo(fmd_adm_t *, int, char *[]);
extern int cmd_faulty(fmd_adm_t *, int, char *[]);
//extern int cmd_flush(fmd_adm_t *, int, char *[]);
//extern int cmd_gc(fmd_adm_t *, int, char *[]);
extern int cmd_load(fmd_adm_t *, int, char *[]);
//extern int cmd_repair(fmd_adm_t *, int, char *[]);
//extern int cmd_repaired(fmd_adm_t *, int, char *[]);
//extern int cmd_replaced(fmd_adm_t *, int, char *[]);
//extern int cmd_acquit(fmd_adm_t *, int, char *[]);
//extern int cmd_reset(fmd_adm_t *, int, char *[]);
//extern int cmd_rotate(fmd_adm_t *, int, char *[]);
extern int cmd_unload(fmd_adm_t *, int, char *[]);
extern int is_digit(const char*);
extern int check_esc(char* ,char*);
extern int check_sub_rep(char* ,char*);
extern int check_sub_legal(const char *);

#endif    /* _FMADM_H */

