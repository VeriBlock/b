// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_P2P_SYNC_HPP
#define BITCOIN_SRC_VBK_P2P_SYNC_HPP

#include <chainparams.h>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <vbk/pop_service.hpp>

#include "veriblock/mempool.hpp"

namespace VeriBlock {

namespace p2p {

const static std::string get_prefix = "g";
const static std::string offer_prefix = "of";

const static uint32_t MAX_POP_DATA_SENDING_AMOUNT = MAX_INV_SZ;

template <typename PopDataType>
void offerPopData(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker)
{
    AssertLockHeld(cs_main);
    auto& pop_mempool = VeriBlock::getService<VeriBlock::PopService>().getMemPool();
    auto data = pop_mempool.getMap<PopDataType>();
    LogPrint(BCLog::NET, "offer pop data: %s, known data count: %d\n", PopDataType::name(), data.size());

    std::vector<std::vector<uint8_t>> hashes;
    for (const auto& el : data) {
        hashes.push_back(el.first.asVector());
        if (hashes.size() == MAX_POP_DATA_SENDING_AMOUNT) {
            connman->PushMessage(node, msgMaker.Make(offer_prefix + PopDataType::name(), hashes));
            data.clear();
        }
    }

    if (!hashes.empty()) {
        connman->PushMessage(node, msgMaker.Make(offer_prefix + PopDataType::name(), hashes));
    }
}

template <typename PopDataType>
void sendPopData(CConnman* connman, const CNetMsgMaker& msgMaker, const std::vector<PopDataType>& data)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "send PopData: count %d\n", data.size());
    connman->ForEachNode([&connman, &msgMaker, &data](CNode* pnode) {
        for (const auto& el : data) {
            connman->PushMessage(pnode, msgMaker.Make(PopDataType::name(), el));
        }
    });
}

bool processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);

} // namespace p2p
} // namespace VeriBlock


#endif