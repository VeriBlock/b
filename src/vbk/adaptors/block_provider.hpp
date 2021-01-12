// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP

#include <dbwrapper.h>
#include <vbk/adaptors/block_batch_adaptor.hpp>
#include <vbk/adaptors/block_iterator.hpp>

#include "veriblock/storage/block_provider.hpp"

namespace VeriBlock {

template <typename BlockT>
struct BlockProvider : public altintegration::BlockProvider<BlockT> {
    using hash_t = typename BlockT::hash_t;

    ~BlockProvider() override = default;

    BlockProvider(CDBWrapper& db) : db_(db) {}

    bool getTipHash(hash_t& out) const override
    {
        return db_.Read(tip_key<BlockT>(), out);
    }

    bool getBlock(const hash_t& hash, altintegration::BlockIndex<BlockT>& out) const override
    {
        return db_.Read(block_key<BlockT>(hash), out);
    }

    std::shared_ptr<altintegration::BlockIterator<BlockT>> getBlockIterator() const override
    {
        std::shared_ptr<CDBIterator> it(db_.NewIterator());
        return std::make_shared<BlockIterator<BlockT>>(it);
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif