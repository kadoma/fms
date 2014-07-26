/*
 * fmd_ckpt.c
 *
 *  Created on: Sep 19, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *      fmd_ckpt.c
 */

#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <assert.h>

#include <fmd.h>
#include <fmd_ckpt.h>
#include <fmd_case.h>
#include <fmd_serd.h>
#include <fmd_dispq.h>
#include <fmd_event.h>
#include <fmd_module.h>
#include <fmd_list.h>
#include <fmd_hash.h>
#include <fmd_log.h>

/* global reference */
extern fmd_t fmd;

#define	IS_P2ALIGNED(v, a)	((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)

/*
 * The fmd_ckpt_t structure is used to manage all of the state needed by the
 * various subroutines that save and restore checkpoints.  The structure is
 * initialized using fmd_ckpt_create() or fmd_ckpt_open() and is destroyed
 * by fmd_ckpt_destroy().  Refer to the subroutines below for more details.
 */
typedef struct fmd_ckpt {
	char ckp_src[PATH_MAX];	/* ckpt input or output filename */
	uint8_t *ckp_buf;	/* data buffer base address */
	fcf_hdr_t *ckp_hdr;	/* file header pointer */
	uint8_t *ckp_ptr;	/* data buffer pointer */
	uint64_t ckp_size;	/* data buffer size */
	fcf_sec_t *ckp_secp;	/* section header table pointer */
	uint8_t ckp_secs;	/* number of sections */
	int ckp_fd;		/* output descriptor */
} fmd_ckpt_t;

static int
fmd_ckpt_create(fmd_ckpt_t *ckp)
{
	char *filename = NULL;
	int fd;
	filename = ckp->ckp_src;

	ckp->ckp_size = sizeof (fcf_hdr_t);

	(void) unlink(ckp->ckp_src);
	if ((fd = open(filename, O_RDWR | O_CREAT | O_SYNC | O_APPEND, S_IREAD | S_IWRITE)) < 0) {
		printf("FMD: failed to create ckpt file %s\n", filename);
		return (-1);
	}
	
	ckp->ckp_fd = fd;

	return (ckp->ckp_fd);
}

/*PRINTFLIKE2*/
static int
fmd_ckpt_inval(fmd_ckpt_t *ckp, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	va_end(ap);

	free(ckp->ckp_buf);
	return (-1);
}

