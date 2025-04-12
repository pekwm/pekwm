/*
 * pekwm_panel_sysinfo.c for pekwm
 * Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
 *
 * This program is licensed under the GNU GPL.
 * See the LICENSE file for more information.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* time-conversion constants. */
#define SEC_PER_YEAR 31536000
#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

/* platform independent system information data. */
struct pekwm_panel_sysinfo {
	unsigned long uptime;
	float load1;
	float load5;
	float load15;
	unsigned long ram_kb;
	unsigned long free_ram_kb;
	unsigned long cache_ram_kb;
	unsigned long swap_kb;
	unsigned long free_swap_kb;
	unsigned short num_procs;
};

#if defined(__linux__) || defined(UNITTEST)

struct linux_meminfo_map {
	const char *key;
	unsigned long *value;
};

static void
_linux_read_proc_meminfo(struct pekwm_panel_sysinfo *info, FILE *fp)
{
	const char *expected_end = " kB\n";

	struct linux_meminfo_map mi_map[] = {
		{"MemTotal", &info->ram_kb},
		{"MemFree", &info->free_ram_kb},
		{"Cached", &info->cache_ram_kb},
		{"SwapTotal", &info->swap_kb},
		{"SwapFree", &info->free_swap_kb},
		{NULL, NULL}
	};

	char line[64];
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *pos = strchr(line, ':');
		if (pos == NULL) {
			continue;
		}
		*pos = '\0';

		char *endptr;
		long value = strtol(pos + 1, &endptr, 10);
		if (strcmp(endptr, expected_end) == 0) {
			for (int i = 0; mi_map[i].key; i++) {
				if (strcmp(mi_map[i].key, line) == 0) {
					*mi_map[i].value = value;
					break;
				}
			}
		}
	}
}

#endif /* defined(__linux__) || defined(UNITTEST) */

#ifdef __linux__

#include <sys/sysinfo.h>
#include <stdlib.h>
#include <string.h>

static unsigned long
_to_kb(unsigned int mem_unit, unsigned long value)
{
	return (value * mem_unit) / 1024;
}

static float
_to_load(unsigned long value)
{
	return (float)value / (1 << SI_LOAD_SHIFT);
}

static int
_read_sysinfo(struct pekwm_panel_sysinfo *info)
{
	struct sysinfo sinfo;
	int err = sysinfo(&sinfo);
	if (err) {
		return err;
	}

	info->uptime = sinfo.uptime;
	info->load1 = _to_load(sinfo.loads[0]);
	info->load5 = _to_load(sinfo.loads[1]);
	info->load15 = _to_load(sinfo.loads[2]);
	info->ram_kb = _to_kb(sinfo.mem_unit, sinfo.totalram);
	info->free_ram_kb = _to_kb(sinfo.mem_unit, sinfo.freeram);
	info->cache_ram_kb = _to_kb(sinfo.mem_unit, sinfo.bufferram);
	info->swap_kb = _to_kb(sinfo.mem_unit, sinfo.totalswap);
	info->free_swap_kb = _to_kb(sinfo.mem_unit, sinfo.freeswap);
	info->num_procs = sinfo.procs;

	/* read after sysinfo, overwriting information from sysinfo with
	 * /proc/meminfo information if /proc is available. */
	FILE *fp = fopen("/proc/meminfo", "r");
	if (fp) {
		_linux_read_proc_meminfo(info, fp);
		fclose(fp);
	}

	return 0;
}

#else /* ! __linux__ */
#ifdef __sun__

#include <sys/stat.h>
#include <sys/swap.h>
#include <kstat.h>

static long _pagesize_kb = 0;

static float
_to_load(unsigned long value)
{
	return (float)value / 256;
}

static kstat_t*
_kstat_lookup_read(kstat_ctl_t *kc, char *module, int id, char *name)
{
	kstat_t *ksp = kstat_lookup(kc, module, id, name);
	if (ksp) {
		kid_t id = kstat_read(kc, ksp, NULL);
		if (id != -1) {
			return ksp;
		}
	}
	return NULL;
}

