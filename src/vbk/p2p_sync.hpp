// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_P2P_SYNC_HPP
#define BITCOIN_SRC_VBK_P2P_SYNC_HPP

#include <chainparams.h>
#include <map>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <vbk/pop_service.hpp>

#include "veriblock/mempool.hpp"

namespace VeriBlock {

namespace p2p {

// The state of the Node that stores already known Pop Data
struct PopDataNodeState {
    std::set<altintegration::ATV::id_t> known_atv{};
    std::set<altintegration::VTB::id_t> known_vtb{};
    std::set<altintegration::VbkBlock::id_t> known_blocks{};

    template <typename T>
    std::set<typename T::id_t>& getSet();
};

template <>
inline std::set<altintegration::ATV::id_t>& PopDataNodeState::getSet<altintegration::ATV>()
{
    return known_atv;
}

template <>
inline std::set<altintegration::VTB::id_t>& PopDataNodeState::getSet<altintegration::VTB>()
{
    return known_vtb;
}

template <>
inline std::set<altintegration::VbkBlock::id_t>& PopDataNodeState::getSet<altintegration::VbkBlock>()
{
    return known_blocks;
}

template <typename PopDataType>
typename PopDataType::id_t getId(const PopDataType& data);

template <>
inline altintegration::ATV::id_t getId(const altintegration::ATV& data)
{
    return data.getId();
}

template <>
inline altintegration::VTB::id_t getId(const altintegration::VTB& data)
{
    return data.getId();
}

template <>
inline altintegration::VbkBlock::id_t getId(const altintegration::VbkBlock& data)
{
    return data.getShortHash();
}

} // namespace p2p

} // namespace VeriBlock


namespace VeriBlock {

namespace p2p {

/** Map maintaining peer PopData state  */
extern std::map<NodeId, PopDataNodeState> mapPopDataNodeState GUARDED_BY(cs_main);

const static std::string get_prefix = "g";
const static std::string offer_prefix = "of";

const static uint32_t MAX_POP_DATA_SENDING_AMOUNT = MAX_INV_SZ;

template <typename PopDataType>
void offerPopData(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto& pop_mempool = VeriBlock::getService<VeriBlock::PopService>().getMemPool();
    auto data = pop_mempool.getMap<PopDataType>();

    auto& known_set = mapPopDataNodeState[node->GetId()].getSet<PopDataType>();

    std::vector<std::vector<uint8_t>>
        hashes;
    for (const auto& el : data) {
        if (known_set.find(el.first) == known_set.end()) {
            hashes.push_back(el.first.asVector());
            known_set.insert(el.first);
        }

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
void sendPopData(CConnman* connman, const CNetMsgMaker& msgMaker, const std::vector<PopDataType>& data) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    LogPrint(BCLog::NET, "send PopData: count %d\n", data.size());

    connman->ForEachNode([&connman, &msgMaker, &data](CNode* pnode) {
        TRY_LOCK(cs_main, locked);

        auto& known_set = mapPopDataNodeState[pnode->GetId()].getSet<PopDataType>();
        for (const auto& el : data) {
            known_set.insert(getId(el));
            connman->PushMessage(pnode, msgMaker.Make(PopDataType::name(), el));
        }
    });
}

bool processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);

} // namespace p2p
} // namespace VeriBlock


#endif