static int
fmd_ckpt_open(fmd_ckpt_t *ckp)
{
//	struct stat64 st;
	struct stat st;
	uint64_t seclen;
	uint8_t i;
	int err;

	bzero(ckp, sizeof (fmd_ckpt_t));

	(void) snprintf(ckp->ckp_src, PATH_MAX, "/usr/lib/fm/ckpt/now/ckpt.cpt");

	if (access(ckp->ckp_src, 0) != 0) {
		printf("FMD: have no checkpoint yet.\n");
		return (-1);
	}

	if ((ckp->ckp_fd = open(ckp->ckp_src, O_RDONLY)) == -1) {
		printf("FMD: failed to open checkpoint file %s\n", ckp->ckp_src);
		return (-1); /* failed to open checkpoint file */
	}

//	if (fstat64(ckp->ckp_fd, &st) == -1) {
	if (fstat(ckp->ckp_fd, &st) == -1) {
		err = errno;
		(void) close(ckp->ckp_fd);
		printf("FMD: ckpt file %d fstat is error.\n", ckp->ckp_fd);
		return (-1);
	}

	ckp->ckp_buf = malloc(st.st_size);
	assert(ckp->ckp_buf != NULL);
	memset(ckp->ckp_buf, 0, st.st_size);
	ckp->ckp_hdr = (void *)ckp->ckp_buf;
	ckp->ckp_size = read(ckp->ckp_fd, ckp->ckp_buf, st.st_size);

	if (ckp->ckp_size != st.st_size || ckp->ckp_size < sizeof (fcf_hdr_t) ||
	    ckp->ckp_size != ckp->ckp_hdr->fcfh_filesz) {
		free(ckp->ckp_buf);
		(void) close(ckp->ckp_fd);
		printf("FMD: ckpt file %s is error.\n", ckp->ckp_src);
		return (-1);
	}

	(void) close(ckp->ckp_fd);
	ckp->ckp_fd = -1;

	/*
	 * Once we've read in a consistent copy of the FCF file and we're sure
	 * the header can be accessed, go through it and make sure everything
	 * is valid.  We also check that unused bits are zero so we can expand
	 * to use them safely in the future and support old files if needed.
	 */
	if (bcmp(&ckp->ckp_hdr->fcfh_ident[FCF_ID_MAG0],
	    FCF_MAG_STRING, FCF_MAG_STRLEN) != 0)
		return (fmd_ckpt_inval(ckp, "FMD: bad checkpoint magic string\n"));

	if (ckp->ckp_hdr->fcfh_ident[FCF_ID_MODEL] != FCF_MODEL_NATIVE)
		return (fmd_ckpt_inval(ckp, "FMD: bad checkpoint data model\n"));

	if (ckp->ckp_hdr->fcfh_ident[FCF_ID_ENCODING] != FCF_ENCODE_NATIVE)
		return (fmd_ckpt_inval(ckp, "FMD: bad checkpoint data encoding\n"));

	if (ckp->ckp_hdr->fcfh_ident[FCF_ID_VERSION] != FCF_VERSION_1) {
		return (fmd_ckpt_inval(ckp, "FMD: bad checkpoint version %u\n",
		    ckp->ckp_hdr->fcfh_ident[FCF_ID_VERSION]));
	}

	for (i = FCF_ID_PAD; i < FCF_ID_SIZE; i++) {
		if (ckp->ckp_hdr->fcfh_ident[i] != 0) {
			return (fmd_ckpt_inval(ckp,
			    "FMD: bad checkpoint padding at id[%d]", i));
		}
	}

	if (ckp->ckp_hdr->fcfh_flags & ~FCF_FL_VALID)
		return (fmd_ckpt_inval(ckp, "FMD: bad checkpoint flags\n"));

	if (ckp->ckp_hdr->fcfh_pad != 0)
		return (fmd_ckpt_inval(ckp, "FMD: reserved field in use\n"));

	if (ckp->ckp_hdr->fcfh_hdrsize < sizeof (fcf_hdr_t) ||
	    ckp->ckp_hdr->fcfh_secsize < sizeof (fcf_sec_t)) {
		return (fmd_ckpt_inval(ckp,
		    "FMD: bad header and/or section size\n"));
	}

	seclen = (uint64_t)ckp->ckp_hdr->fcfh_secnum *
	    (uint64_t)ckp->ckp_hdr->fcfh_secsize;

	if (ckp->ckp_hdr->fcfh_secoff > ckp->ckp_size ||
	    seclen > ckp->ckp_size ||
	    ckp->ckp_hdr->fcfh_secoff + seclen > ckp->ckp_size ||
	    ckp->ckp_hdr->fcfh_secoff + seclen < ckp->ckp_hdr->fcfh_secoff)
		return (fmd_ckpt_inval(ckp, "FMD: truncated section headers\n"));

	if (!IS_P2ALIGNED(ckp->ckp_hdr->fcfh_secoff, sizeof (uint64_t)) ||
	    !IS_P2ALIGNED(ckp->ckp_hdr->fcfh_secsize, sizeof (uint64_t)))
		return (fmd_ckpt_inval(ckp, "FMD: misaligned section headers\n"));

	/*
	 * Once the header is validated, iterate over the section headers
	 * ensuring that each one is valid w.r.t. offset, alignment, and size.
	 * We also pick up the string table pointer during this pass.
	 */
	ckp->ckp_secp = (void *)(ckp->ckp_buf + ckp->ckp_hdr->fcfh_secoff);
	ckp->ckp_secs = ckp->ckp_hdr->fcfh_secnum;

#if 0
	for (i = 0; i < ckp->ckp_secs; i++) {
		fcf_sec_t *sp = (void *)(ckp->ckp_buf +
		    ckp->ckp_hdr->fcfh_secoff + ckp->ckp_hdr->fcfh_secsize * i);

		const fmd_ckpt_desc_t *dp = &_fmd_ckpt_sections[sp->fcfs_type];

		if (sp->fcfs_flags != 0) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has invalid "
			    "section flags (0x%x)\n", i, sp->fcfs_flags));
		}

		if (sp->fcfs_align & (sp->fcfs_align - 1)) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has invalid "
			    "alignment (%u)\n", i, sp->fcfs_align));
		}

		if (sp->fcfs_offset & (sp->fcfs_align - 1)) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u is not properly"
			    " aligned (offset %llu)\n", i, sp->fcfs_offset));
		}

		if (sp->fcfs_entsize != 0 &&
		    (sp->fcfs_entsize & (sp->fcfs_align - 1)) != 0) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has misaligned "
			    "entsize %u\n", i, sp->fcfs_entsize));
		}

		if (sp->fcfs_offset > ckp->ckp_size ||
		    sp->fcfs_size > ckp->ckp_size ||
		    sp->fcfs_offset + sp->fcfs_size > ckp->ckp_size ||
		    sp->fcfs_offset + sp->fcfs_size < sp->fcfs_offset) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has corrupt "
			    "size or offset\n", i));
		}

		if (sp->fcfs_type >= sizeof (_fmd_ckpt_sections) /
		    sizeof (_fmd_ckpt_sections[0])) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has unknown "
			    "section type %u\n", i, sp->fcfs_type));
		}

		if (sp->fcfs_align != dp->secd_align) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has align %u "
			    "(not %u)\n", i, sp->fcfs_align, dp->secd_align));
		}

		if (sp->fcfs_size < dp->secd_size ||
		    sp->fcfs_entsize < dp->secd_entsize) {
			return (fmd_ckpt_inval(ckp, "FMD: section %u has short "
			    "size or entsize\n", i));
		}

		switch (sp->fcfs_type) {
		case FCF_SECT_STRTAB:
			if (ckp->ckp_strs != NULL) {
				return (fmd_ckpt_inval(ckp, "FMD: multiple string "
				    "tables are present in checkpoint file\n"));
			}

			ckp->ckp_strs = (char *)ckp->ckp_buf + sp->fcfs_offset;
			ckp->ckp_strn = sp->fcfs_size;

			if (ckp->ckp_strs[ckp->ckp_strn - 1] != '\0') {
				return (fmd_ckpt_inval(ckp, "FMD: string table %u "
				    "is missing terminating nul byte\n", i));
			}
			break;

		case FCF_SECT_MODULE:
			if (ckp->ckp_modp != NULL) {
				return (fmd_ckpt_inval(ckp, "FMD: multiple module "
				    "sects are present in checkpoint file\n"));
			}
			ckp->ckp_modp = sp;
			break;
		}
	}
