#include <stdio.h>
#include <libintl.h>
#include <locale.h>

#define _(DOMAIN, S) dgettext(DOMAIN, S)

int
main(int argc, char *argv[])
{
	/* cpumem.po */
	setlocale(LC_ALL, "");
	bindtextdomain("GMCA", "/usr/lib/fm/dict");
	textdomain("GMCA");

	printf(_("GMCA", "ereport.cpu.intel.l0cache\n"));
	printf(_("GMCA", "GMCA-CACHE-01.class\n"));

	/* disk.po */
	setlocale(LC_ALL, "");
	bindtextdomain("DISK", "/usr/lib/fm/dict");
	textdomain("DISK");

	printf(_("DISK", "ereport.io.scsi.disk.predictive-failure\n"));
	printf(_("DISK", "DISK-SCSI-01.class\n"));

	/* ipmi.po */
	setlocale(LC_ALL, "");
	bindtextdomain("IPMI", "/usr/lib/fm/dict");
	textdomain("IPMI");

	printf(_("IPMI", "ereport.ipmi.cpu.temp-warn\n"));
	printf(_("IPMI", "IPMI-CPU-01.class\n"));
	
	/* mpio.po */
        setlocale(LC_ALL, "");
        bindtextdomain("MPIO", "/usr/lib/fm/dict");
        textdomain("MPIO");

        printf(_("MPIO", "ereport.io.mpio.failpath\n"));
        printf(_("MPIO", "MPIO-DM-01.class\n"));

	/* network.po */
        setlocale(LC_ALL, "");
        bindtextdomain("NETWORK", "/usr/lib/fm/dict");
        textdomain("NETWORK");

        printf(_("NETWORK", "ereport.io.network.init-pcidev-fail\n"));
        printf(_("NETWORK", "NETWORK-NIC-01.class\n"));

	/* pcie.po */
        setlocale(LC_ALL, "");
        bindtextdomain("PCIE", "/usr/lib/fm/dict");
        textdomain("PCIE");

        printf(_("PCIE", "ereport.io.pcie.\n"));
        printf(_("PCIE", "PCIE-AER-01.class\n"));

	/* service.po */
        setlocale(LC_ALL, "");
        bindtextdomain("SERVICE", "/usr/lib/fm/dict");
        textdomain("SERVICE");

        printf(_("SERVICE", "ereport.service.network.apache.http.network-unreachable\n"));
        printf(_("SERVICE", "SERVICE-APACHE-01.class\n"));

	/* topo.po */
        setlocale(LC_ALL, "");
        bindtextdomain("TOPO", "/usr/lib/fm/dict");
        textdomain("TOPO");

        printf(_("TOPO", "ereport.topo.cpu.hotadd\n"));
        printf(_("TOPO", "TOPO-CPU-01.class\n"));

	return 0;
}

