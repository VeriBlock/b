// VeriBlock Blockchain Project
// Copyright 2017-2018 VeriBlock, Inc
// Copyright 2018-2019 Xenios SEZC
// All rights reserved.
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include "rpc_register.hpp"

#include "merkle.hpp"
#include "pop_service.hpp"
#include "vbk/service_locator.hpp"
#include <chainparams.h>
#include <consensus/merkle.h>
#include <key_io.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <index/txindex.h>
#include <util/validation.h>
#include <validation.h>
#include <wallet/rpcwallet.h>
#include <wallet/rpcwallet.h> // for GetWalletForJSONRPCRequest
#include <wallet/wallet.h>    // for CWallet

namespace VeriBlock {

namespace {

UniValue createPopTx(const CScript& scriptSig)
{
    LOCK(cs_main);

    auto tx = VeriBlock::MakePopTx(scriptSig);

    const uint256& hashTx = tx.GetHash();
    if (!::mempool.exists(hashTx)) {
        TxValidationState state;
        auto tx_ref = MakeTransactionRef<const CMutableTransaction&>(tx);
        auto result = AcceptToMemoryPool(mempool, state, tx_ref,
            nullptr /* plTxnReplaced */, false /* bypass_limits */, 0 /* nAbsurdFee */, false /* test accept */);
        if (result) {
            std::string err;
            if(g_rpc_chain && !g_rpc_chain->broadcastTransaction(tx_ref, err, 0, true)){
                throw JSONRPCError(RPC_TRANSACTION_ERROR, err);
            }
//            RelayTransaction(hashTx, *this->connman);
            return hashTx.GetHex();
        }

        if (state.IsInvalid()) {
            throw JSONRPCError(RPC_TRANSACTION_REJECTED, FormatStateMessage(state));
        }

        throw JSONRPCError(RPC_TRANSACTION_ERROR, FormatStateMessage(state));
    }

    return hashTx.GetHex();
}

uint256 GetBlockHashByHeight(const int height)
{
    if (height < 0 || height > ChainActive().Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

    return ChainActive()[height]->GetBlockHash();
}

CBlock GetBlockChecked(const CBlockIndex* pblockindex)
{
    CBlock block;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the
        // block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }

    return block;
}

} // namespace

UniValue getpopdata(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getpopdata block_height\n"
            "\nFetches the data relevant to POP-mining the given block.\n"
            "\nArguments:\n"
            "1. block_height         (numeric, required) The height index\n"
            "\nResult:\n"
            "{\n"
            "    \"block_header\" : \"block_header_hex\",  (string) Hex-encoded block header\n"
            "    \"raw_contextinfocontainer\" : \"contextinfocontainer\",  (string) Hex-encoded raw authenticated ContextInfoContainer structure\n"
            "    \"last_known_veriblock_blocks\" : [ (array) last known VeriBlock blocks at the given Bitcoin block\n"
            "        \"blockhash\",                (string) VeriBlock block hash\n"
            "       ... ]\n"
            "    \"last_known_bitcoin_blocks\" : [ (array) last known Bitcoin blocks at the given Bitcoin block\n"
            "        \"blockhash\",                (string) Bitcoin block hash\n"
            "       ... ]\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getpopdata", "1000") + HelpExampleRpc("getpopdata", "1000"));

    int height = request.params[0].get_int();

    LOCK(cs_main);

    uint256 blockhash = GetBlockHashByHeight(height);

    UniValue result(UniValue::VOBJ);

    //get the block and its header
    const CBlockIndex* pBlockIndex = LookupBlockIndex(blockhash);

