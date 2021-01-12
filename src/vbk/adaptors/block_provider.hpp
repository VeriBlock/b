// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_PROVIDER_HPP

#include <dbwrapper.h>
#include <vbk/adaptors/block_batch_adaptor.hpp>

#include "veriblock/storage/block_provider.hpp"

namespace VeriBlock {

struct BlockProvider : public altintegration::BlockProvider {
    ~BlockProvider() override = default;

    BlockProvider(CDBWrapper& db) : db_(db) {}

    bool getTip(altintegration::BlockIndex<altintegration::BtcBlock>& out) const override
    {
        typename altintegration::BtcBlock::hash_t hash;
        if(!db_.Read(BlockBatchAdaptor::btctip(), hash)) {
            return false;
        }
        return db_.Read(std::make_pair(DB_BTC_BLOCK, hash), out);
    }

    bool getTip(altintegration::BlockIndex<altintegration::VbkBlock>& out) const override
    {
        typename altintegration::VbkBlock::hash_t hash;
        if(!db_.Read(BlockBatchAdaptor::vbktip(), hash)) {
            return false;
        }
        return db_.Read(std::make_pair(DB_VBK_BLOCK, hash), out);
    }

    bool getTip(altintegration::BlockIndex<altintegration::AltBlock>& out) const override
    {
        typename altintegration::AltBlock::hash_t hash;
        if(!db_.Read(BlockBatchAdaptor::alttip(), hash)) {
            return false;
        }
        return db_.Read(std::make_pair(DB_ALT_BLOCK, hash), out);
    }

    bool getBlock(const typename altintegration::BtcBlock::hash_t& hash, altintegration::BlockIndex<altintegration::BtcBlock>& out) const override
    {
        return db_.Read(std::make_pair(DB_BTC_BLOCK, hash), out);
    }

    bool getBlock(const typename altintegration::VbkBlock::hash_t& hash, altintegration::BlockIndex<altintegration::VbkBlock>& out) const override
    {
        return db_.Read(std::make_pair(DB_VBK_BLOCK, hash), out);
    }

    bool getBlock(const typename altintegration::AltBlock::hash_t& hash, altintegration::BlockIndex<altintegration::AltBlock>& out) const override
    {
        return db_.Read(std::make_pair(DB_ALT_BLOCK, hash), out);
    }

private:
    CDBWrapper& db_;
};

} // namespace VeriBlock

#endif