#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/ipmi.h>
#include <OpenIPMI/ipmiif.h>
#include <OpenIPMI/ipmi_msgbits.h>
#include <OpenIPMI/ipmi_addr.h>

#include "fm_ipmi.h"

#define	SCANNING_DISABLED	0x40
#define	READING_UNAVAILABLE	0x20

static int sdr_max_read_len = 0xff;

static struct sdr_record_list *sdr_list_head	= NULL;
static struct sdr_record_list *sdr_list_tail	= NULL;
static struct ipmi_sdr_iterator *sdr_list_itr	= NULL;

/* Send command to IPMI use ioctl via fd */
static struct ipmi_rs *
ipmi_openipmi_send_cmd(int fd, struct ipmi_req *req)
{
	struct	ipmi_req	_req;		/* request which send to IPMI */
	struct 	ipmi_recv	recv;		/* recive data container */
	struct	ipmi_addr	addr;		
	static	struct ipmi_rs	rsp;		/* response return */
	struct	ipmi_system_interface_addr bmc_addr = {
		addr_type:	IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
		channel: 	IPMI_BMC_CHANNEL,
	};
	static int curr_seq = 0;
	fd_set	rset;

	/* Set req and send */
	memset(&_req, 0, sizeof(struct ipmi_req));
/*	bmc_addr.lun	= req->msg.lun;*/
	_req.addr	= (unsigned char *) &bmc_addr;
	_req.addr_len	= sizeof(bmc_addr);
	_req.msgid	= curr_seq++;
	_req.msg.netfn	= req->msg.netfn;
	_req.msg.cmd	= req->msg.cmd;
	_req.msg.data	= req->msg.data;
	_req.msg.data_len = req->msg.data_len;

	if ( ioctl(fd, IPMICTL_SEND_COMMAND, &_req) < 0 ) {
		fprintf(stderr, "Unable to send command\n");
		return NULL;
	}

	/* wait for and retrieve response */

	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	if ( select(fd+1, &rset, NULL, NULL, NULL) < 0 ) {
		fprintf(stderr, "I/O Error\n");
		return NULL;
	}
	if ( FD_ISSET(fd, &rset) == 0 ) {
		fprintf(stderr, "No data available\n");
		return NULL;
	}

	/* Get data */
	recv.addr 	= (unsigned char*) &addr;
	recv.addr_len	= sizeof(addr);
	recv.msg.data	= rsp.data;
	recv.msg.data_len = sizeof(rsp.data);
	
	if ( ioctl(fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv) < 0 ){
		fprintf(stderr, "Error receiving message");
		if ( errno != EMSGSIZE )
			return NULL;
	}

	rsp.ccode = recv.msg.data[0];
	rsp.data_len = recv.msg.data_len - 1;
	if ( rsp.ccode == 0 && rsp.data_len > 0 ) {
		memmove(rsp.data, rsp.data+1, rsp.data_len);
		rsp.data[recv.msg.data_len] = 0;
	}

	return &rsp;
}

/* retrieve a raw sensor reading from ipmb */
static struct ipmi_rs *
ipmi_sdr_get_sensor_reading_ipmb(int fd, uint8_t sensor, uint8_t target, uint8_t lun)
{
	struct ipmi_req		req;
	struct ipmi_rs	*rsp;

	memset(&req, 0, sizeof(req));
	req.msg.netfn 	= IPMI_NETFN_SENSOR_EVENT_REQUEST;
	req.msg.cmd	= IPMI_GET_SENSOR_READING_CMD;
	req.msg.data	= &sensor;
	req.msg.data_len = 1;

	rsp = ipmi_openipmi_send_cmd(fd, &req);
	return rsp;
}

/* Obtain SDR reservation ID */
static int ipmi_sdr_get_reservation(int fd, uint16_t *reserve_id)
{
	struct ipmi_rs	*rsp;
	struct ipmi_req	req;

	memset(&req, 0, sizeof(req));
	req.msg.netfn = IPMI_NETFN_STORAGE_REQUEST;
	req.msg.cmd = IPMI_RESERVE_SDR_REPOSITORY_CMD;
	rsp = ipmi_openipmi_send_cmd(fd, &req);
	if (rsp == NULL || rsp->ccode > 0)
		return -1;

	*reserve_id = ((struct sdr_reserve_repo_rs *) &(rsp->data))->reserve_id;

	return 0;
}

