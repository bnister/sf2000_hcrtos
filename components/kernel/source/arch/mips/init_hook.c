#include <generated/br2_autoconf.h>
#include <string.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <unistd.h>

static unsigned int cache_patch_read = 0;
typedef void (*func_ptr) (void);

extern func_ptr __CTOR_LIST__[];
extern func_ptr __DTOR_LIST__[];
extern void _start(void);
extern int OsKHeapInit(void);
extern int OsKMmzInit(void);
int __cpu_clock_hz = 0;

void __attribute__((weak)) hardware_init_hook(void)
{
	REG32_WRITE((uint32_t)&SYSIO0 + 0x38, 0);
	REG32_WRITE((uint32_t)&SYSIO0 + 0x3c, 0);
	REG32_CLR_BIT((uint32_t)&SYSIO0 + 0x40, BIT2);
	REG32_CLR_BIT((uint32_t)0xb8870000, BIT25);

#if defined(CONFIG_SYS_RST_WDT)
	REG32_SET_BIT((uint32_t)&SYSIO0 + 0x80, BIT23);
	usleep(1);
	REG32_CLR_BIT((uint32_t)&SYSIO0 + 0x80, BIT23);
#endif
	return;
}

#define usec2tick(usec) ((usec)*27 / 128)
void __attribute__ ((section(".standby.text"))) hw_watchdog_reset(unsigned long delay_us)
{
	void *wdt_addr = (void *)&WDT0;

	REG32_WRITE(wdt_addr + 4, 0);
	REG32_WRITE(wdt_addr, 0 - usec2tick(delay_us));
	REG32_WRITE(wdt_addr + 4, 0x26);
}

void __attribute__ ((section(".standby.text"))) hw_watchdog_disable(void)
{
	void *wdt_addr = (void *)&WDT0;
	REG32_WRITE(wdt_addr + 4, 0);
}

void __attribute__((weak)) ddr_256MB_space_mapping(void)
{
#if (defined CONFIG_CPU_MIPS32R1) && (defined CONFIG_SOC_HC16XX)
	REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x220, BIT24);
#else
	REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x220, BIT24);
#endif
}

void __attribute__((weak)) ddr_access_priority_init(void)
{
#ifdef CONFIG_SOC_HC16XX
	/* DE */
	*(volatile uint32_t *)0xb8801010 &= ~(1 << 22);
	*(volatile uint32_t *)0xb8801024 &= ~(0xff << 16);
	*(volatile uint32_t *)0xb8801024 |= (0x40 << 16);
	*(volatile uint32_t *)0xb880100c &= ~(0x3 << 28);
	*(volatile uint32_t *)0xb880100c |= (0x1 << 28);

	*(volatile uint32_t *)0xb8801010 &= ~(0x3 << 18);
	*(volatile uint32_t *)0xb8801020 &= ~(0xffff << 16);
	*(volatile uint32_t *)0xb8801020 |= (0x4040 << 16);
	*(volatile uint32_t *)0xb880100c &= ~(0xf << 20);
	*(volatile uint32_t *)0xb880100c |= (0xA << 20);

	/* VE */
	*(volatile uint32_t *)0xb8801010 &= ~(0x3 << 20);
	*(volatile uint32_t *)0xb8801024 &= ~(0xffff << 0);
	*(volatile uint32_t *)0xb8801024 |= (0xA0A0 << 0);
	*(volatile uint32_t *)0xb880100c &= ~(0xF << 24);
	*(volatile uint32_t *)0xb880100c |= (0xA << 24);
#elif defined(CONFIG_SOC_HC15XX)
	*(volatile uint32_t *)0xb880100c = 0xefff4055;
	*(volatile uint32_t *)0xb8801010 &= 0xffffffef;
	*(volatile uint32_t *)0xb8801070 &= 0xffff00ff;
	*(volatile uint32_t *)0xb8801070 |= 0x00002000;
#endif
}

static inline int scpu_clksel2clk(int sel)
{
	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
		switch (sel) {
		case 0:
			return 594 * 1000 * 1000;
		case 1:
			return 396 * 1000 * 1000;
		case 2:
			return 297 * 1000 * 1000;
		case 3:
		case 4:
		case 5:
		case 6:
			return 198 * 1000 * 1000;
		default:
			return 800 * 1000 * 1000;
		}
	} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		switch (sel) {
		case 0:
			return 594 * 1000 * 1000;
		case 1:
			return 450 * 1000 * 1000;
		case 2:
			return 396 * 1000 * 1000;
		case 3:
			return 297 * 1000 * 1000;
		case 4:
			return 198 * 1000 * 1000;
		case 5:
			return 24 * 1000 * 1000;
		default:
			return 800 * 1000 * 1000;
		}
	}

	return 800 * 1000 * 1000;
}

