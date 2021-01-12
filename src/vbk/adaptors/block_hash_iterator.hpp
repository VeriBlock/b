// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BLOCK_HASH_ITERATOR_HPP
#define INTEGRATION_REFERENCE_BTC_BLOCK_HASH_ITERATOR_HPP

#include <dbwrapper.h>

#include "veriblock/storage/block_hash_iterator.hpp"

namespace VeriBlock {

template <typename BlockT, char DbBlockPrefix>
struct BlockHashIterator : public altintegration::BlockHashIterator<BlockT> {
    using hash_t = typename BlockT::hash_t;

    ~BlockHashIterator() override = default;

    BlockHashIterator(CDBIterator& iter) : iter_(iter) {}

    bool next() override
    {
        iter_.Next();
        return true;
    }

    hash_t value() const override
    {
        std::pair<char, hash_t> key;
        iter_.GetKey(key);
        return key.second;
    }

    bool valid() const override
    {
        std::pair<char, hash_t> key;
        LogPrintf("BlockHashIterator valid() valid: %d, valid_key: %d, valid prefix: %d, current prefix: %c, prefix: %c, result: %d \n",
        iter_.Valid(), iter_.GetKey(key), key.first == DbBlockPrefix, key.first, DbBlockPrefix, iter_.Valid() && iter_.GetKey(key) && key.first == DbBlockPrefix);

        return iter_.Valid() && iter_.GetKey(key) && key.first == DbBlockPrefix;
    }

    bool seek_start() override
    {
        iter_.Seek(std::make_pair(DbBlockPrefix, hash_t()));
        return true;
    }

private:
    CDBIterator& iter_;
};

} // namespace VeriBlock


#endif