static uint32_t
_kstat_data_ui32(kstat_t *ksp, char *name)
{
	kstat_named_t *val = kstat_data_lookup(ksp, name);
	if (val && val->data_type == KSTAT_DATA_UINT32) {
		return val->value.ui32;
	}
	return 0;
}

static uint64_t
_kstat_data_ui64(kstat_t *ksp, char *name)
{
	kstat_named_t *val = kstat_data_lookup(ksp, name);
	if (val && val->data_type == KSTAT_DATA_UINT64) {
		return val->value.ui64;
	}
	return 0;
}

static void
_read_sysinfo_kstat(struct pekwm_panel_sysinfo *info, kstat_ctl_t *kc)
{
	kstat_t *ksp;
	ksp = _kstat_lookup_read(kc, "unix", 0, "system_misc");
	if (ksp) {
		kstat_named_t *val;
		time_t boot_time = _kstat_data_ui32(ksp, "boot_time");
		if (boot_time != 0) {
			info->uptime = time(NULL) - boot_time;
		}
		info->load1 = _to_load(_kstat_data_ui32(ksp, "avenrun_1min"));
		info->load5 = _to_load(_kstat_data_ui32(ksp, "avenrun_5min"));
		info->load15 = _to_load(_kstat_data_ui32(ksp, "avenrun_15min"));
		info->num_procs = _kstat_data_ui32(ksp, "nproc");

	}
	ksp = _kstat_lookup_read(kc, "unix", 0, "system_pages");
	if (ksp) {
		info->ram_kb =
			_kstat_data_ui64(ksp, "physmem") * _pagesize_kb;
		info->free_ram_kb =
			_kstat_data_ui64(ksp, "pagesfree") * _pagesize_kb;
	}
}

static int
_read_sysinfo(struct pekwm_panel_sysinfo *info)
{
	if (_pagesize_kb == 0) {
		_pagesize_kb = sysconf(_SC_PAGESIZE) / 1024;
	}

	kstat_ctl_t *kc = kstat_open();
	if (kc == NULL) {
		perror("ERROR: kstat_open: ");
		return -1;
	}

	_read_sysinfo_kstat(info, kc);

	/* not using kstat to read unix:0:vminfo:swap_avail and
	 * unix:0:vminfo:swap_free, seemingly raw stats that I have not
	 * figured out how to read, use swapctl instead. */
	swaptbl_t *swt = malloc(sizeof(swaptbl_t) + sizeof(swapent_t) * 16);
	if (swt) {
		swt->swt_n = 16;
		int n = swapctl(SC_LIST, swt);
		for (int i = 0; i < n; i++) {
			swapent_t *ste = &swt->swt_ent[i];
			info->swap_kb += ste->ste_pages * _pagesize_kb;
			info->free_swap_kb += ste->ste_free * _pagesize_kb;
		}
		free(swt);
	}

	/* needs to be kept aorund for the swapctl call */
	kstat_close(kc);

	return 0;
}

#else /* !__sun__ */
#ifdef __NetBSD__

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include <sys/vmmeter.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

