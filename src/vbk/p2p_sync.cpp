// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "validation.h"
#include "vbk/p2p_sync.hpp"
#include <veriblock/pop.hpp>


namespace VeriBlock {
namespace p2p {

const int64_t offerIntervalMs = 5 * 1000;
const int64_t getIntervalMs = 5 * 1000;

CCriticalSection cs_popstate;

static std::map<NodeId, std::shared_ptr<PopDataNodeState>> mapPopDataNodeState;
// static uint32_t offers_recv = 0;


template <>
PopPayloadState<altintegration::ATV>& PopDataNodeState::getPayloadState()
{
    return atvs;
}

template <>
PopPayloadState<altintegration::VTB>& PopDataNodeState::getPayloadState()
{
    return vtbs;
}

template <>
PopPayloadState<altintegration::VbkBlock>& PopDataNodeState::getPayloadState()
{
    return vbks;
}

PopDataNodeState& getPopDataNodeState(const NodeId& id) EXCLUSIVE_LOCKS_REQUIRED(cs_popstate)
{
    AssertLockHeld(cs_popstate);
    std::shared_ptr<PopDataNodeState>& val = mapPopDataNodeState[id];
    if (val == nullptr) {
        mapPopDataNodeState[id] = std::make_shared<PopDataNodeState>();
        val = mapPopDataNodeState[id];
    }
    return *val;
}

void erasePopDataNodeState(const NodeId& id) EXCLUSIVE_LOCKS_REQUIRED(cs_popstate)
{
    AssertLockHeld(cs_popstate);
    mapPopDataNodeState.erase(id);
}

template <typename pop_t>
bool receiveGetPayload(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    std::vector<std::vector<uint8_t>> requested_data;
    vRecv >> requested_data;

    if (requested_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        LOCK(cs_main);

        LogPrint(BCLog::NET, "peer %d send oversized message getdata size() = %u \n", node->GetId(), requested_data.size());
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", requested_data.size()));
        return false;
    }

    LOCK(cs_main);
    LOCK(cs_popstate);
    auto& nodestate = getPopDataNodeState(node->GetId());
    auto& state = nodestate.getPayloadState<pop_t>();

    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    auto currentTime = GetTimeMillis();
    if (state.lastProcessedGet + getIntervalMs < currentTime) {
        // it's not time to process getters yet.
        // save them in setRecv and exit.
        for (const auto& hash : requested_data) {
            state.recvGets.push_back(hash);
        }
        state.lastProcessedGet = GetTimeMillis();
        return true;
    }

    // process GET
    LogPrint(BCLog::NET, "Processing {} GETs\n", state.recvGets.size());

    for (auto it = state.recvGets.begin(), end = state.recvGets.end(); it != end;) {
        const auto& hash = *it;
        const auto* data = pop_mempool.get<pop_t>(hash);
        if (data != nullptr) {
            connman->PushMessage(node, msgMaker.Make(pop_t::name(), *data));
            it = state.recvGets.erase(it);
        } else {
            ++it;
        }
    }
    state.lastProcessedGet = GetTimeMillis();

    return true;
}

template <typename pop_t>
bool receiveOfferPayload(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    LogPrint(BCLog::NET, "received offered pop data: %s, bytes size: %d\n", pop_t::name(), vRecv.size());

    std::vector<std::vector<uint8_t>> offered_data;
    vRecv >> offered_data;

    if (offered_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        LOCK(cs_main);
        LogPrint(BCLog::NET, "peer %d sent oversized message getdata size() = %u \n", node->GetId(), offered_data.size());
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", offered_data.size()));
        return false;
    }

    LOCK(cs_main);
    LOCK(cs_popstate);
    auto& nodestate = getPopDataNodeState(node->GetId());
    auto& state = nodestate.getPayloadState<pop_t>();

    for (const auto& hash : offered_data) {
        state.recvOffers.push_back(hash);
    }
    return true;
}

template <typename pop_t>
bool receivePayload(CNode* node, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    LogPrint(BCLog::NET, "received pop data: %s, bytes size: %d\n", pop_t::name(), vRecv.size());
    pop_t data;
    vRecv >> data;

    altintegration::ValidationState valstate;
    if (!VeriBlock::GetPop().check(data, valstate)) {
        LogPrint(BCLog::NET, "peer %d sent statelessly invalid pop data: %s\n", node->GetId(), valstate.toString());
        LOCK(cs_main);
        Misbehaving(node->GetId(), 20, strprintf("statelessly invalid pop data getdata, reason: %s", valstate.toString()));
        return false;
    }


    LOCK(cs_main);
    LOCK(cs_popstate);
    auto& nodeState = getPopDataNodeState(node->GetId());
    auto& state = nodeState.template getPayloadState<pop_t>();
    const auto id = data.getId().asVector();

    if (state.sentGets.count(id) == 0) {
        LogPrint(BCLog::NET, "peer %d sent pop data %s that has not been requested\n", node->GetId(), pop_t::name());
        Misbehaving(node->GetId(), 20, strprintf("peer %d sent pop data %s that has not been requested", node->GetId(), pop_t::name()));
        return false;
    }

    // we received body, so remove
    state.sentGets.erase(id);

    auto result = pop_mempool.submit(data, valstate);
    if (!result && result.status == altintegration::MemPool::FAILED_STATELESS) {
        LogPrint(BCLog::NET, "peer %d sent statelessly invalid pop data: %s\n", node->GetId(), valstate.toString());
        Misbehaving(node->GetId(), 20, strprintf("statelessly invalid pop data getdata, reason: %s", valstate.toString()));
        return false;
    }

    return true;
}

int receivePopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    auto& pop_mempool = VeriBlock::GetPop().getMemPool();

