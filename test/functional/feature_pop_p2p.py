#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

from test_framework.pop import POP_PAYOUT_DELAY, endorse_block
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    sync_mempools,
    p2p_port,
)

from test_framework.mininode import (
    P2PInterface,
    msg_offer_atv,
    msg_get_atv,
    msg_atv,
)

class BaseNode(P2PInterface):
    def __init__(self):
        super().__init__()

    def on_block(self, message):
        pass

    def on_inv(self, message):
        pass

class PopP2PSync(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypopminer()

    def setup_network(self):
        self.setup_nodes()

        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)

    def _case1_offer_pop_data_ddos_node_0(self):
        self.nodes[0].add_p2p_connection(BaseNode())
        self.log.warning("running _case1_offer_pop_data_ddos_node_0()")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        atv_id = endorse_block(self.nodes[0], self.apm, 5, addr)

        assert len(self.nodes[0].getpeerinfo()) == 1
        peerinfo = self.nodes[0].getpeerinfo()[0]
        assert(not 'noban' in peerinfo['permissions'])

        # spaming node1
        self.log.info("spaming node0 ...")
        disconnected = False
        for i in range(1000):
            try:
                msg = msg_offer_atv([atv_id])
                # Send message is used to send a P2P message to the node over our P2PInterface
                self.nodes[0].p2p.send_message(msg)
            except IOError:
                disconnected = True

        self.log.info("node0 ban our peer")
        assert disconnected

        assert len(self.nodes[0].getpeerinfo()) == 0

    def _case2_get_pop_data_ddos_node_0(self):
        self.nodes[0].add_p2p_connection(BaseNode())
        self.log.warning("running _case2_get_pop_data_ddos_node_0()")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        atv_id = endorse_block(self.nodes[0], self.apm, 5, addr)

        assert len(self.nodes[0].getpeerinfo()) == 1
        peerinfo = self.nodes[0].getpeerinfo()[0]
        assert(not 'noban' in peerinfo['permissions'])

        # spaming node1
        self.log.info("spaming node0 ...")
        disconnected = False
        for i in range(1000):
            try:
                msg = msg_get_atv([atv_id])
                # Send message is used to send a P2P message to the node over our P2PInterface
                self.nodes[0].p2p.send_message(msg)
            except IOError:
                disconnected = True

        self.log.info("node0 ban our peer")
        assert disconnected

        assert len(self.nodes[0].getpeerinfo()) == 0


    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=100)
        self.sync_all(self.nodes)

        from pypopminer import MockMiner
        self.apm = MockMiner()

        self._case1_offer_pop_data_ddos_node_0()
        self._case2_get_pop_data_ddos_node_0()


if __name__ == '__main__':
    PopP2PSync().main()
