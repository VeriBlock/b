// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>
#include <util/strencodings.h>
#include <vbk/merkle.hpp>
#include "genesis.hpp"

namespace VeriBlock {

CScript ScriptWithPrefix(uint32_t nBits)
{
    CScript script;
    if (nBits <= 0xff)
        script << nBits << CScriptNum(1);
    else if (nBits <= 0xffff)
        script << nBits << CScriptNum(2);
    else if (nBits <= 0xffffff)
        script << nBits << CScriptNum(3);
    else
        script << nBits << CScriptNum(4);

    return script;
}

CBlock CreateGenesisBlock(
    std::string pszTimestamp,
    const CScript& genesisOutputScript,
    uint32_t nTime,
    uint32_t nNonce,
    uint32_t nBits,
    int32_t nVersion,
    const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = VeriBlock::ScriptWithPrefix(nBits) << std::vector<uint8_t>{pszTimestamp.begin(), pszTimestamp.end()};
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = VeriBlock::TopLevelMerkleRoot(nullptr, genesis);
    return genesis;
}

CBlock CreateGenesisBlock(
    uint32_t nTime,
    uint32_t nNonce,
    uint32_t nBits,
    int32_t nVersion,
    const CAmount& genesisReward,
    const std::string& initialPubkeyHex,
    const std::string& pszTimestamp)
{
    const CScript genesisOutputScript = CScript() << ParseHex(initialPubkeyHex) << OP_CHECKSIG;
    return VeriBlock::CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}


CBlock MineGenesisBlock(
    uint32_t nTime,
    const std::string& pszTimestamp,
    const std::string& initialPubkeyHex,
    uint32_t nBits,
    uint32_t nVersion,
    uint32_t nNonce, // starting nonce
    uint64_t genesisReward)
{
    CBlock genesis = CreateGenesisBlock(nTime, nNonce, nBits, nVersion, genesisReward, initialPubkeyHex, pszTimestamp);

    printf("started genesis block mining...\n");
    while (!CheckProofOfWork(genesis.GetHash(), genesis.nBits, Params().GetConsensus())) {
        ++genesis.nNonce;
        if (genesis.nNonce > 4294967294LL) {
            ++genesis.nTime;
            genesis.nNonce = 0;
            printf("nonce reset... nTime=%d\n", genesis.nTime);
        }
    }

    assert(CheckProofOfWork(genesis.GetHash(), genesis.nBits, Params().GetConsensus()));

    return genesis;
}

} // namespace VeriBlock