static struct ipmi_sdr_iterator *
ipmi_sdr_start(int fd)
{
	struct ipmi_sdr_iterator *itr;
	struct ipmi_rs	*rsp;
	struct ipmi_req	req;
	struct sdr_repo_info_rs sdr_info;

	itr = malloc(sizeof (struct ipmi_sdr_iterator));
	if (itr == NULL){
		return NULL;
	}

	itr->use_built_in = 0;

	memset(&req, 0, sizeof(struct ipmi_req));
	req.msg.netfn = IPMI_NETFN_STORAGE_REQUEST;
	req.msg.cmd = IPMI_GET_SDR_REPOSITORY_INFO_CMD;
	rsp = ipmi_openipmi_send_cmd(fd, &req);
	if (rsp == NULL) {
		free(itr);
		return NULL;
	}
	if (rsp->ccode > 0) {
		free(itr);
		return NULL;
	}

	memcpy(&sdr_info, rsp->data, sizeof(sdr_info));
	itr->total = sdr_info.count;
	itr->next = 0;

	if ( ipmi_sdr_get_reservation(fd, &(itr->reservation)) < 0 ) {
		fprintf(stderr, "IPMI: Unable to obtain SDR reservation\n");
		free(itr);
		return NULL;
	}

	return itr;
}

/* Retreive SDR record header */
static struct sdr_get_rs *
ipmi_sdr_get_next_header(int fd, struct ipmi_sdr_iterator *itr)
{
	int try;
	struct sdr_get_rs 	*header;
	struct ipmi_req		req;
	struct ipmi_rs		*rsp;
	struct sdr_get_rq	sdr_rq;

	if (itr->next == 0xffff)
		return NULL;

	header = (struct sdr_get_rs *)malloc(sizeof(struct sdr_get_rs));
	if ( header == NULL ) {
		fprintf(stderr, "IPMI: malloc error\n");
		return NULL;
	}

	memset(&sdr_rq, 0, sizeof(sdr_rq));
	sdr_rq.reserve_id = itr->reservation;
	sdr_rq.id	= itr->next;
	sdr_rq.offset	= 0;
	sdr_rq.length	= 5;	/* Get the header */

	memset(&req, 0, sizeof(req));
	req.msg.netfn	= IPMI_NETFN_STORAGE_REQUEST;
	req.msg.cmd 	= IPMI_GET_SDR_CMD;
	req.msg.data	= (uint8_t *) & sdr_rq;
	req.msg.data_len= sizeof(sdr_rq);

	for ( try=0; try<5; try++ ) {
		sdr_rq.reserve_id = itr->reservation;
		rsp = ipmi_openipmi_send_cmd(fd, &req);
		if (rsp == NULL){
			fprintf(stderr, "IPMI: Get SDR %04x command failed\n", itr->next);
			continue;
		} else if (rsp->ccode == 0xc5) { /* lost reservation */
			fprintf(stderr, "IPMI: SDR reservation %04x cancelled\n"
					"IPMI: Sleeping a bit and retry...\n", 
					itr->reservation);
			sleep(rand() & 3);

			if (ipmi_sdr_get_reservation(fd, &(itr->reservation)) < 0) {
				fprintf(stderr, "IPMI: Unable to renew SDR reservation\n");
				return NULL;
			}
			
		} else if ( rsp->ccode > 0) {
			fprintf(stderr, "IPMI: Get SDR %04x command failed: %d\n",
					itr->next, rsp->ccode);
			continue;
		} else
			break;
	}

	if ( try == 5 || !rsp )
		return NULL;

	memcpy(header, rsp->data, sizeof(struct sdr_get_rs));
	if (header->length == 0) {
		fprintf(stderr, "IPMI: Record id 0x%04x: invalid length\n", itr->next);
		free(header);
		return NULL;
	}

	itr->next = header->next;
	return header;
}

