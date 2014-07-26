#define	MAX_PAYLOAD		1024	/* maximum payload size */

#define PCIEAER_ERR_UNKNOWN	-1	/* unknown error */

/* multiple */
#define PCIEAER_MULTI_FIRST	0	/* first occur */	
#define PCIEAER_MULTI_MULTI	1	/* occured many times */

/* severity */
#define PCIEAER_SEV_WARN	0	/* nofatal error */
#define PCIEAER_SEV_ERR		1	/* fatal error */
#define PCIEAER_SEV_RECOVER	2	/* recoverable error */

/* type */
#define PCIEAER_TYPE_BUSUA	0	/* link inaccessible */
#define PCIEAER_TYPE_BUSDL	1	/* data link error */
#define	PCIEAER_TYPE_BUSTL	2	/* data transfer link error */
#define PCIEAER_TYPE_BUSPL	3	/* phsical link error */

/* error info */
#define PCIEAER_INFO_MSG	"MSG"
#define PCIEAER_INFO_ERR	"ERR"
#define PCIEAER_INFO_BUSUA	"BUS_UA"
#define PCIEAER_INFO_BUSDL	"BUS_DL"
#define PCIEAER_INFO_BUSTL	"BUS_TL"
#define PCIEAER_INFO_BUSPL	"BUS_PL"

#define FIELDS_TO_FILL		6	/* How many field the raw message contained.
					   At present, raw message consist of severity, 
					   type, domain, busnumber, slot and func, that
					   is, 6 field in total.
					 */

struct fm_kevt *kernel_event_recv(void);			/* Read message from kernel */
