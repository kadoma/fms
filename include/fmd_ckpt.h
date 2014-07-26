#ifndef	_FMD_CKPT_H
#define	_FMD_CKPT_H

#include <sys/types.h>
/*
 * Fault Manager Checkpoint Format (FCF)
 *
 * Fault manager modules can checkpoint state in the FCF format so that they
 * can survive restarts, module failures, and reboots.  The FCF format is
 * versioned and extensible so that it can be revised and so that internal data
 * structures can be modified or extended compatibly.  It is also specified as
 * a Project Private interface so that incompatible changes can occur as we see
 * fit.  All FCF structures use fixed-size types so that the 32-bit and 64-bit
 * forms are identical and consumers can use either data model transparently.
 *
 * The file layout is structured as follows:
 *
 * +---------------+-------------------+----- ... ----+---- ... ------+
 * |   fcf_hdr_t   |  fcf_sec_t[ ... ] |   section    |   section     |
 * | (file header) | (section headers) |   #1 data    |   #N data     |
 * +---------------+-------------------+----- ... ----+---- ... ------+
 * |<------------ fcf_hdr.fcfh_filesz ------------------------------->|
 *
 * The file header stores meta-data including a magic number, data model for
 * the checkpointed module, data encoding, and other miscellaneous properties.
 * The header describes its own size and the size of the section headers.  By
 * convention, an array of section headers follows the file header, and then
 * the data for all the individual sections listed in the section header table.
 *
 * The section headers describe the size, offset, alignment, and section type
 * for each section.  Sections are described using a set of #defines that tell
 * the consumer what kind of data is expected.  Sections can contain links to
 * other sections by storing a fcf_secidx_t, an index into the section header
 * array, inside of the section data structures.  The section header includes
 * an entry size so that sections with data arrays can grow their structures.
 *
 * Finally, strings are always stored in ELF-style string tables along with a
 * string table section index and string table offset.  Therefore strings in
 * FCF are always arbitrary-length and not bound to the current implementation.
 */

#define	FCF_ID_SIZE	16	/* total size of fcfh_ident[] in bytes */

typedef struct fcf_hdr {
	uint8_t fcfh_ident[FCF_ID_SIZE]; /* identification bytes (see below) */
	uint32_t fcfh_flags;		/* file attribute flags (if any) */
	uint32_t fcfh_hdrsize;		/* size of file header in bytes */
	uint32_t fcfh_secsize;		/* size of section header in bytes */
	uint32_t fcfh_secnum;		/* number of section headers */
	uint64_t fcfh_secoff;		/* file offset of section headers */
	uint64_t fcfh_filesz;		/* file size of entire FCF file */
	uint64_t fcfh_cgen;		/* checkpoint generation number */
	uint64_t fcfh_pad;		/* reserved for future use */
} fcf_hdr_t;

#define	FCF_ID_MAG0	0	/* first byte of magic number */
#define	FCF_ID_MAG1	1	/* second byte of magic number */
#define	FCF_ID_MAG2	2	/* third byte of magic number */
#define	FCF_ID_MAG3	3	/* fourth byte of magic number */
#define	FCF_ID_MODEL	4	/* FCF data model (see below) */
#define	FCF_ID_ENCODING	5	/* FCF data encoding (see below) */
#define	FCF_ID_VERSION	6	/* FCF file format major version (see below) */
#define	FCF_ID_PAD	7	/* start of padding bytes (all zeroes) */

#define	FCF_MAG_MAG0	0x7F	/* FCF_ID_MAG[0-3] */
#define	FCF_MAG_MAG1	'F'
#define	FCF_MAG_MAG2	'C'
#define	FCF_MAG_MAG3	'F'

#define	FCF_MAG_STRING	"\177FCF"
#define	FCF_MAG_STRLEN	4

#define	FCF_MODEL_NONE	0	/* FCF_ID_MODEL */
#define	FCF_MODEL_ILP32	1
#define	FCF_MODEL_LP64	2

#ifdef _LP64
#define	FCF_MODEL_NATIVE	FCF_MODEL_LP64
#else
#define	FCF_MODEL_NATIVE	FCF_MODEL_ILP32
#endif

#define	FCF_ENCODE_NONE	0	/* FCF_ID_ENCODING */
#define	FCF_ENCODE_LSB	1
#define	FCF_ENCODE_MSB	2

#ifdef _BIG_ENDIAN
#define	FCF_ENCODE_NATIVE	FCF_ENCODE_MSB
#else
#define	FCF_ENCODE_NATIVE	FCF_ENCODE_LSB
#endif

#define	FCF_VERSION_1	1	/* FCF_ID_VERSION */
#define	FCF_VERSION	FCF_VERSION_1

#define	FCF_FL_VALID	0	/* mask of all valid fcfh_flags bits */

typedef struct fcf_sec {
	uint32_t fcfs_type;	/* section type (see below) */
	uint64_t fcfs_offset;	/* offset of section data within file */
	uint64_t fcfs_size;	/* size of section data in bytes */
	uint32_t fcfs_evtnum;	/* number of events */
} fcf_sec_t;

/*
 * Section types (fcfs_type values).  These #defines should be kept in sync
 * with the decoding table declared in fmd_mdb.c in the fcf_sec() dcmd, and
 * with the size and alignment table declared at the top of fmd_ckpt.c.
 */
#define	FCF_SECT_CASE_SERD	0	/* case & serd section */
#define	FCF_SECT_DISPQ		1	/* dispq section */

typedef struct fcf_case {
	uint64_t fcfc_uuid;			/* case uuid */
	uint64_t fcfc_rscid;
	char	 fcfc_fault[50];	/* for fmd_case_type use */
	char	 fcfc_serd[50];     /* for fmd_case_type use */
	uint8_t  fcfc_state;		/* case state (see below) */
	uint32_t fcfc_count;
	uint64_t fcfc_create;
	uint64_t fcfc_fire;
	uint64_t fcfc_close;
} fcf_case_t;

#define FCF_CASE_CREATE		0x00
#define FCF_CASE_FIRED		0x01
#define FCF_CASE_CLOSED		0x02

typedef struct fcf_serd {
	uint64_t fcfd_sid;	/* engine ID */
	uint32_t fcfd_n;	/* engine N parameter */
	uint32_t fcfd_t;	/* engine T parameter */
} fcf_serd_t;

typedef struct fcf_dispq {
	uint8_t fcfq_ev_num;
} fcf_dispq_t;

typedef struct fcf_event {
	uint64_t fcfe_ev_eid;
	uint64_t fcfe_ev_uuid;		/* event (fault.* & list.*)'s case id */
	uint64_t fcfe_ev_rscid;		/* resource id */
	char	 fcfe_ev_class[50];
	uint8_t  fcfe_ev_refs;
	uint64_t fcfe_ev_create;	/* created time */
} fcf_event_t;

/*
 * The checkpoint subsystem provides a very simple set of interfaces to the
 * reset of fmd: namely, checkpoints can be saved, restored, or deleted by mod.
 * In the reference implementation, these are implemented to use FCF files.
 */

extern void fmd_ckpt_save(void);
extern void fmd_ckpt_restore(void);
extern void fmd_ckpt_delete(char *);
extern int fmd_ckpt_rename(char *);
extern uint64_t fmd_ckpt_conf(void);

#endif	/* _FMD_CKPT_H */
