#include "vbk/p2p_sync.hpp"

#include "veriblock/entities/atv.hpp"
#include "veriblock/entities/vbkblock.hpp"
#include "veriblock/entities/vtb.hpp"

namespace VeriBlock {
namespace p2p {

template <typename PopDataType>
bool processGetPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received offered pop data: %s, bytes size: %d", PopDataType::name(), vRecv.size());
    std::vector<uint8_t> data_hash;
    vRecv >> data_hash;

    const PopDataType* data = pop_mempool.get<PopDataType>(data_hash);
    if (!data) {
        const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
        connman->PushMessage(node, msgMaker.Make(PopDataType::name(), *data));
    }
}

template <typename PopDataType>
bool processOfferPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received offered pop data: %s, bytes size: %d", PopDataType::name(), vRecv.size());
    std::vector<uint8_t> data_hash;
    vRecv >> data_hash;

    if (!pop_mempool.get<PopDataType>(data_hash)) {
        const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
        connman->PushMessage(node, msgMaker.Make(get_prefix + PopDataType::name(), data_hash));
    }

    return true;
}

template <typename PopDataType>
bool processPopData(CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received pop data: %s, bytes size: %d", PopDataType::name(), vRecv.size());
    PopDataType data;
    vRecv >> data;

    altintegration::ValidationState state;
    if (!pop_mempool.submit(data, state)) {
        LogPrint(BCLog::NET, "VeriBlock-PoP: %s ", state.GetPath());
        return false;
    }

    return true;
}

bool processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, int64_t nTimeReceived, const CChainParams& chainparams, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc)
{
    auto& pop_mempool = VeriBlock::getService<VeriBlock::PopService>().getMemPool();
    // process Pop Data
    if (strCommand == altintegration::ATV::name()) {
        LOCK(cs_main);
        return processPopData<altintegration::ATV>(vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VTB::name()) {
        LOCK(cs_main);
        return processPopData<altintegration::VTB>(vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        return processPopData<altintegration::VbkBlock>(vRecv, pop_mempool);
    }
    //----------------------

    // offer Pop Data
    if (strCommand == offer_prefix + altintegration::ATV::name()) {
        LOCK(cs_main);
        return processOfferPopData<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == offer_prefix + altintegration::VTB::name()) {
        LOCK(cs_main);
        return processOfferPopData<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == offer_prefix + altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        return processOfferPopData<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }
    //-----------------

    // get Pop Data
    if (strCommand == get_prefix + altintegration::ATV::name()) {
        LOCK(cs_main);
        return processGetPopData<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == get_prefix + altintegration::VTB::name()) {
        LOCK(cs_main);
        return processGetPopData<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == get_prefix + altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        return processGetPopData<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }

    return true;
}


} // namespace p2p

} // namespace VeriBlock