static inline int mcpu_clksel2clk(int sel)
{
	switch (sel) {
	case 0:
		return 594 * 1000 * 1000;
	case 1:
		return 396 * 1000 * 1000;
	case 2:
		return 297 * 1000 * 1000;
	case 3:
		return 198 * 1000 * 1000;
	case 4:
		return 900 * 1000 * 1000;
	case 5:
		return 1188 * 1000 * 1000;
	case 6:
		return 24 * 1000 * 1000;
	default:
		return 900 * 1000 * 1000;
	}

	return 900 * 1000 * 1000;
}

#define mips32_get_c0_register(source, sel)                                    \
	({                                                                     \
		unsigned int __res;                                            \
		if (sel == 0)                                                  \
			__asm__ __volatile__("mfc0\t%0, " #source "\n\t"       \
					     : "=r"(__res));                   \
		else                                                           \
			__asm__ __volatile__(".set\tmips32\n\t"                \
					     "mfc0\t%0, " #source ", " #sel    \
					     "\n\t"                            \
					     ".set\tmips0\n\t"                 \
					     : "=r"(__res));                   \
		__res;                                                         \
	})

#define read_c0_ebase() mips32_get_c0_register($15, 1)

uint32_t get_processor_id(void)
{
	return read_c0_ebase() & 0x3;
}

#define MHZ2MCTRL(x) (((((x) * 10) - 24) / 24) | 0x8000)
#define MHZ2MCTRL2(x) (((((x) * 10) - 27) / 27) | 0x8000)
void __attribute__((weak)) scpu_clock_init(void)
{
	int np;
	u32 clock = 0xffffffff;
	u32 scpu_dig_pll = 0xffffffff;

	if (get_processor_id() != 0)
		return;

	hw_watchdog_reset(50000);
	np = fdt_get_node_offset_by_path("/hcrtos/scpu");
	if (np >= 0) {
		fdt_get_property_u_32_index(np, "scpu-dig-pll-clk", 0,
					    &scpu_dig_pll);
		fdt_get_property_u_32_index(np, "clock", 0, &clock);
	}

	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1512) {
		if (scpu_dig_pll != 0xffffffff) {
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x380, 16, 16,
					 MHZ2MCTRL2(scpu_dig_pll));
		}
		if (clock != 0xffffffff) {
			__cpu_clock_hz = scpu_clksel2clk(clock);
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 8, 3,
					 clock);
		}
		if (clock == 7) {
			__cpu_clock_hz = scpu_dig_pll * 1000 * 1000;
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x7c, 7, 1, 1);
		}
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 22, 1, 1);
	} else if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) ==
		   0x1600) {
		if (scpu_dig_pll != 0xffffffff) {
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x3b0, 16, 16,
					 MHZ2MCTRL(scpu_dig_pll));
		}
		if (clock != 0xffffffff) {
			__cpu_clock_hz = scpu_clksel2clk(clock);
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x9c, 8, 3,
					 clock);
		}
		if (clock == 7) {
			__cpu_clock_hz = scpu_dig_pll * 1000 * 1000;
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x9c, 19, 1, 1);
		}
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 22, 1, 1);
	}
	hw_watchdog_disable();
}

void __attribute__((weak)) mcpu_clock_init(void)
{
	int np;
	u32 clock = 0xffffffff;
	u32 mcpu_dig_pll = 0xffffffff;

	if (get_processor_id() == 0)
		return;

	hw_watchdog_reset(50000);
	np = fdt_get_node_offset_by_path("/hcrtos/mcpu");
	if (np >= 0) {
		fdt_get_property_u_32_index(np, "mcpu-dig-pll-clk", 0,
					    &mcpu_dig_pll);
		fdt_get_property_u_32_index(np, "clock", 0, &clock);
	}

	if (REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		if (mcpu_dig_pll != 0xffffffff) {
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x380, 16, 16,
					 MHZ2MCTRL(mcpu_dig_pll));
		}
		if (clock != 0xffffffff) {
			__cpu_clock_hz = mcpu_clksel2clk(clock);
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 8, 3,
					 clock);
		}
		if (clock == 7) {
			__cpu_clock_hz = mcpu_dig_pll * 1000 * 1000;
			REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x7c, 7, 1, 1);
		}
		REG32_SET_FIELD2((uint32_t)&MSYSIO0 + 0x74, 22, 1, 1);
	}
	hw_watchdog_disable();
}

