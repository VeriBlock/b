#ifndef BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP
#define BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP

#include <vbk/entity/pop.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/pop_service/pop_service_exception.hpp>

#include <memory>
#include <vector>

#include <chainparams.h>

#include <grpc++/grpc++.h>

#include "integration.grpc.pb.h"

namespace VeriBlock {

class PopServiceImpl : public PopService
{
private:
    std::shared_ptr<VeriBlock::GrpcPopService::Stub> grpcPopService;

public:
    PopServiceImpl();

    ~PopServiceImpl() override = default;

    void savePopTxToDatabase(const CBlock& block, const int& nHeight) override;

    std::vector<BlockBytes> getLastKnownVBKBlocks(size_t blocks) override;
    std::vector<BlockBytes> getLastKnownBTCBlocks(size_t blocks) override;

    bool checkVTBinternally(const std::vector<uint8_t>& bytes) override;
    bool checkATVinternally(const std::vector<uint8_t>& bytes) override;

    int compareTwoBranches(const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip) override;

    void rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) override;

    bool blockPopValidation(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) override;

    void updateContext(const std::vector<std::vector<uint8_t>>& veriBlockBlocks, const std::vector<std::vector<uint8_t>>& bitcoinBlocks) override;

     bool parsePopTx(const CTransactionRef& tx, Publications* publications, Context* ctx, PopTxType* type) override;

    bool determineATVPlausibilityWithBTCRules(AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params) override;

    void addPayloads(const CBlock& block, const int& nHeight, const Publications& publications) override;

    void removePayloads(const CBlock&, const int&) override;

    void setConfig() override;

public:
    virtual void getPublicationsData(const Publications& tx, PublicationData& publicationData);
};

bool blockPopValidationImpl(PopServiceImpl& pop, const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state);

} // namespace VeriBlock
#endif //BITCOIN_SRC_VBK_POP_SERVICE_POP_SERVICE_IMPL_HPP
