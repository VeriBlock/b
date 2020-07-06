#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Feature POP popdata max size test

"""

from test_framework.pop import POP_PAYOUT_DELAY, mine_vbk_blocks, endorse_block
from test_framework.test_framework import BitcoinTestFramework
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.payout import POW_PAYOUT
from test_framework.util import (
    connect_nodes,
    sync_mempools,
)

class PopPayouts(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypopminer()

    def setup_network(self):
        self.setup_nodes()

        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)


    def generate_simple_transaction(self, node_id, tx_amount):

        self.nodes[node_id].generate(nblocks=1000)
        send_amount = 0.001
        receiver_addr = self.nodes[node_id].getnewaddress()

        for i in range(tx_amount):
            if i % 10 == 0:
                self.log.warning("generate new transaction, {}".format(i))
            self.nodes[node_id].sendtoaddress(receiver_addr, send_amount)

    def _test_case(self):
        self.log.warning("running _test_case()")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))

        # Generate vtbs
        vbk_blocks = 1
        mine_vbk_blocks(self.nodes[0], self.apm, vbk_blocks)

        # Generate simple transactions
        tx_amount = 10000
        self.generate_simple_transaction(node_id = 0, tx_amount = tx_amount)

        containingblockhash = self.nodes[0].generate(nblocks=1)[0]
        containingblock = self.nodes[0].getblock(containingblockhash)

        assert len(containingblock['tx']) == tx_amount + 1
        assert len(containingblock['pop']['data']['vbkblocks']) <= vbk_blocks

        self.log.warning("success! _test_case()")

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypopminer import MockMiner
        self.apm = MockMiner()

        self._test_case()

if __name__ == '__main__':
    PopPayouts().main()
