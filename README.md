fms
===

Linux Fault Management System 

FMS provides automated diagnosis of faulty hardware, and can take proactive measures to correct (e.g., offline a CPU) hardware-related faults. It can be considered the FMA(Solaris Fault Architecture) for Linux. 

cmd - FMD main routine and CLI tools
	fms/fmd - FMD main routine
		fmd_main.c - main
		fmd_case.c - user case and diagnosis engine
		fmd_queue.c - dispatch queue

	fms/evt_modules - event source modules and agent modules
		evt_modules/evtsrc - event source modules 
		evt_modules/agent - agent modules

	fms/evt_modules/kfm - kernel module for process events
	fms/evt_modules/kfm/kfmadm - debug tools for kernel module

	fms/lib/ -  some shared modules lib
		fms/lib/libcase - case 
		fms/lib/libesc - the events
		fms/lib/libfmd - shared lib
		fms/lib/libfmd_msg
		fms/lib/libfmd_adm

	fms/tools/fmadm - FMD management tool(configurate,query,monitor, etc£©
	fms/tools/fmtopo - topology inquiry
	fms/tools/fminject - fault injection

	dict - event library
	include - header files

	fms_conf - configue files for FMS . /usr/lib/fms

	err-inject - test inject