    // process Pop Data
    if (strCommand == altintegration::ATV::name()) {
        return receivePayload<altintegration::ATV>(pfrom, vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VTB::name()) {
        return receivePayload<altintegration::VTB>(pfrom, vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VbkBlock::name()) {
        return receivePayload<altintegration::VbkBlock>(pfrom, vRecv, pop_mempool);
    }
    //----------------------

    // offer Pop Data
    static std::string ofATV = offer_prefix + altintegration::ATV::name();
    if (strCommand == ofATV) {
        return receiveOfferPayload<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string ofVTB = offer_prefix + altintegration::VTB::name();
    if (strCommand == ofVTB) {
        return receiveOfferPayload<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string ofVBK = offer_prefix + altintegration::VbkBlock::name();
    if (strCommand == ofVBK) {
        return receiveOfferPayload<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }
    //-----------------

    // get Pop Data
    static std::string getATV = get_prefix + altintegration::ATV::name();
    if (strCommand == getATV) {
        return receiveGetPayload<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string getVTB = get_prefix + altintegration::VTB::name();
    if (strCommand == getVTB) {
        return receiveGetPayload<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string getVBK = get_prefix + altintegration::VbkBlock::name();
    if (strCommand == getVBK) {
        return receiveGetPayload<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }

    return -1;
}

template <typename pop_t>
void broadcastOfferPayload(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LOCK(cs_popstate);

    auto& nodeState = getPopDataNodeState(node->GetId());
    auto& state = nodeState.getPayloadState<pop_t>();
    auto& pop_mempool = VeriBlock::GetPop().getMemPool();
    std::vector<std::vector<uint8_t>> hashes;

    auto addhashes = [&](const std::unordered_map<typename pop_t::id_t, std::shared_ptr<pop_t>>& map) {
        for (const auto& el : map) {
            auto id = el.first.asVector();

            if (state.sentOffers.count(id)) {
                // do not advertise payload which we already advertised
                continue;
            }

            state.sentOffers.insert(id);
            hashes.push_back(id);

            if (hashes.size() == MAX_POP_DATA_SENDING_AMOUNT) {
                connman->PushMessage(node, msgMaker.Make(offer_prefix + pop_t::name(), hashes));
                hashes.clear();
            }
        }
    };

    addhashes(pop_mempool.getMap<pop_t>());
    addhashes(pop_mempool.getInFlightMap<pop_t>());

    if (!hashes.empty()) {
        connman->PushMessage(node, msgMaker.Make(offer_prefix + pop_t::name(), hashes));
    }
}

template <typename pop_t>
void broadcastGetPayload(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LOCK(cs_popstate);

    auto& nodeState = getPopDataNodeState(node->GetId());
    auto& state = nodeState.getPayloadState<pop_t>();
    auto& pop_mempool = VeriBlock::GetPop().getMemPool();

    auto currentTime = GetTimeMillis();
    if (state.lastProcessedOffer + offerIntervalMs < currentTime) {
        // it's not time to process offers yet.
        // save them in recvOffers and exit.
        return;
    }

    LogPrint(BCLog::NET, "Processing {} OFFERs\n", state.recvOffers.size());

    // it's time to process offers. do that all at once.
    std::vector<typename PopPayloadState<pop_t>::id_t> requestIds;
    for (auto it = state.recvOffers.begin(), end = state.recvOffers.end(); it != end; ++it) {
        if (requestIds.size() == MAX_POP_DATA_SENDING_AMOUNT) {
            // we processed range [begin... it). remove this subset.
            state.recvOffers.erase(state.recvOffers.begin(), it);
            break;
        }

        const auto& hash = *it;
        bool isKnown = pop_mempool.isKnown<pop_t>(hash);
        if (isKnown) {
            // we already know about this payload.
            continue;
        }

        requestIds.push_back(hash);
    }

    // finally send GETs for offered payloads
    if (!requestIds.empty()) {
        for (auto& hash : requestIds) {
            state.sentGets.insert(hash);
        }
        connman->PushMessage(node, msgMaker.Make(get_prefix + pop_t::name(), requestIds));
    }
    state.lastProcessedOffer = GetTimeMillis();
}

template <typename pop_t>
void broadcastPayload(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LOCK(cs_popstate);

    auto& nodeState = getPopDataNodeState(node->GetId());
    auto& state = nodeState.getPayloadState<pop_t>();
    auto& pop_mempool = VeriBlock::GetPop().getMemPool();

    auto currentTime = GetTimeMillis();
    if (state.lastProcessedGet + getIntervalMs < currentTime) {
        // it's not time to process getters yet.
        // save them in setRecv and exit.
        return;
    }

    // process GET
    LogPrint(BCLog::NET, "Processing {} GETs\n", state.recvGets.size());

    for (auto it = state.recvGets.begin(), end = state.recvGets.end(); it != end;) {
        const auto& hash = *it;
        const auto* data = pop_mempool.get<pop_t>(hash);
        if (data != nullptr) {
            connman->PushMessage(node, msgMaker.Make(pop_t::name(), *data));
            it = state.recvGets.erase(it);
        } else {
            ++it;
        }
    }
    state.lastProcessedGet = GetTimeMillis();
}


void broadcastPopData(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    broadcastOfferPayload<altintegration::ATV>(node, connman, msgMaker);
    broadcastOfferPayload<altintegration::VTB>(node, connman, msgMaker);
    broadcastOfferPayload<altintegration::VbkBlock>(node, connman, msgMaker);

    broadcastGetPayload<altintegration::ATV>(node, connman, msgMaker);
    broadcastGetPayload<altintegration::VTB>(node, connman, msgMaker);
    broadcastGetPayload<altintegration::VbkBlock>(node, connman, msgMaker);

    broadcastPayload<altintegration::ATV>(node, connman, msgMaker);
    broadcastPayload<altintegration::VTB>(node, connman, msgMaker);
    broadcastPayload<altintegration::VbkBlock>(node, connman, msgMaker);
}

} // namespace p2p

} // namespace VeriBlock
