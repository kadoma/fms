fms
===

Linux Fault Management System 

FMS provides automated diagnosis of faulty hardware, and can take proactive measures to correct (e.g., offline a CPU) hardware-related faults. It can be considered the FMA(Solaris Fault Architecture) for Linux. 

cmd - FMD main routine and CLI tools
	cmd/fmd - FMD main routine
		fmd_main.c - main
		fmd_case.c - user case
		fmd_dispq.c - dispatch queue
		fmd_serd.c - diagnosis engine

	cmd/modules - event source modules and agent modules
		cmd/modules/evtsrc - event source modules 
		cmd/modules/agent - agent modules

	cmd/fmadm - FMD management tool(configurate,query,monitor, etc£©
	cmd/fmdump - log inquiry
	cmd/fmtmsg - event inquiry
	cmd/fmtopo - topology inquiry
	cmd/fmckpt - checkpoint tool
	cmd/fminject - fault injection

dict - event library
esc - rule library
include - header files

lib - library functions
	lib/libesc - diagnostic
	lib/libfmd - main routine library
		fmd_api.c - interfaces
		fmd_ckpt.c - checkpoint and rollback
		fmd_event.c - events
		fmd_xprt.c - transport channel

	lib/libfmd_adm - FMD management
	lib/libfmd_msg - message mechanism
	lib/libnvpair - key-value dataset
	lib/libtopo - system resource and topology

	lib/modules
		lib/modules/libagent - agent module
		lib/modules/libevtsrc - event source module
		lib/modules/libmod - others

scripts - inject faults
kfmd - fmd kernel space codes

