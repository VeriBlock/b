// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SRC_VBK_UTIL_HPP
#define BITCOIN_SRC_VBK_UTIL_HPP

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>

#include <vbk/config.hpp>
#include <vbk/service_locator.hpp>

#include <algorithm>
#include <amount.h>
#include <chain.h>
#include <functional>


namespace VeriBlock {
/**
 * Create new Container with elements filtered elements of original container. All elements for which pred returns false will be removed.
 * @tparam Container any container, such as std::vector
 * @param v instance of container to be filtered
 * @param pred predicate. Returns true for elements that need to stay in container.
 */
template <typename Container>
Container filter_if(const Container& inp, std::function<bool(const typename Container::value_type&)> pred)
{
    Container v = inp;
    v.erase(std::remove_if(
                v.begin(), v.end(), [&](const typename Container::value_type& t) {
                    return !pred(t);
                }),
        v.end());
    return v;
}

inline CAmount getCoinbaseSubsidy(const CAmount& subsidy)
{
    return subsidy * (100 - VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PERCENTAGE) / 100;
}

inline CBlockHeader headerFromBytes(const std::vector<uint8_t>& v)
{
    CDataStream stream(v, SER_NETWORK, PROTOCOL_VERSION);
    CBlockHeader header;
    stream >> header;
    return header;
}

inline altintegration::AltBlock blockToAltBlock(int nHeight, const CBlockHeader& block)
{
    altintegration::AltBlock alt;
    alt.height = nHeight;
    alt.timestamp = block.nTime;
    alt.previousBlock = std::vector<uint8_t>(block.hashPrevBlock.begin(), block.hashPrevBlock.end());
    auto hash = block.GetHash();
    alt.hash = std::vector<uint8_t>(hash.begin(), hash.end());
    return alt;
}

inline altintegration::AltBlock blockToAltBlock(const CBlockIndex& index)
{
    return blockToAltBlock(index.nHeight, index.GetBlockHeader());
}

} // namespace VeriBlock
#endif
