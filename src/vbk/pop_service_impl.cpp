// VeriBlock Blockchain Project
// Copyright 2017-2018 VeriBlock, Inc
// Copyright 2018-2019 Xenios SEZC
// All rights reserved.
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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
/*
bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}
typedef std::vector<unsigned char> valtype;

#define stacktop(i) (stack.at(stack.size() + (i)))
void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}
*/
} // namespace


namespace {

std::vector<uint8_t> HashFunction(const std::vector<uint8_t>& data)
{
    CHashWriter stream(SER_GETHASH, PROTOCOL_VERSION);
    stream.write((const char*)data.data(), data.size());
    auto hash = stream.GetHash();

    return std::vector<uint8_t>(hash.begin(), hash.end());
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

bool PopServiceImpl::checkPopPayloads(const CBlockIndex& indexPrev, const CBlock& fullBlock, BlockValidationState& state)
{
    // does not modify internal state, so no locking required
    altintegration::AltTree copy = *altTree;
    return addAllPayloadsToBlockImpl(copy, indexPrev, fullBlock, state);
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
    auto right = blockToAltBlock(leftForkTip);
    auto state = altintegration::ValidationState();
    altTree->acceptBlock(left, state);
    altTree->acceptBlock(right, state);

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
}

void PopServiceImpl::invalidateBlockByHash(const uint256& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto v = block.asVector();
    altTree->removeSubtree(v);
}

bool PopServiceImpl::setState(const uint256& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    altintegration::ValidationState state;
    return altTree->setState(block.asVector(), state);
}

std::vector<altintegration::PopData> PopServiceImpl::getPopData(const CBlockIndex& currentBlockIndex)
{
    altintegration::AltBlock current = VeriBlock::blockToAltBlock(currentBlockIndex.nHeight, currentBlockIndex.GetBlockHeader());
    altintegration::ValidationState state;
    return mempool->getPop(current, *this->altTree, state);
}

void PopServiceImpl::removePayloads(const std::vector<altintegration::PopData>& v_popData)
{
    mempool->removePayloads(v_popData);
}

bool addAllPayloadsToBlockImpl(altintegration::AltTree& tree, const CBlockIndex& indexPrev, const CBlock& block, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto containing = VeriBlock::blockToAltBlock(indexPrev.nHeight + 1, block.GetBlockHeader());

    altintegration::ValidationState instate;
    std::vector<altintegration::AltPayloads> payloads;
    // TODO transform v_popData to the AltPayloads

    if (!tree.acceptBlock(containing, instate)) {
        return error("[%s] block %s is not accepted by altTree: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (!payloads.empty() && !tree.addPayloads(containing, payloads, instate)) {
        return error("[%s] block %s failed stateful pop validation: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    return true;
}

} // namespace VeriBlock
