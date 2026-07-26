#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xe8213e80, "_printk" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0xab55ab6d, "usb_find_common_endpoints" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0xc9e0b93e, "usb_control_msg" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x645ed827, "usb_deregister" },
	{ 0xd272d446, "__fentry__" },
	{ 0x3fc0ae15, "usb_register_driver" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xe9196a28, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xe8213e80,
	0xcb8b6ec6,
	0xbd03ed67,
	0xab55ab6d,
	0xd710adbf,
	0xc9e0b93e,
	0xd272d446,
	0x645ed827,
	0xd272d446,
	0x3fc0ae15,
	0xd272d446,
	0xe9196a28,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"_printk\0"
	"kfree\0"
	"__ref_stack_chk_guard\0"
	"usb_find_common_endpoints\0"
	"__kmalloc_noprof\0"
	"usb_control_msg\0"
	"__stack_chk_fail\0"
	"usb_deregister\0"
	"__fentry__\0"
	"usb_register_driver\0"
	"__x86_return_thunk\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v10C4pEA60d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "9E8549C532A768B1A1333B5");
