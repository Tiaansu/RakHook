#include "RakHook/rakhook.hpp"
#include "RakHook/detail.hpp"
#include "RakHook/offsets.hpp"
#include "RakHook/hooked_rakclient_interface.hpp"

#include "RakNet/PacketEnumerations.h"

#include <polyhook2/Detour/x86Detour.hpp>

#ifndef MAX_ALLOCA_STACK_ALLOCATION
#define MAX_ALLOCA_STACK_ALLOCATION 1048576
#endif

hooked_rakclient_interface *hooked_interface = nullptr;
void                       *rakpeer          = nullptr;
PlayerID                    gplayerid;

using destroy_ri_t        = void(__cdecl *)(void *);
using handle_rpc_packet_t = bool(__fastcall *)(void *, const char *, int, PlayerID);

std::uint64_t destroy_rakclient_interface_orig = 0;
std::uint64_t handle_rpc_packet_orig           = 0;

// callbacks
void __cdecl destroy_rakclient_interface(void *rakclient_interface) {
    if (rakclient_interface == hooked_interface) {
        rakclient_interface = rakhook::orig;
        delete hooked_interface;
    }
    return ((void(*)(void*))destroy_rakclient_interface_orig)(rakclient_interface);
}

bool __fastcall handle_rpc_packet(void *rp, void *trash, const char *data, int length, PlayerID playerid) {
    rakpeer   = rp;
    gplayerid = playerid;

    RakNet::BitStream                  incoming{std::bit_cast<unsigned char *>(const_cast<char *>(data)), static_cast<unsigned int>(length), true};
    unsigned char                      id        = 0;
    unsigned char                     *input     = nullptr;
    unsigned int                       bits_data = 0;
    std::shared_ptr<RakNet::BitStream> callback_bs{std::make_shared<RakNet::BitStream>()};

    incoming.IgnoreBits(8);
    if (data[0] == ID_TIMESTAMP)
        incoming.IgnoreBits(8 * (sizeof(RakNetTime) + sizeof(unsigned char)));

    int offset = incoming.GetReadOffset();
    incoming.Read(id);

    if (!incoming.ReadCompressed(bits_data))
        return false;

    if (bits_data) {
        bool used_alloca = false;
        if (BITS_TO_BYTES(incoming.GetNumberOfUnreadBits()) < MAX_ALLOCA_STACK_ALLOCATION) {
            input       = std::bit_cast<unsigned char *>(alloca(BITS_TO_BYTES(incoming.GetNumberOfUnreadBits())));
            used_alloca = true;
        } else
            input = new unsigned char[BITS_TO_BYTES(incoming.GetNumberOfUnreadBits())];

        if (!incoming.ReadBits(input, bits_data, false)) {
            if (!used_alloca)
                delete[] input;

            return false;
        }

        callback_bs = std::make_shared<RakNet::BitStream>(input, BITS_TO_BYTES(bits_data), true);

        if (!used_alloca)
            delete[] input;
    }

    for (auto it = rakhook::on_receive_rpc.begin(); it != rakhook::on_receive_rpc.end();) {
        if (auto f = *it) {
            if (!f(id, callback_bs.get()))
                return false;
            it++;
        } else {
            it = rakhook::on_receive_rpc.erase(it);
        }
    }

    incoming.SetWriteOffset(offset);
    incoming.Write(id);
    bits_data = BYTES_TO_BITS(callback_bs->GetNumberOfBytesUsed());
    incoming.WriteCompressed(bits_data);
    if (bits_data)
        incoming.WriteBits(callback_bs->GetData(), bits_data, false);

    return ((bool (__fastcall*)(void*, void*, const char*, int, PlayerID))handle_rpc_packet_orig)(rp, trash, std::bit_cast<char *>(incoming.GetData()), incoming.GetNumberOfBytesUsed(), playerid);
}

std::unique_ptr<PLH::x86Detour> destroy_ri_hook;
std::unique_ptr<PLH::x86Detour> handle_rpc_hook;

