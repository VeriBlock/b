#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Start 2 nodes.
Mine 100 blocks on node0.
Disconnect node[1].
node[0] endorses block 100 (fork A tip).
node[0] mines pop tx in block 101 (fork A tip)

Pop is disabled before block 200 therefore can't handle Pop data
"""

from test_framework.pop import endorse_block, create_endorsed_chain
from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    disconnect_nodes, assert_equal, assert_raises_rpc_error,
)


class PopActivate(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypopminer()

    def setup_network(self):
        self.setup_nodes()

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def _cannot_endorse(self):
        self.log.warning("starting _cannot_endorse()")

        # node0 start with 100 blocks
        self.nodes[0].generate(nblocks=100)
        self.log.info("node0 mined 100 blocks")
        assert self.get_best_block(self.nodes[0])['height'] == 100
        self.log.info("node0 generated 100 blocks")

        # endorse block 100 (fork A tip)
        addr0 = self.nodes[0].getnewaddress()
        txid = endorse_block(self.nodes[0], self.apm, 100, addr0)
        self.log.info("node0 endorsed block 100 (fork A tip)")
        # mine pop tx on node0
        self.log.info('Should not accept POP data before activation block height')
        assert_raises_rpc_error(-1, 'block contains PopData before PopSecurity has been enabled', lambda: self.nodes[0].generate(nblocks=1))

        self.log.warning("_cannot_endorse() succeeded!")

    def run_test(self):
        """Main test logic"""
        from pypopminer import MockMiner
        self.apm = MockMiner()
        self._cannot_endorse()

if __name__ == '__main__':
    PopActivate().main()
