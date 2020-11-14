#pragma once
#include <ctype.h>

namespace rakhook::offsets {
	namespace v037r1 {
		constexpr uintptr_t samp_info           = 0x21a0f8;
		constexpr uintptr_t rakclient_interface = 0x3c9;

		constexpr uintptr_t destroy_interface = 0x342d0;
		constexpr uintptr_t handle_rpc_packet = 0x372f0;

		constexpr uintptr_t alloc_packet   = 0x347e0;
		constexpr uintptr_t offset_packets = 0xdb6;
		constexpr uintptr_t write_lock     = 0x35b10;
		constexpr uintptr_t write_unlock   = 0x35b50;
	};

	namespace v037r3 {
		constexpr uintptr_t samp_info           = 0x26e8dc;
		constexpr uintptr_t rakclient_interface = 0x2c;

		constexpr uintptr_t destroy_interface = 0x37680;
		constexpr uintptr_t handle_rpc_packet = 0x3a6a0;

		constexpr uintptr_t alloc_packet   = 0x37b90;
		constexpr uintptr_t offset_packets = 0xdb6;
		constexpr uintptr_t write_lock     = 0x38ec0;
		constexpr uintptr_t write_unlock   = 0x38f00;
	};

	namespace v037r4 {
		constexpr uintptr_t samp_info           = 0x26ea0c;
		constexpr uintptr_t rakclient_interface = 0x2c;

		constexpr uintptr_t destroy_interface = 0x37d70;
		constexpr uintptr_t handle_rpc_packet = 0x3ad90;

		constexpr uintptr_t alloc_packet   = 0x38280;
		constexpr uintptr_t offset_packets = 0xdb6;
		constexpr uintptr_t write_lock     = 0x395b0;
		constexpr uintptr_t write_unlock   = 0x395f0;
	};
};