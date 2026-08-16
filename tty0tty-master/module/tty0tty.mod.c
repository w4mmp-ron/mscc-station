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
	{ 0xb0957ffe, "down" },
	{ 0x3c592f4f, "up" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x0bd394d8, "tty_termios_baud_rate" },
	{ 0xdcb764ad, "memset" },
	{ 0x724e62d4, "sysfs_notify" },
	{ 0xc5dcfffb, "__tty_alloc_driver" },
	{ 0x67b27ec1, "tty_std_termios" },
	{ 0x4829a47e, "memcpy" },
	{ 0x6cdd97bd, "tty_port_init" },
	{ 0x2527739e, "tty_port_link_device" },
	{ 0x91f2499e, "tty_register_driver" },
	{ 0x358ca349, "kmalloc_caches" },
	{ 0x92997ed8, "_printk" },
	{ 0x8fab51e0, "tty_driver_kref_put" },
	{ 0xc9df4173, "__kmalloc_cache_noprof" },
	{ 0xf223eb25, "tty_register_device_attr" },
	{ 0xb01da031, "tty_unregister_device" },
	{ 0x51572ac7, "__tty_insert_flip_string_flags" },
	{ 0xad1e42cf, "tty_flip_buffer_push" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xe2964344, "__wake_up" },
	{ 0xf60c3135, "tty_port_destroy" },
	{ 0x07c67fc9, "tty_unregister_driver" },
	{ 0x037a0cba, "kfree" },
	{ 0x76c9fa2e, "tty_check_change" },
	{ 0x668b19a1, "down_read" },
	{ 0x53b954a2, "up_read" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x036cce78, "tty_termios_input_baud_rate" },
	{ 0xff8fc3d6, "tty_set_termios" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x1961fe94, "tty_unthrottle" },
	{ 0xaad8c7d6, "default_wake_function" },
	{ 0x4afb2238, "add_wait_queue" },
	{ 0xf3fe8045, "__tracepoint_sched_set_state_tp" },
	{ 0x01000e51, "schedule" },
	{ 0x37110088, "remove_wait_queue" },
	{ 0x53bbefe1, "__trace_set_current_state" },
	{ 0x3f934d30, "tty_driver_flush_buffer" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C65C31D2ED5FD58973628A0");
