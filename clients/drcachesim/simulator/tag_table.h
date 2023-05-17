#include <unordered_map>
#include <stdbool.h>

#include "memref.h"

#ifndef _TAG_TABLE_H_
#define _TAG_TABLE_H_ 1

enum tag_cache_request_type_t
{
    TAG_CACHE_REQUEST_TYPE_READ,
    TAG_CACHE_REQUEST_TYPE_WRITE
};

typedef struct tag_cache_request_t tag_cache_request_t;
struct tag_cache_request_t
{
    uint8_t type;
    uint16_t size; // just the cache line size
    uint16_t tags; // number of tags depends on cache line size
    uint16_t tags_known; // mask indicating which tags are actually known
    uint64_t addr;
};

class tag_table_t {
public:
    void
    update(const memref_t &memref);

    tag_cache_request_t
    get_write_back_entry(const addr_t block_addr, const int block_size) const;

    tag_cache_request_t
    get_miss_entry(const memref_t &memref, const int block_size) const;

private:
    std::unordered_map<addr_t, bool> table_;
    int8_t current_tag_ = -1;
};

#endif /* _TAG_TABLE_H_ */