#endif

	/*
	 * Ensure that the first section is an empty one of type FCF_SECT_NONE.
	 * This is done to ensure that links can use index 0 as a null section.
	 */
	if (ckp->ckp_secs == 0 || ckp->ckp_secp->fcfs_type > FCF_SECT_DISPQ ||
	    ckp->ckp_secp->fcfs_size == 0) {
		return (fmd_ckpt_inval(ckp, "FMD: section 0 is not of the "
		    "appropriate size and/or attributes (SECT_NONE)\n"));
	}

	if (ckp->ckp_src == NULL) {
		return (fmd_ckpt_inval(ckp,
		    "FMD: no ckpt file is found\n"));
	}

	return (0);
}

static void
fmd_ckpt_destroy(fmd_ckpt_t *ckp)
{
	if (ckp->ckp_buf != NULL)
		free(ckp->ckp_buf);
	if (ckp->ckp_fd >= 0)
		(void) close(ckp->ckp_fd);
}

/*
 * fmd_ckpt_error() is used as a wrapper around fmd_error() for ckpt routines.
 * It calls fmd_module_unlock() on behalf of its caller, logs the error, and
 * then aborts the API call and the surrounding module entry point by doing an
 * fmd_module_abort(), which longjmps to the place where we entered the module.
 * Depending on the type of error and conf settings, we will reset or fail.
 */
/*PRINTFLIKE3*/
static void
fmd_ckpt_error(fmd_ckpt_t *ckp, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	va_end(ap);

	fmd_ckpt_destroy(ckp);
}

static void
fmd_ckpt_section_header(fmd_ckpt_t *ckp, fcf_sec_t *fcfs)
{
	uint64_t size;
	size = sizeof(fcf_sec_t);

	fcfs->fcfs_offset = (uint64_t)(ckp->ckp_ptr - ckp->ckp_buf);
	fcfs->fcfs_size = size;		/* step add */

	if (fcfs != NULL) {
		bcopy(fcfs, ckp->ckp_ptr, size);
		ckp->ckp_ptr += size;
	}
}

static void
fmd_ckpt_section_data(fmd_ckpt_t *ckp, const void *data, uint64_t size)
{
	ckp->ckp_secp->fcfs_size += size;

	/*
	 * If the data pointer is non-NULL, copy the data to our buffer; else
	 * the caller is responsible for doing so and updating ckp->ckp_ptr.
	 */
	if (data != NULL) {
		bcopy(data, ckp->ckp_ptr, size);
		ckp->ckp_ptr += size;
	}
}

static int
fmd_ckpt_alloc(fmd_ckpt_t *ckp)
{
	/*
	 * Allocate memory for FCF.
	 */
	ckp->ckp_buf = malloc(ckp->ckp_size);
	assert(ckp->ckp_buf != NULL);
	memset(ckp->ckp_buf, 0, ckp->ckp_size);

	if (ckp->ckp_buf == NULL)
		return (-1); /* errno is set for us */

	ckp->ckp_hdr = (void *)ckp->ckp_buf;

	ckp->ckp_hdr->fcfh_ident[FCF_ID_MAG0] = FCF_MAG_MAG0;
	ckp->ckp_hdr->fcfh_ident[FCF_ID_MAG1] = FCF_MAG_MAG1;
	ckp->ckp_hdr->fcfh_ident[FCF_ID_MAG2] = FCF_MAG_MAG2;
	ckp->ckp_hdr->fcfh_ident[FCF_ID_MAG3] = FCF_MAG_MAG3;
	ckp->ckp_hdr->fcfh_ident[FCF_ID_MODEL] = FCF_MODEL_NATIVE;
	ckp->ckp_hdr->fcfh_ident[FCF_ID_ENCODING] = FCF_ENCODE_NATIVE;
	ckp->ckp_hdr->fcfh_ident[FCF_ID_VERSION] = FCF_VERSION;

	ckp->ckp_hdr->fcfh_hdrsize = sizeof (fcf_hdr_t);
	ckp->ckp_hdr->fcfh_secsize = sizeof (fcf_sec_t);
	ckp->ckp_hdr->fcfh_secnum = ckp->ckp_secs;
	ckp->ckp_hdr->fcfh_secoff = sizeof (fcf_hdr_t);
	ckp->ckp_hdr->fcfh_filesz = ckp->ckp_size;
//	ckp->ckp_hdr->fcfh_cgen = gen;

	ckp->ckp_secs = 0; /* reset section counter for second pass */
	ckp->ckp_secp = (void *)(ckp->ckp_buf + sizeof (fcf_hdr_t));
	ckp->ckp_ptr = (uint8_t *)(ckp->ckp_secp);

	return (0);
}

