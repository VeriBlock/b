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

    def _test_case_vbk(self, payloads_amount):
        self.log.warning("running _test_case_vbk()")

        vbk_blocks = mine_vbk_blocks(self.nodes[0], self.apm, payloads_amount)

        # mine a block on node[1] with this pop tx
        containingblockhash = self.nodes[0].generate(nblocks=1)[0]
        containingblock = self.nodes[0].getblock(containingblockhash)

        print(len(containingblock['pop']['data']['vbkblocks']))
        assert len(containingblock['pop']['data']['vbkblocks']) == payloads_amount == len(vbk_blocks)

        self.log.warning("success! _test_case_vbk()")

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypopminer import MockMiner
        self.apm = MockMiner()

        self._test_case_vbk(10)

if __name__ == '__main__':
    PopPayouts().main()