bool rakhook::initialize() {
    if (initialized)
        return true;

    if (!samp_addr())
        return false;
    const uintptr_t samp_info = *std::bit_cast<uintptr_t *>(offsets::samp_info(true));
    if (!samp_info)
        return false;

    auto **rakclient_interface = std::bit_cast<RakClientInterface **>(samp_info + offsets::rakclient_interface());
    if (!*rakclient_interface)
        return false;

    orig                 = *rakclient_interface;
    hooked_interface     = new hooked_rakclient_interface(orig);
    *rakclient_interface = std::bit_cast<RakClientInterface *>(hooked_interface);

    if (!static_cast<bool>(destroy_ri_hook)) {
        auto func       = std::bit_cast<destroy_ri_t>(offsets::destroy_interface(true));
        destroy_ri_hook = std::make_unique<PLH::x86Detour>((std::uint64_t)func, (std::uint64_t)&destroy_rakclient_interface, &destroy_rakclient_interface_orig);
    }
    if (!static_cast<bool>(handle_rpc_hook)) {
        auto func       = std::bit_cast<handle_rpc_packet_t>(offsets::handle_rpc_packet(true));
        handle_rpc_hook = std::make_unique<PLH::x86Detour>((std::uint64_t)func, (std::uint64_t)&handle_rpc_packet, &handle_rpc_packet_orig);
    }

    destroy_ri_hook->hook();
    handle_rpc_hook->hook();

    initialized = true;
    return true;
}

void rakhook::destroy() {
    if (!initialized)
        return;

    const uintptr_t samp_info = *std::bit_cast<uintptr_t *>(samp_addr(offsets::samp_info()));
    if (!samp_info)
        return;

    auto **rakclient_interface = std::bit_cast<RakClientInterface **>(samp_info + offsets::rakclient_interface());
    *rakclient_interface       = orig;

    destroy_ri_hook.reset();
    handle_rpc_hook.reset();
    delete hooked_interface;
}

bool rakhook::send(RakNet::BitStream *bs, PacketPriority priority, PacketReliability reliability, char ord_channel) {
    if (!initialized)
        return false;
    return orig->Send(bs, priority, reliability, ord_channel);
}

bool rakhook::send_rpc(int id, RakNet::BitStream *bs, PacketPriority priority, PacketReliability reliability, char ord_channel, bool sh_timestamp) {
    if (!initialized)
        return false;
    return orig->RPC(&id, bs, priority, reliability, ord_channel, sh_timestamp);
}

bool rakhook::emul_rpc(unsigned char id, RakNet::BitStream &rpc_bs) {
    if (!initialized || !rakpeer)
        return false;

    RakNet::BitStream bs;
    bs.Write<unsigned char>(ID_RPC);
    bs.Write(id);
    bs.WriteCompressed<unsigned int>(BYTES_TO_BITS(rpc_bs.GetNumberOfBytesUsed()));
    bs.WriteBits(rpc_bs.GetData(), BYTES_TO_BITS(rpc_bs.GetNumberOfBytesUsed()), false);

    handle_rpc_packet_t handle_rpc;
    if (handle_rpc_hook && handle_rpc_packet_orig) {
        handle_rpc = std::bit_cast<handle_rpc_packet_t>((void*)handle_rpc_packet_orig);
    } else {
        handle_rpc = std::bit_cast<handle_rpc_packet_t>(offsets::handle_rpc_packet(true));
    }
    return handle_rpc(rakpeer, std::bit_cast<char *>(bs.GetData()), bs.GetNumberOfBytesUsed(), gplayerid);
}

bool rakhook::emul_packet(RakNet::BitStream &pbs) {
    if (!initialized || !rakpeer)
        return false;
    Packet *send_packet = std::bit_cast<Packet *(*)(size_t)>(samp_addr(offsets::alloc_packet()))(pbs.GetNumberOfBytesUsed());
    memcpy(send_packet->data, pbs.GetData(), send_packet->length);

    // RakPeer::AddPacketToProducer
    char *packets      = static_cast<char *>(rakpeer) + offsets::offset_packets();
    auto  write_lock   = std::bit_cast<Packet **(__thiscall *)(void *)>(samp_addr(offsets::write_lock()));
    auto  write_unlock = std::bit_cast<void(__thiscall *)(void *)>(samp_addr(offsets::write_unlock()));

    *write_lock(packets) = send_packet;
    write_unlock(packets);

    return true;
}