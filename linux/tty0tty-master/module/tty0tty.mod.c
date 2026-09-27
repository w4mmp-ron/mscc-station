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
	{ 0x7a4a235c, "__tty_alloc_driver" },
	{ 0xf4fa66aa, "tty_std_termios" },
	{ 0x92c5f7d6, "tty_port_init" },
	{ 0x73e445fa, "tty_port_link_device" },
	{ 0x99b8bcd2, "tty_register_driver" },
	{ 0xe8213e80, "_printk" },
	{ 0x9ff29899, "tty_driver_kref_put" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xc4fee520, "kmalloc_caches" },
	{ 0x4574d0c7, "__kmalloc_cache_noprof" },
	{ 0xf4289573, "tty_register_device_attr" },
	{ 0xaa5303d9, "tty_unregister_device" },
	{ 0x86891b00, "__tty_insert_flip_string_flags" },
	{ 0x894786aa, "tty_flip_buffer_push" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x8b881dc5, "const_current_task" },
	{ 0x2247bd2b, "default_wake_function" },
	{ 0xb730487b, "add_wait_queue" },
	{ 0x57fa0ee9, "__tracepoint_sched_set_state_tp" },
	{ 0xd272d446, "schedule" },
	{ 0xb730487b, "remove_wait_queue" },
	{ 0xb2e62cba, "__trace_set_current_state" },
	{ 0x5a844b26, "__x86_indirect_thunk_rdx" },
	{ 0x3b1bb331, "tty_unthrottle" },
	{ 0x3b1bb331, "tty_driver_flush_buffer" },
	{ 0x68a1b6c6, "__wake_up" },
	{ 0x92c5f7d6, "tty_port_destroy" },
	{ 0x9ff29899, "tty_unregister_driver" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xe804603d, "__init_waitqueue_head" },
	{ 0xd272d446, "__fentry__" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xd5c3f38c, "down" },
	{ 0xd5c3f38c, "up" },
	{ 0xdd6830c7, "sprintf" },
	{ 0x8e5094c8, "tty_termios_baud_rate" },
	{ 0xde3fd502, "sysfs_notify" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0x878b9234, "tty_check_change" },
	{ 0x8efcc8cd, "down_read" },
	{ 0x8efcc8cd, "up_read" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0x8e5094c8, "tty_termios_input_baud_rate" },
	{ 0x76f9d337, "tty_set_termios" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xe9196a28, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x7a4a235c,
	0xf4fa66aa,
	0x92c5f7d6,
	0x73e445fa,
	0x99b8bcd2,
	0xe8213e80,
	0x9ff29899,
	0xbd03ed67,
	0xc4fee520,
	0x4574d0c7,
	0xf4289573,
	0xaa5303d9,
	0x86891b00,
	0x894786aa,
	0x092a35a2,
	0x8b881dc5,
	0x2247bd2b,
	0xb730487b,
	0x57fa0ee9,
	0xd272d446,
	0xb730487b,
	0xb2e62cba,
	0x5a844b26,
	0x3b1bb331,
	0x3b1bb331,
	0x68a1b6c6,
	0x92c5f7d6,
	0x9ff29899,
	0xcb8b6ec6,
	0xe804603d,
	0xd272d446,
	0xd272d446,
	0xd5c3f38c,
	0xd5c3f38c,
	0xdd6830c7,
	0x8e5094c8,
	0xde3fd502,
	0x90a48d82,
	0xbd03ed67,
	0x878b9234,
	0x8efcc8cd,
	0x8efcc8cd,
	0x092a35a2,
	0x8e5094c8,
	0x76f9d337,
	0xd272d446,
	0xe9196a28,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__tty_alloc_driver\0"
	"tty_std_termios\0"
	"tty_port_init\0"
	"tty_port_link_device\0"
	"tty_register_driver\0"
	"_printk\0"
	"tty_driver_kref_put\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"tty_register_device_attr\0"
	"tty_unregister_device\0"
	"__tty_insert_flip_string_flags\0"
	"tty_flip_buffer_push\0"
	"_copy_to_user\0"
	"const_current_task\0"
	"default_wake_function\0"
	"add_wait_queue\0"
	"__tracepoint_sched_set_state_tp\0"
	"schedule\0"
	"remove_wait_queue\0"
	"__trace_set_current_state\0"
	"__x86_indirect_thunk_rdx\0"
	"tty_unthrottle\0"
	"tty_driver_flush_buffer\0"
	"__wake_up\0"
	"tty_port_destroy\0"
	"tty_unregister_driver\0"
	"kfree\0"
	"__init_waitqueue_head\0"
	"__fentry__\0"
	"__x86_return_thunk\0"
	"down\0"
	"up\0"
	"sprintf\0"
	"tty_termios_baud_rate\0"
	"sysfs_notify\0"
	"__ubsan_handle_out_of_bounds\0"
	"__ref_stack_chk_guard\0"
	"tty_check_change\0"
	"down_read\0"
	"up_read\0"
	"_copy_from_user\0"
	"tty_termios_input_baud_rate\0"
	"tty_set_termios\0"
	"__stack_chk_fail\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C65C31D2ED5FD58973628A0");
