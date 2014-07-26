/*
 * check_http.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *
 * Apache Service Event Source Module HTTP CHECK
 *
 * This plugin tests the HTTP service on the specified host. It can test
 * normal (http) and secure (https) servers, follow redirects, search for
 * strings and regular expressions, check connection times, and report on
 * certificate expiration times.
 */

#include "common.h"
#include "netutils.h"
#include "utils.h"
#include "apache_fm.h"
#include <ctype.h>

#define HTTP_EXPECT "HTTP/1."
enum {
	MAX_IPV4_HOSTLENGTH = 255,
	HTTP_PORT = 80,
	HTTPS_PORT = 443,
	MAX_PORT = 65535
};

#define my_recv(buf, len) read(sd, buf, len)
#define my_send(buf, len) send(sd, buf, len, 0)

/* my_connect and wrapper macros */
#define my_tcp_connect(addr, port, s) np_net_connect(addr, port, s, IPPROTO_TCP)
#define my_udp_connect(addr, port, s) np_net_connect(addr, port, s, IPPROTO_UDP)
int np_net_connect(const char *address, int port, int *sd, int proto);

int no_body = FALSE;

volatile int sigcount = 0;

unsigned int socket_timeout = DEFAULT_SOCKET_TIMEOUT;
unsigned int socket_timeout_state = STATE_CRITICAL;

struct timeval tv;

#define HTTP_URL "/"
#define CRLF "\r\n"

int server_port = HTTP_PORT;
char *server_address;
char *host_name;
char *server_url;
char *user_agent;
int server_url_length;
int server_expect_yn = 0;
char server_expect[MAX_INPUT_BUFFER] = HTTP_EXPECT;
char string_expect[MAX_INPUT_BUFFER] = "";
double warning_time = 0;
int check_warning_time = FALSE;
double critical_time = 0;
int check_critical_time = FALSE;
int display_html = FALSE;
int verbose = FALSE;
int sd;
int min_page_len = 0;
int max_page_len = 0;
char *http_method;
char *http_content_type;
char buffer[MAX_INPUT_BUFFER];

int check_http(char *faultname);
char *perfd_time(double microsec);
char *perfd_size(int page_len);

/* handles socket timeouts */
void
socket_timeout_alarm_handler(int sig)
{
	if (sig == SIGALRM) {
		printf("Socket timeout after %d seconds\n", socket_timeout);
		sigcount = 1;
	} else {
		printf("Abnormal timeout after %d seconds\n", socket_timeout);
	}
//	exit(socket_timeout_state);
}

void
before_check(void)
{
	struct sigaction sact;
	int result = STATE_UNKNOWN;

	/* critical time threshold */
	critical_time = 10;
	check_critical_time = TRUE;

	/* warning time threshold */
	warning_time = 5;
	check_warning_time = TRUE;

	/* show html link */
	display_html = TRUE;

	/* verbose */
	verbose = TRUE;

	/* no-body */
	no_body = TRUE;

	/* User Agent String */
	asprintf(&user_agent, "User-Agent: %s", "check_http");

	/* Host Name (virtual host) */
	host_name = strdup("www.kernel.org");

	/* Server port */
	server_port = HTTP_PORT;

	/* Server IP-address */
	server_address = strdup("199.6.1.164");

	/* URL path */
	server_url = strdup("http://www.kernel.org/");
	server_url_length = strlen(server_url);

	/* Set HTTP method */
	if (http_method)
		free(http_method);
	http_method = strdup("GET");

	if (display_html == TRUE)
		printf("<A HREF=\"%s://%s:%d%s\" target=\"_blank\">\n",
		       "http", host_name ? host_name : server_address,
		       server_port, server_url);

	/* initialize alarm signal handling, set socket timeout, start timer */
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = socket_timeout_alarm_handler;
	sigaction(SIGALRM, &sact, NULL);
//      (void) signal(SIGALRM, socket_timeout_alarm_handler);
	(void) alarm(socket_timeout);
	gettimeofday(&tv, NULL);

//	result = check_http();

//	return result;
}

/* Returns 1 if we're done processing the document body; 0 to keep going */
static int
document_headers_done(char *full_page)
{
	const char *body;

	for (body = full_page; *body; body++) {
		if (!strncmp(body, "\n\n", 2) || !strncmp(body, "\n\r\n", 3))
		break;
	}

	if (!*body)
	return 0;  /* haven't read end of headers yet */

	full_page[body - full_page] = 0;
	return 1;
}

