#include "lz4_file_reader.h"

/* clang-format off */ /* (make vera++ newline-after-type check happy) */
template <>
/* clang-format on */
file_reader_t<lz4_istream_t *>::~file_reader_t<lz4_istream_t *>()
{
    for (auto lz4_stream : input_files_)
        delete lz4_stream;
    delete[] thread_eof_;
}

template <>
bool
file_reader_t<lz4_istream_t *>::open_single_file(const std::string &path)
{
    lz4_istream_t *lz4_istream = new lz4_istream_t(path);
    if (!*lz4_istream)
        return false;
    VPRINT(this, 1, "Opened input file %s\n", path.c_str());
    input_files_.push_back(lz4_istream);
    return true;
}

template <>
bool
file_reader_t<lz4_istream_t *>::read_next_thread_entry(size_t thread_index,
                                                       OUT trace_entry_t *entry,
                                                       OUT bool *eof)
{
    if (!input_files_[thread_index]->read((char *) entry, sizeof(*entry))) {
        *eof = input_files_[thread_index]->eof();
        return false;
    }
    VPRINT(this, 4, "Read from thread #%zd file: type=%d, size=%d, addr=%zu\n",
           thread_index, entry->type, entry->size, entry->addr);
    return true;
}

template <>
bool
file_reader_t<lz4_istream_t *>::is_complete()
{
    // Not supported.
    return false;
}