static int
_read_sysinfo(struct pekwm_panel_sysinfo *info)
{
	int mib[6];
	size_t size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	struct timespec boottime = {0};
	size = sizeof(boottime);
	if (! sysctl(mib, 2, &boottime, &size, NULL, 0)) {
		info->uptime = time(NULL) - boottime.tv_sec;
	}

	mib[1] = KERN_PROC2;
	mib[2] = KERN_PROC_ALL;
	mib[3] = 0; /* process selector, unused */
	mib[4] = sizeof(struct kinfo_proc2);
	mib[5] = 0; /* number of kinfo_proc2 to return */
	size = 0;
	if (! sysctl(mib, 6, NULL, &size, NULL, 0)) {
		info->num_procs = size / sizeof(struct kinfo_proc2);
	}

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM64;
	uint64_t physmem = 0;
	size = sizeof(physmem);
	if (! sysctl(mib, 2, &physmem, &size, NULL, 0)) {
		info->ram_kb = physmem / 1024;
	}

	struct uvmexp uvmexp = {0};
	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP;
	size = sizeof(uvmexp);
	if (! sysctl(mib, 2, &uvmexp, &size, NULL, 0)) {
		int pagesize_kb = uvmexp.pagesize / 1024;
		info->swap_kb = uvmexp.swpages * pagesize_kb;
		info->free_swap_kb =
			info->swap_kb - (uvmexp.swpgonly * pagesize_kb);

		mib[1] = VM_METER;
		struct vmtotal vmtotal = {0};
		size = sizeof(vmtotal);
		if (! sysctl(mib, 2, &vmtotal, &size, NULL, 0)) {
			info->free_ram_kb = vmtotal.t_free * pagesize_kb;
		}
	}

	mib[1] = VM_LOADAVG;
	struct loadavg loadavg = {0};
	size = sizeof(loadavg);
	if (! sysctl(mib, 2, &loadavg, &size, NULL, 0)) {
		info->load1 = ((float)loadavg.ldavg[0]) / loadavg.fscale;
		info->load5 = ((float)loadavg.ldavg[1]) / loadavg.fscale;
		info->load15 = ((float)loadavg.ldavg[2]) / loadavg.fscale;
	}

	return 0;
}

#else /* !__NetBSD__ */
#ifdef __FreeBSD__

#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/vmmeter.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

static int _pagesize_kb = 0;

static int
_read_sysinfo(struct pekwm_panel_sysinfo *info)
{
	if (_pagesize_kb == 0) {
		_pagesize_kb = sysconf(_SC_PAGESIZE) / 1024;
	}

	double loadavg[3];
	if (getloadavg(loadavg, 3) == 3) {
		info->load1 = loadavg[0];
		info->load5 = loadavg[1];
		info->load15 = loadavg[2];
	}

	int mib[6];
	size_t len;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	struct timeval boottime = {0};
	len = sizeof(boottime);
	if (! sysctl(mib, 2, &boottime, &len, NULL, 0)) {
		info->uptime = time(NULL) - boottime.tv_sec;
	}

	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ALL;
	len = 0;
	if (sysctl(mib, 3, NULL, &len, NULL, 0) >= 0) {
		info->num_procs = len / sizeof(struct kinfo_proc);
	}

	info->ram_kb = sysconf(_SC_PHYS_PAGES) * _pagesize_kb;

	mib[0] = CTL_VM;
	mib[1] = VM_TOTAL;
	struct vmtotal vmt;
	len = sizeof(vmt);
	if (! sysctl(mib, 2, &vmt, &len, NULL, 0)) {
		info->free_ram_kb = vmt.t_free * _pagesize_kb;
	}

	// info->swap_kb  = 0;
	// info->free_swap_kb  = 0;

	return 0;
}

#else /* !__FreeBSD__ */
#ifdef __OpenBSD__

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/vmmeter.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

static int
_read_sysinfo(struct pekwm_panel_sysinfo *info)
{
	size_t size;
	int mib[6];
	int pagesize_kb = sysconf(_SC_PAGESIZE) / 1024;

	double loadavg[3];
	if (getloadavg(loadavg, 3) == 3) {
		info->load1 = loadavg[0];
		info->load5 = loadavg[1];
		info->load15 = loadavg[2];
	}

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	struct timespec boottime = {0};
	size = sizeof(boottime);
	if (! sysctl(mib, 2, &boottime, &size, NULL, 0)) {
		info->uptime = time(NULL) - boottime.tv_sec;
	}

	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ALL;
	mib[3] = 0; /* process selector, unused */
	mib[4] = sizeof(struct kinfo_proc);
	mib[5] = 0; /* number of kinfo_proc to return */
	size = 0;
	if (! sysctl(mib, 6, NULL, &size, NULL, 0)) {
		info->num_procs = size / sizeof(struct kinfo_proc);
	}

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM64;
	uint64_t physmem = 0;
	size = sizeof(physmem);
	if (! sysctl(mib, 2, &physmem, &size, NULL, 0)) {
		info->ram_kb = physmem / 1024;
	}

	struct uvmexp uvmexp;
	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP;
	size = sizeof(uvmexp);
	if (! sysctl(mib, 2, &uvmexp, &size, NULL, 0)) {
		int pagesize_kb = uvmexp.pagesize / 1024;
		info->swap_kb = uvmexp.swpages * pagesize_kb;
		info->free_swap_kb =
			info->swap_kb - (uvmexp.swpgonly * pagesize_kb);

		mib[0] = CTL_VM;
		mib[1] = VM_METER;

		struct vmtotal vmtotal;
		size = sizeof(vmtotal);
		if (! sysctl(mib, 2, &vmtotal, &size, NULL, 0)) {
			info->free_ram_kb = vmtotal.t_free * pagesize_kb;
		}
	}

	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = VFS_BCACHESTAT;
	struct bcachestats bcstats;
	size = sizeof(bcstats);
	if (! sysctl(mib, 3, &bcstats, &size, NULL, 0)) {
		info->cache_ram_kb = bcstats.numbufpages * pagesize_kb;
	}

	return 0;
}

