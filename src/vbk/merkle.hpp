// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_MERKLE_HPP
#define BITCOIN_SRC_VBK_MERKLE_HPP

#include <iostream>

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <veriblock/entities/keystone_container.hpp>

namespace VeriBlock {

altintegration::KeystoneContainer GetKeystones(const CBlockIndex* prev);

uint256 CalculateContextInfoContainerHash(const CBlockIndex* prevIndex, const altintegration::PopData& popData);

uint256 TopLevelMerkleRoot(const CBlockIndex* prevIndex, const CBlock& block, bool* mutated = nullptr);

bool VerifyTopLevelMerkleRoot(const CBlock& block, const CBlockIndex* pprevIndex, BlockValidationState& state);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_MERKLE_HPP