static int
copy_file(char *src, char *dst)
{
	int from_fd, to_fd;
	int bytes_read, bytes_write;
	char buffer[3];
	char *ptr;

	/* open the raw file */
	/* open file readonly, return -1 if error; else fd. */
	if ((from_fd = open(src, O_RDONLY)) == -1) {
		fprintf(stderr, "Open %s Error:%s\n", src, strerror(errno));
		return (-1);
	}   

	(void) unlink(dst);
	/* create the target file */
	if ((to_fd = open(dst, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
		fprintf(stderr,"Open %s Error:%s\n", dst, strerror(errno));
		return (-1);
	}

	/* a classic copy file code as follow */
	while (bytes_read = read(from_fd, buffer, 3)) {
		/* a fatal error */
		if ((bytes_read == -1) && (errno != EINTR))
			break;
		else if (bytes_read > 0) {
			ptr = buffer;
			while (bytes_write = write(to_fd,ptr,bytes_read)) {
				/* a fatal error */
				if ((bytes_write == -1) && (errno != EINTR))
					break;
				/* read bytes have been written */
				else if (bytes_write == bytes_read)
					break;
				/* only write a part, continue to write */
				else if (bytes_write > 0) {
					ptr += bytes_write;
					bytes_read -= bytes_write;
				}
			}
			/* fatal error when writting */
			if (bytes_write == -1)
				break;
		}
	}
	close(from_fd);
	close(to_fd);

	return 0;
}

/*
 * Copy the checkpoint file to /now directory for special use.
 */
int
fmd_ckpt_rename(char *src)
{
	char dir[PATH_MAX], dst[PATH_MAX];

	(void) sprintf(dir, "/usr/lib/fm/ckpt/now");
	(void) sprintf(dst, "%s/ckpt.cpt", dir);

	if (access(dir, 0) != 0) {
		printf("FMD: FCF file %s rename failed.\n", src);
		return (-1);
	}

	if (copy_file(src, dst) != 0 && errno != ENOENT) {
		printf("FMD: failed to rename %s\n", src);
		return (-1);
	}

	return 0;
}

static int
fmd_ckpt_commit(fmd_ckpt_t *ckp)
{
	fcf_sec_t *secbase = (void *)(ckp->ckp_buf + sizeof (fcf_hdr_t));
	uint64_t size = ckp->ckp_size;

	/*
	 * Before committing the checkpoint, we assert that fmd_ckpt_t's sizes
	 * and current pointer locations all add up appropriately.  Any ASSERTs
	 * which trip here likely indicate an inconsistency in the code for the
	 * reservation pass and the buffer update pass of the FCF subroutines.
	 */
	assert((uint64_t)(ckp->ckp_ptr - ckp->ckp_buf) == size);
	assert(ckp->ckp_secs == ckp->ckp_hdr->fcfh_secnum);
	assert(ckp->ckp_secp == secbase + ckp->ckp_hdr->fcfh_secnum);
	assert(ckp->ckp_ptr == ckp->ckp_buf + ckp->ckp_hdr->fcfh_filesz);

	if (write(ckp->ckp_fd, ckp->ckp_buf, ckp->ckp_size) != ckp->ckp_size ||
	    fsync(ckp->ckp_fd) != 0 || close(ckp->ckp_fd) != 0)
		return (-1); /* errno is set for us */

	ckp->ckp_fd = -1; /* fd is now closed */
	return (fmd_ckpt_rename(ckp->ckp_src) != 0);
}

static void
fmd_ckpt_save_event(fmd_ckpt_t *ckp, fmd_event_t *evt)
{
	uint64_t size = sizeof (fcf_event_t);
	fcf_event_t *fcfe = (void *)ckp->ckp_ptr;

	fcfe->fcfe_ev_create = (uint64_t)evt->ev_create;
	fcfe->fcfe_ev_refs = evt->ev_refs;
	fcfe->fcfe_ev_eid = evt->ev_eid;
	fcfe->fcfe_ev_uuid = evt->ev_uuid;
	fcfe->fcfe_ev_rscid = evt->ev_rscid;
	snprintf(fcfe->fcfe_ev_class, 50, "%s", evt->ev_class);

	ckp->ckp_ptr += size;
	ckp->ckp_secp->fcfs_size += size;
}

static void
fmd_ckpt_save_serd(fmd_ckpt_t *ckp, fcf_serd_t *fcfd, struct fmd_serd_type *ptype)
{
	fcfd->fcfd_sid = ptype->fmd_serdid;
	fcfd->fcfd_n = ptype->N;
	fcfd->fcfd_t = ptype->T;

	fmd_ckpt_section_data(ckp, fcfd, sizeof(fcf_serd_t));
}

static int
fmd_case_event_count(fmd_case_t *cp)
{
	struct list_head *pos = NULL;
	fmd_event_t *evt;
	int count = 0;

	list_for_each(pos, &cp->cs_event) {
		evt = list_entry(pos, fmd_event_t, ev_list);
		count++;
	}
	return count;
}

static void
fmd_ckpt_save_case(fmd_ckpt_t *ckp, fmd_case_t *cp)
{
	struct fmd_serd_type *ptype;
	struct fmd_hash *phash = &fmd.fmd_esc.hash_clsname;
	struct list_head *pos = NULL;
	fmd_event_t *evt;
	fcf_sec_t  fcfs;
	fcf_case_t fcfc;
	fcf_serd_t fcfd;
	uint64_t sid;
	int ev_num;

	memset(&fcfs, 0, sizeof(fcf_sec_t));
	memset(&fcfc, 0, sizeof(fcf_case_t));
	memset(&fcfd, 0, sizeof(fcf_serd_t));

	fcfc.fcfc_uuid = cp->cs_uuid;
	fcfc.fcfc_rscid = cp->cs_rscid;
	fcfc.fcfc_count = cp->cs_count;
	fcfc.fcfc_create = (uint64_t)cp->cs_create;
	fcfc.fcfc_fire = (uint64_t)cp->cs_fire;
	fcfc.fcfc_close = (uint64_t)cp->cs_close;
//	snprintf(&fcfc.fcfc_fault, 50, "%s", hash_get_key(phash, cp->cs_type->fault));
//	snprintf(&fcfc.fcfc_serd, 50, "%s", hash_get_key(phash, cp->cs_type->serd));
	snprintf(fcfc.fcfc_fault, 50, "%s", hash_get_key(phash, cp->cs_type->fault));
	snprintf(fcfc.fcfc_serd, 50, "%s", hash_get_key(phash, cp->cs_type->serd));

	switch (cp->cs_flag) {
	case CASE_CREATE:
		fcfc.fcfc_state = FCF_CASE_CREATE;
		break;
	case CASE_FIRED:
		fcfc.fcfc_state = FCF_CASE_FIRED;
		break;
	case CASE_CLOSED:
		fcfc.fcfc_state = FCF_CASE_CLOSED;
		break;
	default:
		printf("FMD: case %p (%d) has invalid state %u\n",
		  (void *)cp, cp->cs_uuid, cp->cs_flag);
	}

	fmd_ckpt_section_header(ckp, &fcfs);
	fmd_ckpt_section_data(ckp, &fcfc, sizeof(fcf_case_t));

	sid = cp->cs_type->serd;
	ptype = fmd_query_serd(sid);

	ev_num = fmd_case_event_count(cp);
	ckp->ckp_secp->fcfs_evtnum = ev_num;
	ckp->ckp_secp->fcfs_type = FCF_SECT_CASE_SERD;

	fmd_ckpt_save_serd(ckp, &fcfd, ptype);

	list_for_each(pos, &cp->cs_event) {
		evt = list_entry(pos, fmd_event_t, ev_list);
		fmd_ckpt_save_event(ckp, evt);
	}
}

static void
fmd_ckpt_save_dispq(fmd_ckpt_t *ckp)
{
	fmd_event_t *evt;
	ring_t *ring;
	fcf_sec_t fcfs;
	fcf_dispq_t fcfq;
	int dispq_evt = 0;
	int p = 0;

	memset(&fcfs, 0, sizeof(fcf_sec_t));
	memset(&fcfq, 0, sizeof(fcf_dispq_t));

	ring = &fmd.fmd_dispq.dq_ring;
	dispq_evt = ring->rear - ring->front;

	if (!ring_empty(ring)) {
		if (dispq_evt > 0) {
			fcfq.fcfq_ev_num = dispq_evt;
			ckp->ckp_secp->fcfs_evtnum = dispq_evt;
			ckp->ckp_secp->fcfs_type = FCF_SECT_DISPQ;
			ckp->ckp_secs++;

			fmd_ckpt_section_header(ckp, &fcfs);
			fmd_ckpt_section_data(ckp, &fcfq, sizeof(fcf_dispq_t));
			do {
				/* retrieve event */
				evt = (fmd_event_t *)ring->r_item[p];

				if(evt == NULL) {
					printf("FMD: dispq ckpt failed.\n");
					return;
				}
				/* ckpt event */
				fmd_ckpt_save_event(ckp, evt);

				p--;
			} while (--fcfq.fcfq_ev_num);
		}
	} else {
		printf("FMD: dispq has no events, no need to ckpt for it.\n");
		ckp->ckp_size = ckp->ckp_size -
			sizeof(fcf_sec_t) - sizeof(fcf_dispq_t);		/* reduce dispq sec bytes */
		ckp->ckp_hdr->fcfh_secnum = ckp->ckp_hdr->fcfh_secnum - 1;	/* reduce dispq sec count */
		ckp->ckp_hdr->fcfh_filesz = ckp->ckp_size;

		return;
	}
}

static void
fmd_ckpt_resv_module(fmd_ckpt_t *ckp)
{
	struct list_head *pos = NULL;
	struct fmd_serd_type *ptype = NULL;
	fmd_case_t *cp = NULL;
	ring_t *ring = &fmd.fmd_dispq.dq_ring;

	uint64_t cs_secs_size, dispq_sec_size;	/* exclude event */
	int cs_evt = 0, dispq_evt = 0, ev_total = 0, case_num = 0;
	int secs = 0;

	list_for_each(pos, &fmd.list_case) {
		cp = list_entry(pos, fmd_case_t, cs_list);

		cs_evt += fmd_case_event_count(cp);
		case_num++;
	}

	dispq_evt = ring->rear - ring->front;
	ev_total = cs_evt + dispq_evt;
	secs = case_num;		/* number of case & serd sections */
	cs_secs_size = secs * (sizeof(fcf_sec_t) + sizeof(fcf_case_t) + sizeof(fcf_serd_t));
	dispq_sec_size = sizeof(fcf_sec_t) + sizeof(fcf_dispq_t);

	ckp->ckp_secs = secs + 1;	/* case & serd sections + dispq section */
	ckp->ckp_size = sizeof(fcf_hdr_t) + cs_secs_size + dispq_sec_size + (ev_total) * sizeof(fcf_event_t);
}

static void
fmd_ckpt_save_module(fmd_ckpt_t *ckp)
{
	struct list_head *pos = NULL;
	fmd_case_t *cp;

	/* case & serd sections */
	list_for_each(pos, &fmd.list_case) {
		cp = list_entry(pos, fmd_case_t, cs_list);
		fmd_ckpt_save_case(ckp, cp);
		ckp->ckp_secp++;
		ckp->ckp_secs++;
	}

	/* dispq section */
	fmd_ckpt_save_dispq(ckp);
}

void
fmd_ckpt_save(void)
{
	fmd_ckpt_t ckp;
	pthread_mutex_t *mutex = &fmd.fmd_dispq.dq_ring.r_mutex;
	char date[20];
	char path[PATH_MAX];
	char *dir = "/usr/lib/fm/ckpt";
	time_t times;
	int err;

	memset(&ckp, 0, sizeof(fmd_ckpt_t));

	assert(fmd_module_lock(mutex));

	if (access(dir, 0) != 0) {
		printf("FMD: Checkpoint Directory %s isn't exist.\n", dir);
		printf("     failed to save Checkpoint.\n");
		return;
	}

	fmd_get_time(date, times);
	(void) snprintf(path, PATH_MAX, "%s/%s.cpt", dir, date);
	(void) snprintf(ckp.ckp_src, PATH_MAX, "%s", path);

	/*
	 * Create a temporary file to write out the checkpoint into, and create
	 * a fmd_ckpt_t structure to manage construction of the checkpoint.  We
	 * then figure out how much space will be required, and allocate it.
	 */
	if (fmd_ckpt_create(&ckp) == -1) {
		printf("FMD: failed to create %s\n", ckp.ckp_src);
		return;
	}

	fmd_ckpt_resv_module(&ckp);

//	if (fmd_ckpt_alloc(&ckp, mp->mod_gen + 1) != 0) {
	if (fmd_ckpt_alloc(&ckp) != 0) {
		printf("FMD: failed to build %s\n", ckp.ckp_src);
		fmd_ckpt_destroy(&ckp);
		return;
	}

	/*
	 * Fill in the checkpoint content, write it to disk, sync it, and then
	 * atomically rename it to the destination path.  If this fails, we
	 * have no choice but to leave all our dirty bits set and return.
	 */
	fmd_ckpt_save_module(&ckp);
	err = fmd_ckpt_commit(&ckp);
	fmd_ckpt_destroy(&ckp);

	if (err != 0) {
		printf("FMD: failed to commit %s\n", ckp.ckp_src);
		return;
	}

	assert(fmd_module_unlock(mutex));
	printf("FMD: saved checkpoint to %s\n", ckp.ckp_src);
}

/*
 * Utility function to retrieve the data pointer for a particular section.  The
 * validity of the header values has already been checked by fmd_ckpt_open().
 */
static const void *
fmd_ckpt_dataptr(fmd_ckpt_t *ckp, const fcf_sec_t *sp)
{
	return (ckp->ckp_buf + sp->fcfs_offset);
}

static void
fmd_ckpt_restore_events(fmd_ckpt_t *ckp, const fcf_sec_t *sp,
		void (*func)(void *, fmd_event_t *), void *arg)
{
	const fcf_event_t *fcfe;
	uint32_t evtnum = sp->fcfs_evtnum;
	int i;

	if (sp->fcfs_size == 0)
		return; /* empty events section or type none */

	if (sp->fcfs_type == FCF_SECT_CASE_SERD)
		fcfe = (void *)(fmd_ckpt_dataptr(ckp, sp) + ckp->ckp_hdr->fcfh_secsize
			+ sizeof (fcf_case_t) + sizeof (fcf_serd_t));
	else if (sp->fcfs_type == FCF_SECT_DISPQ)
		fcfe = (void *)(fmd_ckpt_dataptr(ckp, sp) + ckp->ckp_hdr->fcfh_secsize
			+ sizeof (fcf_dispq_t));

	for (i = 1; i < evtnum + 1; i++) {
		char eclass[50];
		fmd_event_t *ep = (fmd_event_t *)malloc(sizeof (fmd_event_t));
		assert(ep != NULL);

		memset(ep, 0, sizeof (fmd_event_t));
		memset(eclass, 0, 50 * sizeof (char));
		ep->ev_class = eclass;
		
		ep->ev_create = (time_t)fcfe->fcfe_ev_create;
		ep->ev_refs = fcfe->fcfe_ev_refs;
		ep->ev_eid = fcfe->fcfe_ev_eid;
		ep->ev_uuid = fcfe->fcfe_ev_uuid;
		ep->ev_rscid = fcfe->fcfe_ev_rscid;
		ep->ev_data = NULL;
		sprintf(ep->ev_class, "%s", fcfe->fcfe_ev_class);
		INIT_LIST_HEAD(&ep->ev_list);

//		fmd_event_hold(ep);
		func(arg, ep);
//		fmd_event_rele(ep);

		if (sp->fcfs_type == FCF_SECT_CASE_SERD)
			fcfe = (void *)(fmd_ckpt_dataptr(ckp, sp) + ckp->ckp_hdr->fcfh_secsize
				+ sizeof (fcf_case_t) + sizeof (fcf_serd_t)
			   	+ i * sizeof (fcf_event_t));
		else if (sp->fcfs_type == FCF_SECT_DISPQ)
			fcfe = (void *)(fmd_ckpt_dataptr(ckp, sp) + ckp->ckp_hdr->fcfh_secsize
				+ sizeof (fcf_dispq_t) + i * sizeof (fcf_event_t));
	}
}

static void 
fmd_ckpt_restore_serd(fmd_ckpt_t *ckp, const fcf_sec_t *sp, fmd_case_t *pcase)
{
	const fcf_serd_t *fcfd = (void *)(fmd_ckpt_dataptr(ckp, sp)
		   	+ ckp->ckp_hdr->fcfh_secsize + sizeof (fcf_case_t));
	struct fmd_case_type *pcs_type = pcase->cs_type;
	struct fmd_serd_type *pserd = fmd_query_serd(pcs_type->serd);

//	pserd->N = fcfd->fcfd_n;
//	pserd->T = fcfd->fcfd_t;
}

static void
fmd_ckpt_case_insert_event(fmd_case_t *cp, fmd_event_t *ep)
{
	list_add(&ep->ev_list, &cp->cs_event);
}

static void
fmd_ckpt_restore_case(fmd_ckpt_t *ckp, const fcf_sec_t *sp)
{
	const fcf_case_t *fcfc = (void *)(fmd_ckpt_dataptr(ckp, sp)
			+ ckp->ckp_hdr->fcfh_secsize);
	struct fmd_hash *phash = &fmd.fmd_esc.hash_clsname;
	struct list_head *pcstype = &fmd.fmd_esc.list_casetype;
	struct list_head *pcs = &fmd.list_case;
	struct list_head *pos = NULL;
	struct list_head *ppos = NULL;
	struct fmd_case_type *ptype = NULL;
	fmd_case_t *pcase = NULL;
	fmd_case_t *cp = NULL;
	char fault[50];
	char serd[50];
	uint64_t faultid;
	uint64_t serdid;

	memset(fault, 0, 50 * sizeof(char));
	memset(serd, 0, 50 * sizeof(char));

	if (fcfc->fcfc_uuid == (uint64_t)0 || fcfc->fcfc_state > FCF_CASE_CLOSED) {
		fmd_ckpt_error(ckp, "FMD: corrupt case uuid and/or state\n");
	}

//  fmd_module_lock(mp);

	pcase = (fmd_case_t *)malloc(sizeof (fmd_case_t));
	assert(pcase != NULL);
	memset(pcase, 0, sizeof(fmd_case_t));

	pcase->cs_uuid = fcfc->fcfc_uuid;
	pcase->cs_rscid = fcfc->fcfc_rscid;
	pcase->cs_count = fcfc->fcfc_count;
	pcase->cs_create = (time_t)fcfc->fcfc_create;
	pcase->cs_fire = (time_t)fcfc->fcfc_fire;
	pcase->cs_close = (time_t)fcfc->fcfc_close;
	INIT_LIST_HEAD(&pcase->cs_event);
	INIT_LIST_HEAD(&pcase->cs_list);

	switch (fcfc->fcfc_state) {
	case FCF_CASE_CREATE:
		pcase->cs_flag = CASE_CREATE;
		break;
	case FCF_CASE_FIRED:
		pcase->cs_flag = CASE_FIRED;
		break;
	case FCF_CASE_CLOSED:
		pcase->cs_flag = CASE_CLOSED;
		break;
	default:
		fmd_ckpt_error(ckp, "FMD: case %p (%d) has invalid state %u\n",
			(void *)pcase, pcase->cs_uuid, pcase->cs_flag);
	}

	sprintf(fault, "%s", fcfc->fcfc_fault);
	sprintf(serd, "%s", fcfc->fcfc_serd);

	faultid = hash_get(phash, fault);
	serdid = hash_get(phash, serd);

	list_for_each(pos, pcstype) {
		ptype = list_entry(pos, struct fmd_case_type, list);
		if (ptype->fault == faultid && ptype->serd == serdid) {
			pcase->cs_type = ptype;
			break;
		}
	}

	list_for_each(ppos, &fmd.list_case) {
		cp = list_entry(ppos, fmd_case_t, cs_list);
		if (cp->cs_uuid == pcase->cs_uuid) {
			fmd_ckpt_error(ckp, "FMD: duplicate case uuid: %s\n", pcase->cs_uuid);
			break;
		}
	}

	list_add(&pcase->cs_list, pcs);

	/* serd */
	fmd_ckpt_restore_serd(ckp, sp, pcase);

	/* events */
	fmd_ckpt_restore_events(ckp, sp,
		 (void (*)(void *, fmd_event_t *))fmd_ckpt_case_insert_event, pcase);

//	fmd_module_unlock(mp);
}

static void
fmd_ckpt_dispq_insert_event(ring_t *ring, fmd_event_t *ep)
{
	sem_wait(&ring->r_empty);
	pthread_mutex_lock(&ring->r_mutex);
	ring_add(ring, ep);
	pthread_mutex_unlock(&ring->r_mutex);
	sem_post(&ring->r_full);
}

static void
fmd_ckpt_restore_dispq(fmd_ckpt_t *ckp, const fcf_sec_t *sp)
{
	ring_t *ring = &fmd.fmd_dispq.dq_ring;

	/* events */
	fmd_ckpt_restore_events(ckp, sp,
		(void (*)(void *, fmd_event_t *))fmd_ckpt_dispq_insert_event, ring);
}

static void
fmd_ckpt_restore_module(fmd_ckpt_t *ckp)
{
//	const fcf_module_t *fcfm = fmd_ckpt_dataptr(ckp, ckp->ckp_modp);
	const fcf_sec_t *sp;
	uint8_t i;

	sp = (void *)(ckp->ckp_buf + ckp->ckp_hdr->fcfh_secoff);
	for (i = 0; i < ckp->ckp_secs; i++) {
		switch (sp->fcfs_type) {
		case FCF_SECT_CASE_SERD:
			fmd_ckpt_restore_case(ckp, sp);
			break;
		case FCF_SECT_DISPQ:
			fmd_ckpt_restore_dispq(ckp, sp);
			break;
		}
		sp = (void *)(ckp->ckp_buf + sp->fcfs_offset + sp->fcfs_size);
	}

//	mp->mod_gen = ckp->ckp_hdr->fcfh_cgen;
}

/*
 * Restore a checkpoint for the specified module.  Any errors which occur
 * during restore will call fmd_ckpt_error() or trigger an fmd_api_error(),
 * either of which will automatically unlock the module and trigger an abort.
 */
void
fmd_ckpt_restore(void)
{
	fmd_ckpt_t ckp;
	pthread_mutex_t *mutex = &fmd.fmd_dispq.dq_ring.r_mutex;

	memset(&ckp, 0, sizeof(fmd_ckpt_t));
	printf("FMD: ckpt restore begin.\n");

	if (fmd_ckpt_open(&ckp) == -1) {
		if (errno != ENOENT)
			printf("FMD: can't open %s\n", ckp.ckp_src);
		printf("FMD: ckpt restore end.\n");
		return;
	}

	if (ckp.ckp_hdr->fcfh_secnum == 0) {
		printf("FMD: no need to restore.\n");
		printf("FMD: ckpt restore end.\n");
		return;
	}

	assert(fmd_module_lock(mutex));

	fmd_ckpt_restore_module(&ckp);
	fmd_ckpt_destroy(&ckp);

	assert(fmd_module_unlock(mutex));

	printf("FMD: ckpt restore end.\n");
	printf("FMD: restored checkpoint of %s\n", ckp.ckp_src);
}

/*
 * Delete the module's checkpoint file.  This is used by the ckpt.zero property
 * code or by the fmadm reset RPC service path to force a checkpoint delete.
 */
void
fmd_ckpt_delete(char *path)
{
//	char path[PATH_MAX];

//	(void) snprintf(path, sizeof (path),
//	    "%s/%s", mp->mod_ckpt, mp->mod_name);

//	TRACE((FMD_DBG_CKPT, "delete %s ckpt", mp->mod_name));

	if (unlink(path) != 0 && errno != ENOENT)
		printf("FMD: failed to delete %s\n", path);
}

uint64_t
fmd_ckpt_conf(void)
{
	char buf[256];
	char buff[50];
	char filename[] = "/usr/lib/fm/ckpt/ckpt.conf";
	FILE *fp = NULL;
	int ret = 0;

	memset(buf, 0, 256 * sizeof (char));
	memset(buff, 0, 50 * sizeof (char));

	if ((fp = fopen(filename, "r+")) == NULL) {
		printf("FMD: failed to open conf file ckpt.conf\n");
		return (0);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		if ((buf[0] == '\n') || (buf[0] == '\0')
		 || (buf[0] == '#'))
			continue;	/* skip blank line and '#' line*/
		if (strncmp(buf, "interval ", 9) == 0) {
			strcat(buff, &buf[9]);

			ret = atoi(buff);
			if (strstr(buf, "sec") != NULL)
				ret = ret;
			else if (strstr(buf, "min") != NULL)
				ret = ret * 60;
			else if (strstr(buf, "hour") != NULL)
				ret = ret * 60 * 60;
			else if (strstr(buf, "day") != NULL)
				ret = ret * 60 * 60 * 24;
		}
	}
	fclose(fp);

	return (ret);
}

