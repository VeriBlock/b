// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vbk/pop_service_impl.hpp>

#include <chrono>
#include <memory>
#include <thread>

#include <amount.h>
#include <chain.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/sigcache.h>
#include <shutdown.h>
#include <streams.h>
#include <util/strencodings.h>
#include <validation.h>

#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>

#include <veriblock/alt-util.hpp>
#include <veriblock/altintegration.hpp>
#include <veriblock/finalizer.hpp>
#include <veriblock/stateless_validation.hpp>
#include <veriblock/validation_state.hpp>

namespace {

std::vector<uint8_t> HashFunction(const std::vector<uint8_t>& data)
{
    return VeriBlock::headerFromBytes(data).GetHash().asVector();
}

} // namespace

namespace VeriBlock {

void PopServiceImpl::addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams)
{
    PoPRewards rewards = getPopRewards(pindexPrev, consensusParams);
    assert(coinbaseTx.vout.size() == 1 && "at this place we should have only PoW payout here");
    for (const auto& itr : rewards) {
        CTxOut out;
        out.scriptPubKey = itr.first;
        out.nValue = itr.second;
        coinbaseTx.vout.push_back(out);
    }
}

bool PopServiceImpl::checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state)
{
    PoPRewards rewards = getPopRewards(pindexPrev, consensusParams);
    CAmount nTotalPopReward = 0;

    if (tx.vout.size() < rewards.size()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-vouts-size",
            strprintf("checkCoinbaseTxWithPopRewards(): coinbase has incorrect size of pop vouts (actual vouts size=%d vs expected vouts=%d)", tx.vout.size(), rewards.size()));
    }

    std::map<CScript, CAmount> cbpayouts;
    // skip first reward, as it is always PoW payout
    for (auto out = tx.vout.begin() + 1, end = tx.vout.end(); out != end; ++out) {
        // pop payouts can not be null
        if (out->IsNull()) {
            continue;
        }
        cbpayouts[out->scriptPubKey] += out->nValue;
    }

    // skip first (regular pow) payout, and last 2 0-value payouts
    for (const auto& payout : rewards) {
        auto& script = payout.first;
        auto& expectedAmount = payout.second;

        auto p = cbpayouts.find(script);
        // coinbase pays correct reward?
        if (p == cbpayouts.end()) {
            // we expected payout for that address
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-missing-payout",
                strprintf("[tx: %s] missing payout for scriptPubKey: '%s' with amount: '%d'",
                    tx.GetHash().ToString(),
                    HexStr(script),
                    expectedAmount));
        }

        // payout found
        auto& actualAmount = p->second;
        // does it have correct amount?
        if (actualAmount != expectedAmount) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-wrong-payout",
                strprintf("[tx: %s] wrong payout for scriptPubKey: '%s'. Expected %d, got %d.",
                    tx.GetHash().ToString(),
                    HexStr(script),
                    expectedAmount, actualAmount));
        }

        nTotalPopReward += expectedAmount;
    }

    if (tx.GetValueOut() > nTotalPopReward + PoWBlockReward) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
            "bad-cb-pop-amount",
            strprintf("ConnectBlock(): coinbase pays too much (actual=%d vs limit=%d)", tx.GetValueOut(), PoWBlockReward + nTotalPopReward));
    }

    return true;
}

PoPRewards PopServiceImpl::getPopRewards(const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams)
{
    auto& config = getService<Config>();
    if ((pindexPrev.nHeight + 1) < (int)config.popconfig.alt->getRewardParams().rewardSettlementInterval()) return {};
    auto state = altintegration::ValidationState();
    auto blockHash = pindexPrev.GetBlockHash();
    auto rewards = altTree->getPopPayout(blockHash.asVector(), state);
    if (state.IsError()) {
        throw std::logic_error(state.GetDebugMessage());
    }

    int halvings = (pindexPrev.nHeight + 1) / consensusParams.nSubsidyHalvingInterval;
    PoPRewards btcRewards{};
    //erase rewards, that pay 0 satoshis and halve rewards
    for (const auto& r : rewards) {
        auto rewardValue = r.second;
        rewardValue >>= halvings;
        if ((rewardValue != 0) && (halvings < 64)) {
            CScript key = CScript(r.first.begin(), r.first.end());
            btcRewards[key] = rewardValue;
        }
    }

    return btcRewards;
}

bool PopServiceImpl::acceptBlock(const CBlockIndex& indexNew, BlockValidationState& state)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto containing = VeriBlock::blockToAltBlock(indexNew);
    altintegration::ValidationState instate;
    if (!altTree->acceptBlock(containing, instate)) {
        return state.Error(instate.GetDebugMessage());
    }

    return true;
}

bool PopServiceImpl::addAllBlockPayloads(const CBlockIndex& indexPrev, const CBlock& connecting, BlockValidationState& state)
{
    std::lock_guard<std::mutex> lock(mutex);
    return addAllPayloadsToBlockImpl(*altTree, indexPrev, connecting, state);
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownVBKBlocks(size_t blocks)
{
    std::lock_guard<std::mutex> lock(mutex);
    return altintegration::getLastKnownBlocks(altTree->vbk(), blocks);
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownBTCBlocks(size_t blocks)
{
    std::lock_guard<std::mutex> lock(mutex);
    return altintegration::getLastKnownBlocks(altTree->btc(), blocks);
}

// Forkresolution
int PopServiceImpl::compareForks(const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip)
{
    if (&leftForkTip == &rightForkTip) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex);
    auto left = blockToAltBlock(leftForkTip);
    auto right = blockToAltBlock(rightForkTip);
    auto state = altintegration::ValidationState();
    return altTree->comparePopScore(left.hash, right.hash);
}

// Pop rewards
void PopServiceImpl::rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs)
{
    std::lock_guard<std::mutex> lock(mutex);
    // TODO: implement
}

