// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vbk/config.hpp>
#include <vbk/init.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/util.hpp>

#include <vbk/test/util/e2e_fixture.hpp>

#include <gmock/gmock.h>
#include <string>

using ::testing::Return;

inline std::vector<uint8_t> operator""_v(const char* s, size_t size)
{
    return std::vector<uint8_t>{s, s + size};
}

BOOST_AUTO_TEST_SUITE(block_validation_tests)

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

BOOST_FIXTURE_TEST_CASE(BlockWithTooManyPublicationTxes, E2eFixture)
{
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    std::vector<uint256> endorsedBlockHashes;
    auto* walkBlock = ChainActive().Tip();

    size_t test_amount = 10;
    for (size_t i = 0; i < config.popconfig.alt->getMaxPopDataPerBlock() + test_amount; ++i) {
        endorsedBlockHashes.push_back(walkBlock->GetBlockHash());
        walkBlock = walkBlock->pprev;
    }

    BOOST_CHECK_EQUAL(endorsedBlockHashes.size(), config.popconfig.alt->getMaxPopDataPerBlock() + 10);

    std::vector<ATV> atvs;
    atvs.reserve(endorsedBlockHashes.size());
    std::transform(endorsedBlockHashes.begin(), endorsedBlockHashes.end(), std::back_inserter(atvs), [&](const uint256& hash) -> ATV {
        return endorseAltBlock(hash, defaultPayoutInfo);
    });

    BOOST_CHECK_EQUAL(endorsedBlockHashes.size(), atvs.size());

    auto& pop_mempool = pop->getMemPool();
    altintegration::ValidationState state;
    BOOST_CHECK(pop_mempool.submitATV(atvs, state));

    bool isValid = false;
    CBlock block1 = CreateAndProcessBlock({}, ChainActive().Tip()->GetBlockHash(), cbKey, &isValid);
    BOOST_CHECK_EQUAL(block1.v_popData.size(), config.popconfig.alt->getMaxPopDataPerBlock());

    CBlock block2 = CreateAndProcessBlock({}, ChainActive().Tip()->GetBlockHash(), cbKey, &isValid);
    BOOST_CHECK_EQUAL(block2.v_popData.size(), test_amount);

    CBlock block3 = CreateAndProcessBlock({}, ChainActive().Tip()->GetBlockHash(), cbKey, &isValid);
    BOOST_CHECK_EQUAL(block3.v_popData.size(), 0);

    block3.v_popData.insert(block3.v_popData.end(), block1.v_popData.begin(), block1.v_popData.end());
    block3.v_popData.insert(block3.v_popData.end(), block2.v_popData.begin(), block2.v_popData.end());

    BOOST_CHECK_EQUAL(block3.v_popData.size(), config.popconfig.alt->getMaxPopDataPerBlock() + test_amount);

    BlockValidationState block_state;
    BOOST_CHECK(!pop->addAllBlockPayloads(*ChainActive().Tip(), block3, block_state));
    BOOST_CHECK_EQUAL(block_state.GetRejectReason(), "pop-data-size");
}

BOOST_FIXTURE_TEST_CASE(BlockWithLargePopData, E2eFixture)
{
    auto& pop_mempool = pop->getMemPool();
    altintegration::ValidationState state;

    BOOST_CHECK(pop_mempool.submitVTB({ endorseVbkTip() }, state));

    std::vector<uint256> endorsedBlockHashes = { ChainActive().Tip()->GetBlockHash() , ChainActive().Tip()->pprev->GetBlockHash() };
    BOOST_CHECK_EQUAL(endorsedBlockHashes.size(), 2);

    std::vector<ATV> atvs;
    atvs.reserve(endorsedBlockHashes.size());
    std::transform(endorsedBlockHashes.begin(), endorsedBlockHashes.end(), std::back_inserter(atvs), [&](const uint256& hash) -> ATV {
        return endorseAltBlock(hash, defaultPayoutInfo);
    });

    BOOST_CHECK_EQUAL(endorsedBlockHashes.size(), atvs.size());
    BOOST_CHECK(pop_mempool.submitATV(atvs, state));

    std::vector<altintegration::PopData> v_pop_data = pop->getPopData(*ChainActive().Tip());
    BOOST_CHECK_EQUAL(v_pop_data.size(), 2);
    BOOST_CHECK_EQUAL(v_pop_data[0].vtbs.size(),1);

    size_t num_vtbs = 8000;
    v_pop_data[0].vtbs.reserve(num_vtbs);
    std::generate_n(std::back_inserter(v_pop_data[0].vtbs), num_vtbs, [&]() -> VTB {
        return v_pop_data[0].vtbs[0];
    });

    bool isValid = false;
    CBlock block = CreateAndProcessBlock({}, ChainActive().Tip()->GetBlockHash(), cbKey, &isValid);
    block.v_popData = v_pop_data;

    BlockValidationState block_state;
    BOOST_CHECK(!pop->addAllBlockPayloads(*ChainActive().Tip(), block, block_state));
    BOOST_CHECK_EQUAL(block_state.GetRejectReason(), "pop-data-weight");

    // remove one pop_data that does not contain vtbs
    block.v_popData.erase(block.v_popData.begin() + 1);
    block_state = BlockValidationState();
    BOOST_CHECK(pop->addAllBlockPayloads(*ChainActive().Tip(), block, block_state));

    num_vtbs = 2000;
    v_pop_data[0].vtbs.reserve(num_vtbs);
    std::generate_n(std::back_inserter(v_pop_data[0].vtbs), num_vtbs, [&]() -> VTB {
        return v_pop_data[0].vtbs[0];
    });

    block.v_popData = v_pop_data;
    block_state = BlockValidationState();
    BOOST_CHECK(!pop->addAllBlockPayloads(*ChainActive().Tip(), block, block_state));
    BOOST_CHECK_EQUAL(block_state.GetRejectReason(), "pop-data-weight");
}

static altintegration::PopData generateRandPopData()
{
    // add PopData
    auto atvBytes = altintegration::ParseHex(VeriBlockTest::defaultAtvEncoded);
    auto streamATV = altintegration::ReadStream(atvBytes);
    auto atv = altintegration::ATV::fromVbkEncoding(streamATV);

    auto vtbBytes = altintegration::ParseHex(VeriBlockTest::defaultVtbEncoded);
    auto streamVTB = altintegration::ReadStream(vtbBytes);
    auto vtb = altintegration::VTB::fromVbkEncoding(streamVTB);


    altintegration::PopData popData;
    popData.atv = atv;
    popData.vtbs = {vtb, vtb, vtb};

    return popData;
}

BOOST_AUTO_TEST_CASE(GetBlockWeight_test)
{
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

    int64_t popDataWeight = VeriBlock::GetPopDataWeight(popData);

    BOOST_CHECK(popDataWeight > 0);

    // put PopData into block
    block.v_popData = {popData, popData};

    int64_t new_block_weight = GetBlockWeight(block);
    BOOST_CHECK_EQUAL(new_block_weight, expected_block_weight);
}

BOOST_AUTO_TEST_CASE(block_serialization_test)
{
    // Create random block
    CBlock block;
    block.hashMerkleRoot.SetNull();
    block.hashPrevBlock.SetNull();
    block.nBits = 10000;
    block.nNonce = 10000;
    block.nTime = 10000;
    block.nVersion = 1;

    altintegration::PopData popData = generateRandPopData();

    block.v_popData = {popData, popData};

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
