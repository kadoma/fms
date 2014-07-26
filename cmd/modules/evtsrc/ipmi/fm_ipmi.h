#include <nvpair.h>

#define IPMI_BUF_SIZE	1024

#ifndef __max
# define __max(a, b)	((a) > (b) ? (a) : (b))
#endif

struct ipmi_rs {
	uint8_t ccode;
	uint8_t data[IPMI_BUF_SIZE];

	/*
	 * Looks like this is the length of the entire packet, including the RMCP
	 * stuff, then modified to be the length of the extra IPMI message data
	 */
	int data_len;

	struct {
		uint8_t netfn;
		uint8_t cmd;
		uint8_t seq;
		uint8_t lun;
	} msg;

	struct {
		uint8_t authtype;
		uint32_t seq;
		uint32_t id;
		uint8_t bEncrypted;	/* IPMI v2 only */
		uint8_t bAuthenticated;	/* IPMI v2 only */
		uint8_t payloadtype;	/* IPMI v2 only */
		/* This is the total length of the payload or
		   IPMI message.  IPMI v2.0 requires this to
		   be 2 bytes.  Not really used for much. */
		uint16_t msglen;
	} session;

	/*
	 * A union of the different possible payload meta-data
	 */
	union {
		struct {
			uint8_t rq_addr;
			uint8_t netfn;
			uint8_t rq_lun;
			uint8_t rs_addr;
			uint8_t rq_seq;
			uint8_t rs_lun;
			uint8_t cmd;
		} ipmi_response;
		struct {
			uint8_t message_tag;
			uint8_t rakp_return_code;
			uint8_t max_priv_level;
			uint32_t console_id;
			uint32_t bmc_id;
			uint8_t auth_alg;
			uint8_t integrity_alg;
			uint8_t crypt_alg;
		} open_session_response;
		struct {
			uint8_t message_tag;
			uint8_t rakp_return_code;
			uint32_t console_id;
			uint8_t bmc_rand[16];	/* Random number generated by the BMC */
			uint8_t bmc_guid[16];
			uint8_t key_exchange_auth_code[20];
		} rakp2_message;
		struct {
			uint8_t message_tag;
			uint8_t rakp_return_code;
			uint32_t console_id;
			uint8_t integrity_check_value[20];
		} rakp4_message;
		struct {
			uint8_t packet_sequence_number;
			uint8_t acked_packet_number;
			uint8_t accepted_character_count;
			uint8_t is_nack;	/* bool */
			uint8_t transfer_unavailable;	/* bool */
			uint8_t sol_inactive;	/* bool */
			uint8_t transmit_overrun;	/* bool */
			uint8_t break_detected;	/* bool */
		} sol_packet;

	} payload;
};

#define SDR_SENSOR_STAT_ERROR	-3
#define SDR_SENSOR_STAT_UNVAILD	-2
#define SDR_SENSOR_STAT_NOFOUND -1 	

#define SDR_SENSOR_STAT_LO_NC	(1<<0)
#define SDR_SENSOR_STAT_LO_CR	(1<<1)
#define SDR_SENSOR_STAT_LO_NR	(1<<2)
#define SDR_SENSOR_STAT_HI_NC	(1<<3)
#define SDR_SENSOR_STAT_HI_CR	(1<<4)
#define SDR_SENSOR_STAT_HI_NR	(1<<5)

struct sdr_get_rq {
	uint16_t reserve_id;
	uint16_t id;
	uint8_t offset;
	uint8_t length;
} __attribute__ ((packed));

struct sdr_get_rs {
	uint16_t next;
	uint16_t id;
	uint8_t	 version;
	uint8_t	 type;
	uint8_t	 length;
} __attribute__ ((packed));

struct entity_id {
	uint8_t	id;			/* physical entity id */
	uint8_t instance	: 7;	/* instance number */
	uint8_t logical		: 1;	/* physical / logical */
} __attribute__ ((packed));

