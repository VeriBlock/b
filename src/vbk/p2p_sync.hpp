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
#include <node/context.h>
#include <rpc/blockchain.h>
#include <vbk/pop_common.hpp>
#include <veriblock/pop.hpp>

namespace VeriBlock {

namespace p2p {

extern CCriticalSection cs_popstate;


template <typename T>
struct PopPayloadState {
    using id_t = std::vector<uint8_t>;

    // peer sent us these offers of type T
    std::vector<id_t> recvOffers;
    // peer sent us these GETs of type T
    std::vector<id_t> recvGets;

    // set of sent GETs of type T to this heer
    std::unordered_set<id_t> sentGets;
    // set of sent offers of type T to this heer
    std::unordered_set<id_t> sentOffers;

    int64_t lastProcessedOffer{0};
    int64_t lastProcessedGet{0};
};

// The state of the Node that stores already known Pop Data
struct PopDataNodeState {
    PopPayloadState<altintegration::ATV> atvs;
    PopPayloadState<altintegration::VTB> vtbs;
    PopPayloadState<altintegration::VbkBlock> vbks;

    template <typename T>
    PopPayloadState<T>& getPayloadState();
};

PopDataNodeState& getPopDataNodeState(const NodeId& id);

void erasePopDataNodeState(const NodeId& id);

} // namespace p2p

} // namespace VeriBlock


namespace VeriBlock {

namespace p2p {

const static std::string get_prefix = "g";
const static std::string offer_prefix = "of";

const static uint32_t MAX_POP_DATA_SENDING_AMOUNT = 100;
const static uint32_t MAX_POP_RECV_OFFERS_AMOUNT = 1000;

void broadcastPopData(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker);

int receivePopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);

} // namespace p2p
} // namespace VeriBlock


#endif