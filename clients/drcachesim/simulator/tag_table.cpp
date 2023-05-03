#include <assert.h>

#include "tag_table.h"

#define align_floor_pow_2(x, b)     ((x) & (~((b) - 1)))
#define align_ceil_pow_2(x, b)      (((x) + ((b) - 1)) & (~((b) - 1)))
#define CAP_SIZE_BYTES              16

void
tag_table_t::update(const memref_t &memref)
{
    bool tag;
    addr_t memref_addr;
    size_t memref_size;
    if (type_is_instr(memref.instr.type)) {
        tag = false;

        memref_addr = memref.instr.addr;
        memref_size = memref.instr.size;
    } else if (memref.data.type == TRACE_TYPE_READ ||
        memref.data.type == TRACE_TYPE_WRITE || type_is_prefetch(memref.data.type)) {

        if (memref.data.type == TRACE_TYPE_READ && !memref.data.cap_access) {
            // do not know tags for non-capability loads
            assert(memref.data.tag_cheri == -1);
            return;
        }

        assert(memref.data.tag_cheri == 0 || memref.data.tag_cheri == 1);
        assert(memref.data.tag_cheri == 0 || memref.data.cap_access);
        tag = (memref.data.tag_cheri == 1);

        // not outputting prefetch instructions at the moment from QEMU / tracesim tool
        // just remove this assertion if you are
        assert(!type_is_prefetch(memref.data.type));

        memref_addr = memref.data.addr;
        memref_size = memref.data.size;
    } else {
        return;
    }

    addr_t start_addr = align_floor_pow_2(memref_addr, CAP_SIZE_BYTES);
    addr_t final_addr = align_ceil_pow_2(memref_addr + memref_size, CAP_SIZE_BYTES);

    for (addr_t addr = start_addr; addr < final_addr; addr += CAP_SIZE_BYTES) {
        assert(addr % CAP_SIZE_BYTES == 0);

        table_[addr] = tag;
    }
}

tag_cache_request_t
tag_table_t::get_miss_entry(const memref_t &memref, const int block_size) const
{
    addr_t memref_addr;
    size_t memref_size;
    uint8_t entry_type;
    if (type_is_instr(memref.instr.type)) {
        memref_addr = memref.instr.addr;
        memref_size = memref.instr.size;

        entry_type = TAG_CACHE_REQUEST_TYPE_READ;
    } else {
        assert(type_is_prefetch(memref.data.type) ||
            memref.data.type == TRACE_TYPE_READ || memref.data.type == TRACE_TYPE_WRITE);
        memref_addr = memref.data.addr;
        memref_size = memref.data.size;

        entry_type = memref.data.type == TRACE_TYPE_WRITE ?
            TAG_CACHE_REQUEST_TYPE_WRITE : TAG_CACHE_REQUEST_TYPE_READ;
    }

    assert(block_size > 0);
    assert(block_size % CAP_SIZE_BYTES == 0);

    assert(IS_POWER_OF_2(block_size));
    addr_t block_start_addr = align_floor_pow_2(memref_addr, block_size);

    // memref should refer to memory within one cache line (not straddle two cache lines)
    assert(align_ceil_pow_2(memref_addr + memref_size, block_size) - block_start_addr
        == (uint64_t) block_size);

    addr_t start_addr = align_floor_pow_2(memref_addr, CAP_SIZE_BYTES);
    addr_t final_addr = align_ceil_pow_2(memref_addr + memref_size, CAP_SIZE_BYTES);

    assert(final_addr - start_addr <= (uint64_t) block_size);

    tag_cache_request_t entry = {0};
    entry.type = entry_type;
    entry.size = block_size;
    // assuming use_physical option is used, else this address will be virtual
    entry.paddr = block_start_addr;

    uint16_t i = 0;
    for (addr_t addr = start_addr; addr < final_addr; addr += CAP_SIZE_BYTES, i++) {
        assert(i < sizeof(entry.tags) * 8);
        assert(i < block_size / CAP_SIZE_BYTES);

        assert(addr % CAP_SIZE_BYTES == 0);
        auto table_entry = table_.find(addr);
        if (table_entry != table_.end()) {
            assert((entry.tags & (1 << i)) == 0);
            assert((entry.tags_known & (1 << i)) == 0);

            bool tag = table_entry->second;
            if (tag)
                entry.tags |= (1 << i);
            entry.tags_known |= (1 << i);
        }
    }

    return entry;
}
