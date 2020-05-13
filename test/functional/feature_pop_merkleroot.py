#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""An example functional test

The module-level docstring should include a high-level description of
what the test is doing. It's the first thing people see when they open
the file and should give the reader information about *what* the test
is testing and *how* it's being tested
"""
# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from collections import defaultdict

# Avoid wildcard * imports
from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.messages import CInv, hash256
from test_framework.mininode import (
    P2PInterface,
    mininode_lock,
    msg_block,
    msg_getdata,
)
from test_framework.pop import ContextInfoContainer
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    wait_until,
)


class ExampleTest(BitcoinTestFramework):
    # Each functional test is a subclass of the BitcoinTestFramework class.

    # Override the set_test_params(), skip_test_if_missing_module(), add_options(), setup_chain(), setup_network()
    # and setup_nodes() methods to customize the test setup as required.

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.add_nodes(self.num_nodes)

    def run_test(self):
        """Main test logic"""
        self.start_node(0)

        # Generating a block on one of the nodes will get us out of IBD
        blockhashhex = self.nodes[0].generate(nblocks=1)[0]
        block = self.nodes[0].getblock(blockhashhex)
        prevhashhex = block['previousblockhash']
        merkleroothex = block['merkleroot']
        coinbasetx = block['tx'][0]

        ctx = ContextInfoContainer.create(self.nodes[0], prevhashhex)
        ctx.txRoot = coinbasetx
        h = ctx.getTopLevelMerkleRoot().hex()
        print("expected: " + merkleroothex)
        print("got     : " + h)

        assert(h == merkleroothex)


if __name__ == '__main__':
    ExampleTest().main()