/* Return raw SDR record */
static uint8_t *
ipmi_sdr_get_record(int fd, struct sdr_get_rs *header, struct ipmi_sdr_iterator *itr)
{
	int i	= 0;
	int len = header->length; 
	struct ipmi_req	req;
	struct ipmi_rs	*rsp;
	struct sdr_get_rq sdr_rq;
	uint8_t		*data;

	if (len < 1)
		return NULL;

	data = malloc(len + 1);
	if (data == NULL) {
		fprintf(stderr, "IPMI: malloc failure\n");
		return NULL;
	}
	memset(data, 0, len + 1);

	memset(&sdr_rq, 0, sizeof (sdr_rq));
	sdr_rq.reserve_id = itr->reservation;
	sdr_rq.id 	= header->id;
	sdr_rq.offset 	= 0;

	memset(&req, 0, sizeof (req));
	req.msg.netfn	= IPMI_NETFN_STORAGE_REQUEST;
	req.msg.cmd	= IPMI_GET_SDR_CMD;
	req.msg.data	= (uint8_t *) & sdr_rq;
	req.msg.data_len = sizeof (sdr_rq);

	/* read SDR record with partial reads
	 * because a full read usually exceeds the maximum
	 * transport buffer size.  (completion code 0xca)
	 */
	while (i < len) {
		sdr_rq.length = (len - i < sdr_max_read_len) ?
		    len - i : sdr_max_read_len;
		sdr_rq.offset = i + 5;	/* 5 header bytes */

//		fprintf(stdout, "Getting %d bytes from SDR at offset %d\n",
//			sdr_rq.length, sdr_rq.offset);

		rsp = ipmi_openipmi_send_cmd(fd, &req);
		if (rsp == NULL) {
			sdr_max_read_len = sdr_rq.length - 1;
			if (sdr_max_read_len > 0) {
			/* no response may happen if requests are bridged
			   and too many bytes are requested */
			continue;
			} else {
				free(data);
				return NULL;
			}
		}

		switch (rsp->ccode) {
		case 0xca:
			/* read too many bytes at once */
			sdr_max_read_len = sdr_rq.length - 1;
			continue;
		case 0xc5:
			/* lost reservation */
			fprintf(stderr, "IPMI: SDR reservation cancelled. \n"
				"IPMI: Sleeping a bit and retrying...");

			sleep(rand() & 3);

			if (ipmi_sdr_get_reservation(fd, &(itr->reservation)) < 0) {
				free(data);
				return NULL;
			}
			sdr_rq.reserve_id = itr->reservation;
			continue;
		}

		/* special completion codes handled above */
		if (rsp->ccode > 0 || rsp->data_len == 0) {
			free(data);
			return NULL;
		}

		memcpy(data + i, rsp->data + 2, sdr_rq.length);
		i += sdr_max_read_len;
	}

	return data;
}

/*
 * ipmi_init_sensor_list
 *  scan all sensor, add concerned sensor to list
 *
 * Input
 *  fd		- file descriptor of ipmi device
 *
 * Return value
 *  0		- if success
 *  none zero	- if failed
 */
