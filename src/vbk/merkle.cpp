// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "merkle.hpp"
#include <consensus/merkle.h>
#include <hash.h>
#include <vbk/pop_common.hpp>

namespace VeriBlock {

altintegration::KeystoneContainer GetKeystones(const CBlockIndex* prev)
{
    auto& pop = VeriBlock::GetPop();
    auto* index = prev == nullptr ? nullptr : pop.altTree->getBlockIndex(prev->GetBlockHash().asVector());
    return altintegration::KeystoneContainer::fromPrevious(index, pop.config->alt->getKeystoneInterval());
}

uint256 CalculateContextInfoContainerHash(const CBlockIndex* prevIndex, const altintegration::PopData& popData) {
    auto& pop = VeriBlock::GetPop();
    auto& altParams = *pop.config->alt;
    auto* prev = prevIndex == nullptr ? nullptr : pop.altTree->getBlockIndex(prevIndex->GetBlockHash().asVector());
    auto hash = altintegration::CalculateContextInfoContainerHash(popData, prev, altParams);
    return uint256(hash.asVector());
}

uint256 TopLevelMerkleRoot(const CBlockIndex* prevIndex, const CBlock& block, bool* mutated)
{
    int currentHeight = prevIndex == nullptr ? 0 : prevIndex->nHeight + 1;

    auto left = BlockMerkleRoot(block, mutated);
    if (!Params().isPopActive(currentHeight)) {
        return left;
    }

    auto right = CalculateContextInfoContainerHash(prevIndex, block.popData);

    // top level merkle root is a Hash of original MerkleRoot and ContextInfoContainerHash
    return Hash(left.begin(), left.end(), right.begin(), right.end());
}

bool VerifyTopLevelMerkleRoot(const CBlock& block, const CBlockIndex* pprevIndex, BlockValidationState& state)
{
    bool mutated = false;
    uint256 hashMerkleRoot2 = VeriBlock::TopLevelMerkleRoot(pprevIndex, block, &mutated);

    if (block.hashMerkleRoot != hashMerkleRoot2) {
        return state.Invalid(BlockValidationResult ::BLOCK_MUTATED, "bad-txnmrklroot",
            strprintf("hashMerkleRoot mismatch. expected %s, got %s", hashMerkleRoot2.GetHex(), block.hashMerkleRoot.GetHex()));
    }

    // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
    // of transactions in a block without affecting the merkle root of a block,
    // while still invalidating it.
    if (mutated) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txns-duplicate", "duplicate transaction");
    }

    return true;
}

} // namespace VeriBlock
