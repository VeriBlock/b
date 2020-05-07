// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <consensus/validation.h>

#include <vbk/config.hpp>
#include <vbk/init.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/util.hpp>

#include <string>
#include <gmock/gmock.h>

using ::testing::Return;

inline std::vector<uint8_t> operator""_v(const char* s, size_t size)
{
    return std::vector<uint8_t>{s, s + size};
}

//struct BlockValidationFixture : public TestChain100Setup {
//    testing::NiceMock<VeriBlockTest::PopServiceImplMock> pop_impl_mock;
//
//    CScript cbKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    std::vector<uint8_t> btc_header = std::vector<uint8_t>(80, 1);
//    std::vector<uint8_t> vbk_header = std::vector<uint8_t>(64, 2);
//
//    CScript ctxscript = (CScript() << vbk_header << OP_POPVBKHEADER << btc_header << OP_POPBTCHEADER << OP_CHECKPOP);
//    CScript pubscript = (CScript() << "ATV"_v << OP_CHECKATV << "VTB"_v << OP_CHECKVTB << OP_CHECKPOP);
//
//    BlockValidationFixture()
//    {
//        auto& config = VeriBlock::InitConfig();
//
//        CBlockIndex* endorsedBlockIndex = ChainActive().Tip()->pprev;
//        CBlock endorsedBlock;
//        BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));
//
//        stream = std::make_shared<CDataStream>(SER_NETWORK, PROTOCOL_VERSION);
//        *stream << endorsedBlock.GetBlockHeader();
//
//        ON_CALL(pop_impl_mock, determineATVPlausibilityWithBTCRules).WillByDefault(Return(true));
//
//        ON_CALL(pop_service_mock, checkVTBinternally).WillByDefault(Return(true));
//        ON_CALL(pop_service_mock, checkATVinternally).WillByDefault(Return(true));
//        ON_CALL(pop_service_mock, blockPopValidation).WillByDefault(
//          [&](const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) -> bool {
//            return VeriBlock::blockPopValidationImpl(pop_impl_mock, block, pindexPrev, params, state);
//          });
//    };
//
//    std::shared_ptr<CDataStream> stream;
//};
//
BOOST_AUTO_TEST_SUITE(block_validation_tests)

//BOOST_FIXTURE_TEST_CASE(BlockWithTooManyPublicationTxes, BlockValidationFixture)
//{
//    std::vector<CMutableTransaction> pubs;
//    for (size_t i = 0; i < 256 /* more than 50 */; ++i) {
//        CScript script;
//        script << std::vector<uint8_t>{10, (uint8_t)i} << OP_CHECKATV << OP_CHECKPOP;
//        pubs.emplace_back(VeriBlock::MakePopTx(script));
//    }
//
//    bool isBlockValid = true;
//    auto block = CreateAndProcessBlock(pubs, cbKey, &isBlockValid);
//    BOOST_CHECK(!isBlockValid);
//}
//
//BOOST_FIXTURE_TEST_CASE(BlockWithMaxNumberOfPopTxes, BlockValidationFixture)
//{
//    auto& config = VeriBlock::getService<VeriBlock::Config>();
//    std::vector<CMutableTransaction> pubs;
//    for (size_t i = 0; i < std::min(config.max_pop_tx_amount, 256u); ++i) {
//        CScript script;
//        script << std::vector<uint8_t>{10, (uint8_t)i} << OP_CHECKATV << OP_CHECKPOP;
//        pubs.emplace_back(VeriBlock::MakePopTx(script));
//    }
//
//    bool isBlockValid = true;
//    auto block = CreateAndProcessBlock(pubs, cbKey, &isBlockValid);
//    BOOST_CHECK(isBlockValid);
//    BOOST_CHECK(block.vtx.size() == config.max_pop_tx_amount + 1 /* coinbase */);
//}
//
//
//BOOST_FIXTURE_TEST_CASE(BlockWithBothPopTxes, BlockValidationFixture)
//{
//    VeriBlock::Context ctx;
//    ctx.btc.push_back(btc_header);
//    ctx.vbk.push_back(vbk_header);
//
//    CMutableTransaction ctxtx = VeriBlock::MakePopTx(ctxscript);
//    CMutableTransaction pubtx = VeriBlock::MakePopTx(pubscript);
//
//    EXPECT_CALL(pop_service_mock, checkATVinternally).Times(1);
//    EXPECT_CALL(pop_service_mock, checkVTBinternally).Times(1);
//
//    auto block = CreateAndProcessBlock({ctxtx, pubtx}, cbKey);
//
//    BOOST_CHECK(block.vtx.size() == 3);
//    BOOST_CHECK(block.vtx[0]->IsCoinBase());
//    BOOST_CHECK(*block.vtx[1] == CTransaction(ctxtx));
//    BOOST_CHECK(*block.vtx[2] == CTransaction(pubtx));
//}

static altintegration::PopData generateRandPopData() {
    // add PopData
    auto atvBytes = altintegration::ParseHex(VeriBlockTest::defaultAtvEncoded);
    auto streamATV = altintegration::ReadStream(atvBytes);
    auto atv = altintegration::ATV::fromVbkEncoding(streamATV);

    auto vtbBytes = altintegration::ParseHex(VeriBlockTest::defaultVtbEncoded);
    auto streamVTB = altintegration::ReadStream(vtbBytes);
    auto vtb = altintegration::VTB::fromVbkEncoding(streamVTB);


    altintegration::PopData popData;
    popData.atv = atv;
    popData.vtbs = { vtb, vtb, vtb };

    return popData;
}

BOOST_AUTO_TEST_CASE(GetBlockWeight_test) {

    // Create random block
    CBlock block;
    block.hashMerkleRoot.SetNull();
    block.hashPrevBlock.SetNull();
    block.nBits = 10000;
    block.nNonce = 10000;
    block.nTime = 10000;
    block.nVersion = 1;

    int64_t expected_block_weight = GetBlockWeight(block);

    BOOST_CHECK(expected_block_weight > 0);

    altintegration::PopData popData = generateRandPopData();

    int64_t popDataWeight = GetPopDataWeight(popData);

    BOOST_CHECK(popDataWeight > 0);

    // put PopData into block
    block.v_popData = { popData, popData };

    int64_t new_block_weight = GetBlockWeight(block);
    BOOST_CHECK_EQUAL(new_block_weight, expected_block_weight);
}

BOOST_AUTO_TEST_CASE(block_serialization_test) {

    // Create random block
    CBlock block;
    block.hashMerkleRoot.SetNull();
    block.hashPrevBlock.SetNull();
    block.nBits = 10000;
    block.nNonce = 10000;
    block.nTime = 10000;
    block.nVersion = 1;

    altintegration::PopData popData = generateRandPopData();

    block.v_popData = { popData, popData };

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    BOOST_CHECK(stream.size() == 0);
    stream << block;
    BOOST_CHECK(stream.size() != 0);

    CBlock decoded_block;
    stream >> decoded_block;

    BOOST_CHECK(decoded_block.GetHash() == block.GetHash());
    BOOST_CHECK(decoded_block.v_popData[0] == block.v_popData[0]);
    BOOST_CHECK(decoded_block.v_popData[1] == block.v_popData[1]);
}

BOOST_AUTO_TEST_SUITE_END()
