#pragma once
#include "core/block.hpp"
#include <map>
#include <cstddef>

namespace bsl {
enum class ReceiveStatus { accepted, pending_parent, duplicate, invalid };
struct ChainUpdate {
    std::string connected_hash;
    bool fork = false; // A second or later child of an already connected parent.
    std::string old_tip, new_tip;
    std::uint64_t detached_depth = 0;
};
struct ReceiveResult {
    ReceiveStatus status;
    std::vector<ChainUpdate> updates;
};
class BlockTree {
public:
    explicit BlockTree(std::size_t capacity = 100000);
    ReceiveResult receive(const Block& block);
    const Block& tip() const;
    const Block& at(const std::string& hash) const;
    bool contains(const std::string& hash) const { return entries_.count(hash) != 0; }
    std::uint64_t cumulative_work(const std::string& hash) const;
    std::vector<std::string> active_chain() const;
    std::size_t known_count() const { return entries_.size(); }
    std::size_t pending_count() const { return pending_.size(); }
    std::size_t stale_count() const { return entries_.size() - active_chain().size(); }
private:
    struct Entry { Block block; std::uint64_t work; std::size_t children = 0; };
    std::map<std::string, Entry> entries_;
    std::map<std::string, Block> pending_;
    std::string tip_;
    std::size_t capacity_;
    bool follows_parent(const Block& block) const;
    ChainUpdate connect(const Block& block);
    std::uint64_t detached_depth(std::string old_tip, std::string new_tip) const;
};
std::uint64_t block_work(unsigned difficulty);
}
