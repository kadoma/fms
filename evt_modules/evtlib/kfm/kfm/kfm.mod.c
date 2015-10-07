#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x28950ef1, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x784213a6, __VMLINUX_SYMBOL_STR(pv_lock_ops) },
	{ 0x59a4f94d, __VMLINUX_SYMBOL_STR(genl_unregister_family) },
	{ 0xc8b57c27, __VMLINUX_SYMBOL_STR(autoremove_wake_function) },
	{ 0x8f2703b7, __VMLINUX_SYMBOL_STR(wbinvd_on_all_cpus) },
	{ 0xd067fc5c, __VMLINUX_SYMBOL_STR(proc_dointvec) },
	{ 0x4ed12f73, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xc35e4b4e, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0xf1db1704, __VMLINUX_SYMBOL_STR(nla_memcpy) },
	{ 0xb8c7ff88, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x42f90a31, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0xcafc72bc, __VMLINUX_SYMBOL_STR(__genl_register_family_with_ops) },
	{ 0xdc9c9463, __VMLINUX_SYMBOL_STR(nla_put) },
	{ 0x9abdea30, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x9f6e19ab, __VMLINUX_SYMBOL_STR(mem_section) },
	{ 0xf50069a6, __VMLINUX_SYMBOL_STR(netlink_unicast) },
	{ 0xc3bf75bc, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xc6cbbc89, __VMLINUX_SYMBOL_STR(capable) },
	{ 0x78764f4e, __VMLINUX_SYMBOL_STR(pv_irq_ops) },
	{ 0xaf3f0d3e, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0xac3d20e2, __VMLINUX_SYMBOL_STR(unregister_sysctl_table) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0x195c9f2c, __VMLINUX_SYMBOL_STR(kfree_skb) },
	{ 0xfb6af58d, __VMLINUX_SYMBOL_STR(recalc_sigpending) },
	{ 0x43261dca, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irq) },
	{ 0x910538ff, __VMLINUX_SYMBOL_STR(pv_cpu_ops) },
	{ 0xe65cdceb, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xb219d56c, __VMLINUX_SYMBOL_STR(wbinvd_on_cpu) },
	{ 0xcf21d241, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0x23ab0db8, __VMLINUX_SYMBOL_STR(genlmsg_put) },
	{ 0x5c8b5ce8, __VMLINUX_SYMBOL_STR(prepare_to_wait) },
	{ 0xd3719d59, __VMLINUX_SYMBOL_STR(paravirt_ticketlocks_enabled) },
	{ 0x32f9ef99, __VMLINUX_SYMBOL_STR(genl_register_mc_group) },
	{ 0xfa66f77c, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x5541ea93, __VMLINUX_SYMBOL_STR(on_each_cpu) },
	{ 0x6228c21f, __VMLINUX_SYMBOL_STR(smp_call_function_single) },
	{ 0x84c274f9, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0x8fe48b08, __VMLINUX_SYMBOL_STR(register_sysctl_paths) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "EA61F523AF3483EAD1996CF");