struct sdr_record_mask {
	union {
		struct {
			uint16_t assert_event;	/* assertion event mask */
			uint16_t deassert_event;	/* de-assertion event mask */
			uint16_t read;	/* discrete reading mask */
		} discrete;
		struct {
			uint16_t assert_lnc_low:1;
			uint16_t assert_lnc_high:1;
			uint16_t assert_lcr_low:1;
			uint16_t assert_lcr_high:1;
			uint16_t assert_lnr_low:1;
			uint16_t assert_lnr_high:1;
			uint16_t assert_unc_low:1;
			uint16_t assert_unc_high:1;
			uint16_t assert_ucr_low:1;
			uint16_t assert_ucr_high:1;
			uint16_t assert_unr_low:1;
			uint16_t assert_unr_high:1;
			uint16_t status_lnc:1;
			uint16_t status_lcr:1;
			uint16_t status_lnr:1;
			uint16_t reserved:1;
			uint16_t deassert_lnc_low:1;
			uint16_t deassert_lnc_high:1;
			uint16_t deassert_lcr_low:1;
			uint16_t deassert_lcr_high:1;
			uint16_t deassert_lnr_low:1;
			uint16_t deassert_lnr_high:1;
			uint16_t deassert_unc_low:1;
			uint16_t deassert_unc_high:1;
			uint16_t deassert_ucr_low:1;
			uint16_t deassert_ucr_high:1;
			uint16_t deassert_unr_low:1;
			uint16_t deassert_unr_high:1;
			uint16_t status_unc:1;
			uint16_t status_ucr:1;
			uint16_t status_unr:1;
			uint16_t reserved_2:1;
			union {
				struct {
					/* padding lower 8 bits */
					uint16_t readable:8;
					uint16_t lnc:1;
					uint16_t lcr:1;
					uint16_t lnr:1;
					uint16_t unc:1;
					uint16_t ucr:1;
					uint16_t unr:1;
					uint16_t reserved:2;
				} set;
				struct {
					/* readable threshold mask */
					/* padding upper 8 bits */
					uint16_t lnc:1;
					uint16_t lcr:1;
					uint16_t lnr:1;
					uint16_t unc:1;
					uint16_t ucr:1;
					uint16_t unr:1;
					uint16_t reserved:2;
					uint16_t settable:8;
				} read;
			};
		} threshold;
	} type;
} __attribute__ ((packed));

struct sdr_record_full_sensor {
	struct {
		uint8_t owner_id;
		uint8_t lun:2;		/* sensor owner lun */
		uint8_t __reserved:2;
		uint8_t channel:4;	/* channel number */
		uint8_t sensor_num;	/* unique sensor number */
	} keys;

	struct entity_id entity;

	struct {
		struct {
			uint8_t sensor_scan:1;
			uint8_t event_gen:1;
			uint8_t type:1;
			uint8_t hysteresis:1;
			uint8_t thresholds:1;
			uint8_t events:1;
			uint8_t scanning:1;
			uint8_t __reserved:1;
		} init;
		struct {
			uint8_t event_msg:2;
			uint8_t threshold:2;
			uint8_t hysteresis:2;
			uint8_t rearm:1;
			uint8_t ignore:1;
		} capabilities;
		uint8_t type;
	} sensor;

	uint8_t event_type;	/* event/reading type code */

	struct sdr_record_mask mask;

	struct {
		uint8_t pct:1;
		uint8_t modifier:2;
		uint8_t rate:3;
		uint8_t analog:2;
		struct {
			uint8_t base;
			uint8_t modifier;
		} type;
	} unit;

#define SDR_SENSOR_L_LINEAR     0x00
#define SDR_SENSOR_L_LN         0x01
#define SDR_SENSOR_L_LOG10      0x02
#define SDR_SENSOR_L_LOG2       0x03
#define SDR_SENSOR_L_E          0x04
#define SDR_SENSOR_L_EXP10      0x05
#define SDR_SENSOR_L_EXP2       0x06
#define SDR_SENSOR_L_1_X        0x07
#define SDR_SENSOR_L_SQR        0x08
#define SDR_SENSOR_L_CUBE       0x09
#define SDR_SENSOR_L_SQRT       0x0a
#define SDR_SENSOR_L_CUBERT     0x0b
#define SDR_SENSOR_L_NONLINEAR  0x70

	uint8_t linearization;	/* 70h=non linear, 71h-7Fh=non linear, OEM */
	uint16_t mtol;		/* M, tolerance */
	uint32_t bacc;		/* accuracy, B, Bexp, Rexp */