    if (!pBlockIndex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    bool txindexReady = false;
    if (g_txindex) {
        txindexReady = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if(!txindexReady) {
        throw JSONRPCError(RPC_MISC_ERROR, "Blockchain transactions are still in the process of being indexed");
    }

    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << pBlockIndex->GetBlockHeader();
    result.pushKV("block_header", HexStr(ssBlock));

    auto block = GetBlockChecked(pBlockIndex);

    auto& pop = getService<PopService>();
    //context info
    uint256 txRoot = BlockMerkleRoot(block);
    auto keystones = VeriBlock::getKeystoneHashesForTheNextBlock(pBlockIndex->pprev);
    auto contextInfo = VeriBlock::ContextInfoContainer(pBlockIndex->nHeight, keystones, txRoot);
    auto authedContext = contextInfo.getAuthenticated();
    result.pushKV("raw_contextinfocontainer", HexStr(authedContext.begin(), authedContext.end()));

    auto lastVBKBlocks = pop.getLastKnownVBKBlocks(16);

    UniValue univalueLastVBKBlocks(UniValue::VARR);
    for (const auto& b : lastVBKBlocks) {
        univalueLastVBKBlocks.push_back(HexStr(b));
    }
    result.pushKV("last_known_veriblock_blocks", univalueLastVBKBlocks);

    auto lastBTCBlocks = pop.getLastKnownBTCBlocks(16);
    UniValue univalueLastBTCBlocks(UniValue::VARR);
    for (const auto& b : lastBTCBlocks) {
        univalueLastBTCBlocks.push_back(HexStr(b));
    }
    result.pushKV("last_known_bitcoin_blocks", univalueLastBTCBlocks);

    return result;
}

UniValue submitpop(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "submitpop [vtbs] (atv)\n"
            "\nCreates and submits a POP transaction constructed from the provided ATV and VTBs.\n"
            "\nArguments:\n"
            "1. atv       (string, required) Hex-encoded ATV record.\n"
            "2. vtbs      (array, required) Array of hex-encoded VTB records.\n"
            "\nResult:\n"
            "             (string) Transaction hash\n"
            "\nExamples:\n" +
            HelpExampleCli("submitpop", "ATV_HEX [VTB_HEX VTB_HEX]") + HelpExampleRpc("submitpop", "ATV_HEX [VTB_HEX, VTB_HEX]"));

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR});

//     bool txindexReady = false;
//     if (g_txindex) {
//         txindexReady = g_txindex->BlockUntilSyncedToCurrentChain();
//     }
//
//     if(!txindexReady) {
//         throw JSONRPCError(RPC_MISC_ERROR, "Blockchain transactions are still in the process of being indexed");
//     }

    auto& config = VeriBlock::getService<VeriBlock::Config>();
    UniValue popTxs(UniValue::VARR);
    CScript script;

    const UniValue& vtb_array = request.params[1].get_array();
    LogPrint(BCLog::POP, "Submitpop called with ATV and VTBs=%d\n", vtb_array.size());
    for (uint32_t idx = 0u, size = vtb_array.size(); idx < size; ++idx) {
        auto& vtbhex = vtb_array[idx];
        auto vtb = ParseHexV(vtbhex, "vtb[" + std::to_string(idx) + "]");

        CScript appendedScript = script;
        appendedScript << vtb << OP_CHECKVTB << OP_CHECKPOP;
        if (::GetSerializeSize(VeriBlock::MakePopTx(appendedScript),
                               PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > config.max_pop_tx_weight) {
            if (script.empty()) {
                std::ostringstream err;
                err << "VTB[" << idx << "] size exceeds the maximum allowed PoP transaction size";
                throw JSONRPCError(RPC_INVALID_PARAMETER, err.str());
            }
            LogPrint(BCLog::POP, "Creating a split PoP transaction; starting the next transaction at index %d", idx);
            script << OP_CHECKPOP;
            popTxs.push_back(createPopTx(script));
            script = CScript{};
        }

        script << vtb << OP_CHECKVTB;
        LogPrint(BCLog::POP, "VTB%d=\"%s\"\n", idx, vtbhex.get_str());
    }

    auto& atvhex = request.params[0];
    auto atv = ParseHexV(atvhex, "atv");

    CScript appendedScript = script;
    appendedScript << atv << OP_CHECKATV << OP_CHECKPOP;
    if (::GetSerializeSize(VeriBlock::MakePopTx(appendedScript),
                           PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > config.max_pop_tx_weight) {
        if (script.empty()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "ATV size exceeds the maximum allowed PoP transaction size");
        }
        LogPrint(BCLog::POP, "Creating a split PoP transaction; putting the ATV into a separate transaction");
        script << OP_CHECKPOP;
        popTxs.push_back(createPopTx(script));
        script = CScript{};
    }

    script << atv << OP_CHECKATV;
    script << OP_CHECKPOP;

    LogPrint(BCLog::POP, "ATV=\"%s\"\n", atvhex.get_str());
    popTxs.push_back(createPopTx(script));

    return popTxs;
}


const CRPCCommand commands[] = {
    {"pop_mining", "submitpop", &submitpop, {"atv", "vtbs"}},
    {"pop_mining", "getpopdata", &getpopdata, {"blockheight"}},
};

void RegisterPOPMiningRPCCommands(CRPCTable& t)
{
    for (const auto& command : VeriBlock::commands) {
        t.appendCommand(command.name, &command);
    }
}


} // namespace VeriBlock
