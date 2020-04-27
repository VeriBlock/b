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

BOOST_AUTO_TEST_SUITE(pop_tx_tests)

BOOST_FIXTURE_TEST_CASE(No_mempool_for_bad_payloads_pop_tx_test, E2eFixture)
{
    unsigned int initialPoolSize = mempool.size();
    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);
    auto atv = endorseAltBlock(tip->GetBlockHash(), tip->pprev->GetBlockHash(), {});
    // erase signature to make ATV check fail
    atv.transaction.signature = std::vector<unsigned char>(atv.transaction.signature.size(), 0);
    CScript sig;
    sig << atv.toVbkEncoding() << OP_CHECKATV;
    sig << OP_CHECKPOP;
    auto popTx = VeriBlock::MakePopTx(sig);

    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));

    TxValidationState state;
    auto tx_ref = MakeTransactionRef<const CMutableTransaction&>(popTx);
    {
        LOCK(cs_main);
        auto result = AcceptToMemoryPool(mempool, state, tx_ref,
            nullptr /* plTxnReplaced */, false /* bypass_limits */, 0 /* nAbsurdFee */, false /* test accept */);
        BOOST_CHECK(!result);
    }
    BOOST_CHECK_EQUAL(mempool.size(), initialPoolSize);
}

BOOST_AUTO_TEST_SUITE_END()