PopServiceImpl::PopServiceImpl(const altintegration::Config& config)
{
    config.validate();
    altTree = altintegration::Altintegration::create(config);
    mempool = std::make_shared<altintegration::MemPool>(altTree->getParams(), altTree->vbk().getParams(), altTree->btc().getParams(), HashFunction);

    altTree->connectOnInvalidateBlock([&](const altintegration::BlockIndex<altintegration::AltBlock>& invalidated) {
        LOCK(cs_main);
        auto index = LookupBlockIndex(uint256(invalidated.getHash()));
        if (!index) {
            // we don't know this block, do nothing.
            return;
        }
        if (invalidated.status & altintegration::BLOCK_FAILED_CHILD) {
            index->nStatus |= BLOCK_FAILED_CHILD;
        }
        if (invalidated.status & altintegration::BLOCK_FAILED_POP) {
            index->nStatus |= BLOCK_FAILED_VALID;
        }
    });
}

void PopServiceImpl::invalidateBlockByHash(const uint256& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto v = block.asVector();
    altTree->removeSubtree(v);
}

bool PopServiceImpl::setState(const uint256& block, altintegration::ValidationState& state)
{
    std::lock_guard<std::mutex> lock(mutex);
    return altTree->setState(block.asVector(), state);
}

std::vector<altintegration::PopData> PopServiceImpl::getPopData(const CBlockIndex& currentBlockIndex)
{
    altintegration::AltBlock current = VeriBlock::blockToAltBlock(currentBlockIndex.nHeight, currentBlockIndex.GetBlockHeader());
    altintegration::ValidationState state;
    return mempool->getPop(current, *this->altTree);
}

void PopServiceImpl::removePayloadsFromMempool(const std::vector<altintegration::PopData>& v_popData)
{
    mempool->removePayloads(v_popData);
}

bool validatePopDataLimits(const altintegration::AltChainParams& config, const std::vector<altintegration::PopData>& v_pop_data, BlockValidationState& state)
{
    if (v_pop_data.size() > config.getMaxPopDataPerBlock()) {
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "pop-data-size", "[" + std::to_string(v_pop_data.size()) + "] size v_pop_data limits");
    }

    if (v_pop_data.size() == 1) {
        if (v_pop_data[0].toVbkEncoding().size() > config.getSuperMaxPopDataWeight()) {
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "pop-data-weight", "[" + std::to_string(v_pop_data.size()) + "] super weight pop_data limits");
        }
    }
    else {
        for (const auto& pop_data : v_pop_data) {
            if (pop_data.toVbkEncoding().size() > config.getMaxPopDataWeight()) {
                return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "pop-data-weight", "[" + std::to_string(v_pop_data.size()) + "] weight pop_data limits");
            }
        }
    }

    return true;
}

bool addAllPayloadsToBlockImpl(altintegration::AltTree& tree, const CBlockIndex& indexPrev, const CBlock& block, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (!validatePopDataLimits(tree.getParams(), block.v_popData, state)) {
        return false;
    }

    auto containing = VeriBlock::blockToAltBlock(indexPrev.nHeight + 1, block.GetBlockHeader());

    altintegration::ValidationState instate;
    std::vector<altintegration::AltPayloads> payloads(block.v_popData.size());
    // transform v_popData to the AltPayloads
    for (size_t i = 0; i < payloads.size(); ++i) {
        auto& pop_data = block.v_popData[i];

        const altintegration::PublicationData& publicationData = pop_data.atv.transaction.publicationData;
        CBlockHeader endorsedHeader;

        try {
            endorsedHeader = headerFromBytes(publicationData.header);
        } catch (const std::exception& e) {
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "pop-data-alt-block-invalid", "[" + pop_data.atv.getId().asString() + "] can't deserialize endorsed block header: " + e.what());
        }

        // set endorsed header
        AssertLockHeld(cs_main);
        CBlockIndex* endorsedIndex = LookupBlockIndex(endorsedHeader.GetHash());
        if (!endorsedIndex) {
            return state.Invalid(
                BlockValidationResult::BLOCK_INVALID_HEADER,
                "pop-data-endorsed-block-missing",
                "[ " + pop_data.atv.getId().asString() + "]: endorsed block " + endorsedHeader.GetHash().ToString() + " is missing");
        }

        altintegration::AltPayloads p;
        p.containingBlock = containing;
        p.endorsed = blockToAltBlock(*endorsedIndex);
        p.popData = pop_data;

        payloads[i] = p;
    }


    if (!tree.acceptBlock(containing, instate)) {
        return error("[%s] block %s is not accepted by altTree: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (!payloads.empty() && !tree.addPayloads(containing, payloads, instate)) {
        return error("[%s] block %s failed stateful pop validation: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    return true;
}

} // namespace VeriBlock