#else /* ! __OpenBSD__ */

static int
_read_sysinfo(struct pekwm_panel_sysinfo *info)
{
	fprintf(stderr, "ERROR: unsupported platform\n");
	return 1;
}

#endif /* __OpenBSD__ */
#endif /* __FreeBSD__ */
#endif /* __NetBSD__ */
#endif /* __sun__ */
#endif /* __linux__ */

static unsigned int
_to_percent(unsigned long total, unsigned long avail)
{
	if (total == 0.0) {
		return 0;
	}
	unsigned long used = total - avail;
	return (int) (((float)used / total) * 100.0);
}

static void
_to_elapsed_time(unsigned long seconds)
{
	unsigned long s_per[] = {
		SEC_PER_YEAR, SEC_PER_DAY, SEC_PER_HOUR, SEC_PER_MIN
	};
	const char c_per[] = { 'y', 'd', 'h', 'm' };
	for (size_t i = 0; i < sizeof(s_per)/sizeof(s_per[0]); i++) {
		if (seconds > s_per[i]) {
			int per = seconds / s_per[i];
			seconds %= s_per[i];
			printf("%d%c", per, c_per[i]);
		}
	}
	printf("%lus", seconds);
}

static void
_print_sysinfo(struct pekwm_panel_sysinfo *info)
{
	printf("sysinfo_uptime %lu\n", info->uptime);
	printf("sysinfo_uptime_hr ");
	_to_elapsed_time(info->uptime);
	printf("\n");
	printf("sysinfo_load1 %.2f\n", info->load1);
	printf("sysinfo_load5 %.2f\n", info->load5);
	printf("sysinfo_load15 %.2f\n", info->load15);
	printf("sysinfo_mem_total %lu\n", info->ram_kb);
	printf("sysinfo_mem_free %lu\n", info->free_ram_kb);
	unsigned long free_ram_ec_kb = info->free_ram_kb + info->cache_ram_kb;
	printf("sysinfo_mem_free_ec %lu\n", free_ram_ec_kb);
	printf("sysinfo_mem_percent %u\n",
	       _to_percent(info->ram_kb, info->free_ram_kb));
	printf("sysinfo_mem_percent_ec %u\n",
	       _to_percent(info->ram_kb, free_ram_ec_kb));
	printf("sysinfo_mem_cache %lu\n", info->cache_ram_kb);
	printf("sysinfo_mem_cache_percent %u\n",
	       _to_percent(info->ram_kb, info->ram_kb - info->cache_ram_kb));
	printf("sysinfo_swap_total %lu\n", info->swap_kb);
	printf("sysinfo_swap_free %lu\n", info->free_swap_kb);
	printf("sysinfo_swap_percent %u\n",
	       _to_percent(info->swap_kb, info->free_swap_kb));
	printf("sysinfo_numproc %u\n", info->num_procs);
}

#ifndef UNITTEST

int
main(int argc, char *argv[])
{
	struct pekwm_panel_sysinfo info = {0};
	int err = _read_sysinfo(&info);
	if (! err) {
		_print_sysinfo(&info);
	}
	return 0;
}

#endif /* UNITTEST */
