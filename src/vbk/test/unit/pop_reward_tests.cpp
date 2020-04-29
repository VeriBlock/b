// VeriBlock Blockchain Project
// Copyright 2017-2018 VeriBlock, Inc
// Copyright 2018-2019 Xenios SEZC
// All rights reserved.
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <boost/test/unit_test.hpp>
#include <vbk/test/util/e2e_fixture.hpp>
#include <vbk/test/util/tx.hpp>
#include <vbk/pop_service_impl.hpp>

struct PopRewardsTestFixture : public E2eFixture {
};

BOOST_AUTO_TEST_SUITE(pop_reward_tests)

BOOST_FIXTURE_TEST_CASE(addPopPayoutsIntoCoinbaseTx_test, PopRewardsTestFixture)
{
    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);
    CBlock block = endorseAltBlockAndMine(tip->GetAncestor(0)->GetBlockHash(), 0);
    {
        BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == block.GetHash());
    }

     // Generate a 400-block chain:
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    int rewardInterval = (int)VeriBlock::getService<VeriBlock::Config>().popconfig.alt->getRewardParams().rewardSettlementInterval();
    // we already have 101 blocks
    // do not add block with rewards
    // do not add block before block with rewards
    for (int i = 0; i < (rewardInterval - 103); i++) {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        m_coinbase_txns.push_back(b.vtx[0]);
    }

    CBlock beforePayoutBlock = CreateAndProcessBlock({}, scriptPubKey);
    BOOST_CHECK(ChainActive().Tip() != nullptr);
    BOOST_CHECK(ChainActive().Tip()->nHeight + 1 == rewardInterval);

    int n = 0;
    for (const auto& out : beforePayoutBlock.vtx[0]->vout) {
        if (out.nValue > 0) n++;
    }
    BOOST_CHECK(n == 1);

    CBlock payoutBlock = CreateAndProcessBlock({}, scriptPubKey);
    BOOST_CHECK(ChainActive().Tip() != nullptr);
    BOOST_CHECK(ChainActive().Tip()->nHeight == rewardInterval);

    n = 0;
    for (const auto& out : payoutBlock.vtx[0]->vout) {
        if (out.nValue > 0) n++;
    }

    // we've got additional coinbase out
    BOOST_CHECK(n > 1);
}

//BOOST_FIXTURE_TEST_CASE(addPopPayoutsIntoCoinbaseTx_test, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    VeriBlock::PoPRewards rewards = getRewards();
//    ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//              });
//
//    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
//
//    int n = 0;
//    for (const auto& out : block.vtx[0]->vout) {
//        if (rewards.find(out.scriptPubKey) != rewards.end()) {
//            n++;
//        }
//    }
//    // have found 4 pop rewards
//    BOOST_CHECK(n == 4);
//}
//
//BOOST_FIXTURE_TEST_CASE(checkCoinbaseTxWithPopRewards, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    VeriBlock::PoPRewards rewards = getRewards();
//    ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//              });
//    EXPECT_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).Times(testing::AtLeast(1));
//
//    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
//
//    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash()); // means that pop rewards are valid
//    testing::Mock::VerifyAndClearExpectations(&util_service_mock);
//
//    ON_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).WillByDefault(Return(false));
//    EXPECT_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).Times(testing::AtLeast(1));
//
//    BOOST_CHECK_THROW(CreateAndProcessBlock({}, scriptPubKey), std::runtime_error);
//}
//
//BOOST_FIXTURE_TEST_CASE(pop_reward_halving_test, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    while (ChainActive().Height() < Params().GetConsensus().nSubsidyHalvingInterval || ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    VeriBlock::PoPRewards rewards;
//    CScript pop_payout = CScript() << std::vector<uint8_t>(5, 1);
//    rewards[pop_payout] = 50;
//
//   ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//              });
//
//    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
//
//    for (const auto& out : block.vtx[0]->vout) {
//        if (out.scriptPubKey == pop_payout) {
//            // 3 times halving (50 / 8)
//            BOOST_CHECK(6 == out.nValue);
//        }
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE(check_wallet_balance_with_pop_reward, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//    VeriBlock::PoPRewards rewards;
//    rewards[scriptPubKey] = 56;
//
//    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//            });
//
//    auto block = CreateAndProcessBlock({}, scriptPubKey);
//
//    CAmount PoWReward = GetBlockSubsidy(ChainActive().Height(), Params().GetConsensus());
//
//    CBlockIndex* tip = ::ChainActive().Tip();
//    NodeContext node;
//    node.chain = interfaces::MakeChain(node);
//    auto& chain = *node.chain;
//    {
//        CWallet wallet(&chain, WalletLocation(), WalletDatabase::CreateDummy());
//
//        {
//            LOCK(wallet.GetLegacyScriptPubKeyMan()->cs_wallet);
//            // add Pubkey to wallet
//            BOOST_REQUIRE(wallet.GetLegacyScriptPubKeyMan()->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey()));
//        }
//
//        WalletRescanReserver reserver(&wallet);
//        reserver.reserve();
//        CWallet::ScanResult result = wallet.ScanForWalletTransactions(tip->GetBlockHash() /* start_block */, {} /* stop_block */, reserver, false /* update */);
//
//        {
//            LOCK(wallet.cs_wallet);
//            wallet.SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
//        }
//
//        BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
//        BOOST_CHECK(result.last_failed_block.IsNull());
//        BOOST_CHECK_EQUAL(result.last_scanned_block, tip->GetBlockHash());
//        BOOST_CHECK_EQUAL(*result.last_scanned_height, tip->nHeight);
//        // 3 times halving (56 / 8)
//        BOOST_CHECK_EQUAL(wallet.GetBalance().m_mine_immature, PoWReward + 7);
//    }
//}

BOOST_AUTO_TEST_SUITE_END()