int
ipmi_init_sensor_list(int fd)
{
	struct sdr_get_rs	*header;

	if ( sdr_list_itr == NULL ) {
		sdr_list_itr = ipmi_sdr_start(fd);	/* ipmi_sdr_start */
		if ( sdr_list_itr == NULL )
			return 1;
	}

	while ( (header = ipmi_sdr_get_next_header(fd, sdr_list_itr)) != NULL ) {
		int	concerned = 0;
		uint8_t *rec;
		struct 	sdr_record_list *sdrr;

		sdrr = (struct sdr_record_list*)malloc(sizeof(struct sdr_record_list));
		if ( sdrr == NULL ) {
			fprintf(stderr, "IPMI: malloc failure\n");
			return -1;
		}

		/* get the record */
		memset(sdrr, 0, sizeof(struct sdr_record_list));
		sdrr->id	= header->id;
		sdrr->type	= header->type;
		rec = ipmi_sdr_get_record(fd, header, sdr_list_itr);
		if ( rec == NULL )
			continue;

		/* fill the rec info */
		switch ( header->type ) {
		case IPMI_SDR_FULL_SENSOR_RECORD:
			sdrr->record.full = (struct sdr_record_full_sensor*)rec;
			if ( 	/* Proc temp */
				( strstr((const char*)sdrr->record.full->id_string, "Proc") &&
				strstr((const char*)sdrr->record.full->id_string, "Temp") )
				|| /* Mem temp */
				( strstr((const char*)sdrr->record.full->id_string, "Mem Bd") &&
				strstr((const char*)sdrr->record.full->id_string, "Temp") )
				|| /* ioh temp */
				( strstr((const char*)sdrr->record.full->id_string, "I/O Bd") &&
				strstr((const char*)sdrr->record.full->id_string, "Temp") )
				|| /* Fan rpm */
				strstr((const char*)sdrr->record.full->id_string, "Tach Fan")
				) {
				concerned = 1;
			}
			break;
		case IPMI_SDR_COMPACT_SENSOR_RECORD:
			sdrr->record.compact = (struct sdr_record_compact_sensor*)rec;
			if ( 	/* Proc temp */
				( strstr((const char*)sdrr->record.compact->id_string, "Proc") &&
				strstr((const char*)sdrr->record.compact->id_string, "Temp") )
				|| /* Mem temp */
				( strstr((const char*)sdrr->record.compact->id_string, "Mem Bd") &&
				strstr((const char*)sdrr->record.compact->id_string, "Temp") )
				|| /* ioh temp */
				( strstr((const char*)sdrr->record.compact->id_string, "I/O Bd") &&
				strstr((const char*)sdrr->record.compact->id_string, "Temp") )
				|| /* Fan rpm */
				strstr((const char*)sdrr->record.compact->id_string, "Tach Fan")
				) {
				concerned = 1;
			}
			break;
		default:
			free(rec);
			continue;
		}

		if (concerned){
			if (sdr_list_head == NULL)
				sdr_list_head = sdrr;
			else
				sdr_list_tail->next = sdrr;
			sdr_list_tail = sdrr;
		}
	}/* end of while */

	return 0;
}

/*
 * ipmi_get_dev_status
 *  get the sensor's status
 * 
 * Input
 *  fd		- file descriptor of ipmi dev
 *  sensor_id	- name of the sensor
 */
int
ipmi_get_dev_status(int fd, struct sdr_record_list *sdr)
{
	int	validread;
	struct	ipmi_rs	*rsp;
	struct	sdr_record_full_sensor *sensor;
	uint8_t	status;

	if (sdr == NULL) {
		fprintf(stderr, "IPMI: invalid sdr\n");
		return SDR_SENSOR_STAT_NOFOUND;
	}

	/* get current reading */
	validread = 1;
	sensor = sdr->record.full;
	rsp = ipmi_sdr_get_sensor_reading_ipmb(fd,
			sensor->keys.sensor_num,
			sensor->keys.owner_id,
			sensor->keys.lun);

	/* if reading valid? */
	if (rsp == NULL ) {
		fprintf(stderr, "IPMI: Error reading sensor\n");
		return SDR_SENSOR_STAT_ERROR;
	} else if (rsp->ccode 
		|| (rsp->data[1] & READING_UNAVAILABLE) 
		|| !(rsp->data[1] & SCANNING_DISABLED) ) {
		validread = 0;
		return SDR_SENSOR_STAT_UNVAILD;	/* we can read, but the value is unvaild */
	}

	/* get status */
	status = (uint8_t)rsp->data[2];
	/*
	*/
	return status;
}

/*
 * ipmi_sdr_get_status
 *  return the string of status
 */
char *
ipmi_sdr_get_status(uint8_t stat)
{
	if ( stat & SDR_SENSOR_STAT_LO_NR ){
		return "lower-non-recoverable";
	} else if ( stat & SDR_SENSOR_STAT_HI_NR ){
		return "upper-non-recoverable";
	} else if ( stat & SDR_SENSOR_STAT_LO_CR ){
		return "lower-critical";
	} else if ( stat & SDR_SENSOR_STAT_HI_CR ){
		return "upper-critical";
	} else if ( stat & SDR_SENSOR_STAT_LO_NC ){
		return "lower-non-critical";
	} else if ( stat & SDR_SENSOR_STAT_HI_NC ){
		return "upper-non-crititcal";
	}
	return "OK";
}

struct sdr_record_list *
get_sdr_list_head()
{
	return sdr_list_head;
}