	struct {
		uint8_t nominal_read:1;	/* nominal reading field specified */
		uint8_t normal_max:1;	/* normal max field specified */
		uint8_t normal_min:1;	/* normal min field specified */
		uint8_t __reserved:5;
	} analog_flag;

	uint8_t nominal_read;	/* nominal reading, raw value */
	uint8_t normal_max;	/* normal maximum, raw value */
	uint8_t normal_min;	/* normal minimum, raw value */
	uint8_t sensor_max;	/* sensor maximum, raw value */
	uint8_t sensor_min;	/* sensor minimum, raw value */

	struct {
		struct {
			uint8_t non_recover;
			uint8_t critical;
			uint8_t non_critical;
		} upper;
		struct {
			uint8_t non_recover;
			uint8_t critical;
			uint8_t non_critical;
		} lower;
		struct {
			uint8_t positive;
			uint8_t negative;
		} hysteresis;
	} threshold;
	uint8_t __reserved[2];
	uint8_t oem;		/* reserved for OEM use */
	uint8_t id_code;	/* sensor ID string type/length code */
	uint8_t id_string[16];	/* sensor ID string bytes, only if id_code != 0 */
}  __attribute__ ((packed));


struct sdr_record_compact_sensor {
	struct {
		uint8_t owner_id;
		uint8_t lun:2;		/* sensor owner lun */
		uint8_t __reserved:2;
		uint8_t channel:4;	/* channel number */
		uint8_t sensor_num;	/* unique sensor number */
	} keys;

	struct entity_id entity;

	struct {
		struct {
			uint8_t sensor_scan:1;
			uint8_t event_gen:1;
			uint8_t type:1;
			uint8_t hysteresis:1;
			uint8_t thresholds:1;
			uint8_t events:1;
			uint8_t scanning:1;
			uint8_t __reserved:1;
		} init;
		struct {
			uint8_t event_msg:2;
			uint8_t threshold:2;
			uint8_t hysteresis:2;
			uint8_t rearm:1;
			uint8_t ignore:1;
		} capabilities;
		uint8_t type;	/* sensor type */
	} sensor;

	uint8_t event_type;	/* event/reading type code */

	struct sdr_record_mask mask;

	struct {
		uint8_t pct:1;
		uint8_t modifier:2;
		uint8_t rate:3;
		uint8_t analog:2;
		struct {
			uint8_t base;
			uint8_t modifier;
		} type;
	} unit;

	struct {
		uint8_t count:4;
		uint8_t mod_type:2;
		uint8_t __reserved:2;
		uint8_t mod_offset:7;
		uint8_t entity_inst:1;
	} share;

	struct {
		struct {
			uint8_t positive;
			uint8_t negative;
		} hysteresis;
	} threshold;

	uint8_t __reserved[3];
	uint8_t oem;		/* reserved for OEM use */
	uint8_t id_code;	/* sensor ID string type/length code */
	uint8_t id_string[16];	/* sensor ID string bytes, only if id_code != 0 */
} __attribute__ ((packed));

struct sdr_record_list {
	uint16_t id;
	uint8_t version;
	uint8_t type;
	uint8_t length;
	uint8_t *raw;
	struct sdr_record_list *next;
	union {
		struct sdr_record_full_sensor *full;
		struct sdr_record_compact_sensor *compact;
	} record;
};

struct ipmi_sdr_iterator {
	uint16_t reservation;
	int total;
	int next;
	int use_built_in;
};

struct sdr_repo_info_rs {
	uint8_t 	version;
	uint16_t	count;		/* number of record */
	uint16_t	free;		/* free space in SDR */
	uint32_t	add_stamp;	/* last add timestamp */
	uint32_t	erase_stamp;	/* last del timestamp */
	uint8_t		op_support;	/* supported operation */
} __attribute__ ((packed));

struct sdr_reserve_repo_rs {
	uint16_t reserve_id;
} __attribute__ ((packed));

int	ipmi_get_dev_status(int fd, struct sdr_record_list *sdr);
char	*ipmi_sdr_get_status(uint8_t stat);
int	ipmi_init_sensor_list(int fd);
struct	sdr_record_list *get_sdr_list_head();
//struct	sdr_record_list *ipmi_sdr_find_sdr_byid(int fd, char *id); /* abandoned */