/* Checks if the server 'reply' is one of the expected 'statuscodes' */
static int
expected_statuscode(const char *reply, const char *statuscodes)
{
	char *expected, *code;
	int result = 0;

	if ((expected = strdup(statuscodes)) == NULL)
//		die(STATE_UNKNOWN, _("HTTP UNKNOWN - Memory allocation error\n"));

	for (code = strtok(expected, ","); code != NULL; code = strtok(NULL, ","))
		if (strstr(reply, code) != NULL) {
			result = 1;
			break;
		}

	free(expected);

	return result;
}

#if 0
enum {
	FM_OK = 0,
	NETWORK_UNREACHABLE = 1,
	INVALID_ERR = 2,
	SERVER_ERR = 3,
	CLIENT_ERR = 4
};
#endif

int
check_http(char *faultname)
{
	char *msg;
	char *status_line;
	char *status_code;
	char *header;
	char *page;
	char *auth;
	int http_status;
	int i = 0;
	size_t pagesize = 0;
	char *full_page;
	char *buf;
	char *pos;
	long microsec;
	double elapsed_time;
	int page_len = 0;
	int result = STATE_OK;

	int fm_flag = 0;

	before_check();

	/* try to connect to the host at the given port number */
	if (my_tcp_connect(server_address, server_port, &sd) != STATE_OK) {
		printf("HTTP CRITICAL - Unable to open TCP socket\n");
		fm_flag = NETWORK_UNREACHABLE;
		goto failed;
	}

	asprintf(&buf, "%s %s %s\r\n%s\r\n", http_method, server_url, host_name ? "HTTP/1.1" : "HTTP/1.0", user_agent);

	/* tell HTTP/1.1 servers not to keep the connection alive */
	asprintf(&buf, "%sConnection: close\r\n", buf);

	/* optionally send the host header info */
	if (host_name) {
	/*
	 * Specify the port only if we're using a non-default port (see RFC 2616,
	 * 14.23).  Some server applications/configurations cause trouble if the
	 * (default) port is explicitly specified in the "Host:" header line.
	 */
		if (server_port == HTTP_PORT)
			asprintf(&buf, "%sHost: %s\r\n", buf, host_name);
		else
			asprintf(&buf, "%sHost: %s:%d\r\n", buf, host_name, server_port);
	}

	asprintf (&buf, "%s%s", buf, CRLF);

	if (verbose) printf("%s\n", buf);

	my_send(buf, strlen(buf));

	/* fetch the page */
	full_page = strdup("");

	while ((i = my_recv(buffer, MAX_INPUT_BUFFER-1)) > 0) {
		buffer[i] = '\0';
		asprintf(&full_page, "%s%s", full_page, buffer);
		pagesize += i;

		if (no_body && document_headers_done(full_page)) {
			i = 0;
			break;
		}
	}

	if (i < 0 && errno != ECONNRESET) {
		printf("HTTP CRITICAL - Error on receive\n");
	}

	/* return a CRITICAL status if we couldn't read any data */
	if (pagesize == (size_t) 0)
		printf("HTTP CRITICAL - No data received from host\n");

	/* close the connection */
	if (sd) close(sd);

	/* Save check time */
	microsec = deltime(tv);
	elapsed_time = (double)microsec / 1.0e6;

	/* leave full_page untouched so we can free it later */
	page = full_page;

	if (verbose)
		printf("%s://%s:%d%s is %d characters\n", "http", server_address,
			server_port, server_url, (int)pagesize);

	/* find status line and null-terminate it */
	status_line = page;
	page += (size_t) strcspn(page, "\r\n");
	pos = page;
	page += (size_t) strspn(page, "\r\n");
	status_line[strcspn(status_line, "\r\n")] = 0;
	strip(status_line);
	if (verbose)
		printf("STATUS: %s\n", status_line);

	/* find header info and null-terminate it */
	header = page;
	while (strcspn(page, "\r\n") > 0) {
		page += (size_t) strcspn(page, "\r\n");
		pos = page;
		if ((strspn(page, "\r") == 1 && strspn(page, "\r\n") >= 2) ||
			(strspn(page, "\n") == 1 && strspn(page, "\r\n") >= 2))
			page += (size_t) 2;
		else
			page += (size_t) 1;
	}
	page += (size_t) strspn(page, "\r\n");
	header[pos - header] = 0;
	if (verbose)
		printf("**** HEADER ****\n%s\n**** CONTENT ****\n%s\n", header,
                	(no_body ? "  [[ skipped ]]" : page));

#if 0
	/* make sure the status line matches the response we are looking for */
	if (!expected_statuscode(status_line, server_expect)) {
		if (server_port == HTTP_PORT)
			asprintf(&msg, "Invalid HTTP response received from host: %s\n", status_line);
		else
			asprintf(&msg, "Invalid HTTP response received from host on port %d: %s\n",
				server_port, status_line);
		printf("HTTP CRITICAL - %s", msg);
	}

	/* Bypass normal status line check if server_expect was set by user and not default */
	/* NOTE: After this if/else block msg *MUST* be an asprintf-allocated string */
	if (server_expect_yn) {
		asprintf(&msg, "Status line output matched \"%s\" - ", server_expect);
		if (verbose)
			printf("%s\n",msg);
	} else {
	/* Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF */
	/* HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT */
	/* Status-Code = 3 DIGITS */
#endif
		status_code = strchr(status_line, ' ') + sizeof (char);
		if (strspn(status_code, "1234567890") != 3)
			printf("HTTP CRITICAL: Invalid Status Line (%s)\n", status_line);

		http_status = atoi(status_code);

		/* check the return code */

		if (http_status >= 600 || http_status < 100) {
			printf("HTTP CRITICAL: Invalid Status (%s)\n", status_line);
			fm_flag = INVALID_ERR;
		}
		/* server errors result in a critical state */
		else if (http_status >= 500) {
			asprintf(&msg, "%s - ", status_line);
			result = STATE_CRITICAL;
			fm_flag = SERVER_ERR;
		}
		/* client errors result in a warning state */
		else if (http_status >= 400) {
			asprintf(&msg, "%s - ", status_line);
			result = max_state_alt(STATE_WARNING, result);
			fm_flag = CLIENT_ERR;
		}
		/* check redirected page if specified */
		else if (http_status >= 300) {
//
//			if (onredirect == STATE_DEPENDENT)
//				redir(header, status_line);
//			else
//				result = max_state_alt(onredirect, result);
//			asprintf(&msg, _("%s - "), status_line);
		} /* end if (http_status >= 300) */
		else {
		/* Print OK status anyway */
			asprintf(&msg, "%s - ", status_line);
			fm_flag = FM_OK;
		}

//	} /* end else (server_expect_yn)  */

////////////////////////////////////////////////////////////////////////////////////////////////////
failed:
	if (sigcount == 0) {
		/* reset the alarm - must be called *after* redir or we'll never die on redirects! */
		alarm(0);

		switch(fm_flag) {
		case FM_OK:
			result = STATE_OK;
			printf("Apache HTTP Service is OK!\n");
			return result;
		case NETWORK_UNREACHABLE:
			faultname = strdup("network_unreachable");
			printf("Apache_fm_event_post(network_unreachable)\n");
			result = STATE_CRITICAL;
			break;
		case INVALID_ERR:
			faultname = strdup("invalid_error");
			printf("Apache_fm_event_post(invalid_error)\n");
			result = STATE_CRITICAL;
			break;
		case SERVER_ERR:
			faultname = strdup("server_error");
			printf("Apache_fm_event_post(server_error)\n");
			result = STATE_CRITICAL;
			break;
		case CLIENT_ERR:
			faultname = strdup("client_error");
			printf("Apache_fm_event_post(client_error)\n");
			result = STATE_WARNING;
			break;

		default:
			fm_flag = 0;
			result = STATE_DEPENDENT;
			return result;
		}
		
//		apache_fm_event_post(faultname);
		return result;

	} else {
		printf("Socket Timeout!\n");
		faultname = strdup("socket_timeout");
//		apache_fm_event_post(faultname);
		printf("Apache_fm_event_post(socket_timeout)\n");

		return STATE_CRITICAL;
	}
}

char *perfd_time(double elapsed_time)
{
	return fperfdata("time", elapsed_time, "s",
		check_warning_time, warning_time,
		check_critical_time, critical_time,
		TRUE, 0, FALSE, 0);
}

char *perfd_size(int page_len)
{
	return perfdata("size", page_len, "B",
		(min_page_len>0?TRUE:FALSE), min_page_len,
		(min_page_len>0?TRUE:FALSE), 0,
		TRUE, 0, FALSE, 0);
}