static int platform_setup_setbit(unsigned long node, const char *uname,
					int depth, void *data)
{
	char *type = (char *)fdt_get_property_data_by_name(node, "device_type", NULL);
	u32 i, nreg, base, offset, value;

	if (type == NULL) {
		/*
		 * The longtrail doesn't have a device_type on the
		 * /memory node, so look for the node called /memory@0.
		 */
		if (depth != 1 ||
		    strcmp(uname, "hichip,hcrtos-setup-setbit") != 0)
			return 0;
	} else if (strcmp(type, "hichip,hcrtos-setup-setbit") != 0)
		return 0;

	nreg = 0;

	if (fdt_get_property_data_by_name(node, "reg_bit", &nreg) == NULL)
		nreg = 0;
	nreg /= 12;
	if (nreg == 0)
		return 0;

	for (i = 0; i < nreg; i++) {
		fdt_get_property_u_32_index(node, "reg_bit", i * 3 + 0, &base);
		fdt_get_property_u_32_index(node, "reg_bit", i * 3 + 1, &offset);
		fdt_get_property_u_32_index(node, "reg_bit", i * 3 + 2, &value);
		if (value == 1) {
			void *caddr = (void *)MIPS_UNCACHED_ADDR(base);
			REG32_SET_BIT(caddr, BIT(offset));
		}
	}

	return 0;
}

static int platform_setup_setreg(unsigned long node, const char *uname,
				 int depth, void *data)
{
	char *type = (char *)fdt_get_property_data_by_name(node, "device_type", NULL);
	u32 i, nreg, base, value;

	if (type == NULL) {
		if (depth != 1 ||
		    strcmp(uname, "hichip,hcrtos-setup-setreg") != 0)
			return 0;
	} else if (strcmp(type, "hichip,hcrtos-setup-setreg") != 0)
		return 0;

	nreg = 0;

	if (fdt_get_property_data_by_name(node, "reg_value", &nreg) == NULL)
		nreg = 0;
	nreg /= 8;
	if (nreg == 0)
		return 0;

	for (i = 0; i < nreg; i++) {
		fdt_get_property_u_32_index(node, "reg_value", i * 2 + 0, &base);
		fdt_get_property_u_32_index(node, "reg_value", i * 2 + 1, &value);
		if (base != 0) {
			void *caddr = (void *)MIPS_UNCACHED_ADDR(base);
			REG32_WRITE(caddr, value);
		}
	}

	return 0;
}

unsigned int __attribute__((noinline)) write_sync(void)
{
	unsigned int ret = 0;

	if (cache_patch_read) {
		/* for the last write ops before this read ops */
		asm("   sync");
		ret = *(unsigned int *)cache_patch_read;
		asm("   sync");
	}

	return ret;
}

unsigned int __attribute__((weak)) is_amp(void)
{
	return 0;
}

/* Constructors are called in reverse order of the list. */
static void cpp_do_global_ctors(void)
{
	unsigned long nptrs = (unsigned long)__CTOR_LIST__[0];
	unsigned int i;

	if (nptrs == (unsigned long)-1) {
		for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++)
			;
	}

	for (i = nptrs; i >= 1; i--) {
		if (i == 2 || i == 1)
			continue;
		__CTOR_LIST__[i]();
	}
}

/* Destructors are called in forward order of the list. */
static void cpp_do_global_dtors(void)
{
	unsigned long nptrs = (unsigned long)__DTOR_LIST__[0];
	unsigned int i;

	if (nptrs == (unsigned long)-1) {
		for (nptrs = 0; __DTOR_LIST__[nptrs + 1] != 0; nptrs++)
			;
	}

	for (i = 1; i <= nptrs; i++) {
		__DTOR_LIST__[i]();
	}
}

void __attribute__((weak)) software_init_hook(void)
{
	cache_patch_read = MIPS_UNCACHED_ADDR((int)_start);
	ddr_256MB_space_mapping();
	ddr_access_priority_init();
	fdt_early_setup();
	scpu_clock_init();
	mcpu_clock_init();
	OsKHeapInit();
	OsKMmzInit();
	fdt_setup();

	of_scan_flat_dt(platform_setup_setbit, NULL);
	of_scan_flat_dt(platform_setup_setreg, NULL);

	cpp_do_global_ctors();
}
