// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_POP_STATELESS_VALIDATOR_HPP
#define BITCOIN_SRC_VBK_POP_STATELESS_VALIDATOR_HPP

#include <boost/thread.hpp>
#include <boost/thread/detail/thread_group.hpp>
#include <checkqueue.h>

namespace VeriBlock {

enum class PopCheckType {
    POP_CHECK_CONTEXT,
    POP_CHECK_VTB,
    POP_CHECK_ATV
};

class PopCheck
{
public:
    PopCheck() = default;
    PopCheck(const std::shared_ptr<altintegration::PopData>& popData,
        const altintegration::ValidationState& state,
        std::atomic_bool& stopFlag,
        size_t checkIndex = 0,
        PopCheckType checkType = PopCheckType::POP_CHECK_CONTEXT) : popData_(popData),
                                                                    state_(state), stop(&stopFlag), checkIndex_(checkIndex), checkType_(checkType) {}

    bool operator()();

    void swap(PopCheck& check)
    {
        std::swap(popData_, check.popData_);
        std::swap(state_, check.state_);
        std::swap(stop, check.stop);
        std::swap(checkIndex_, check.checkIndex_);
        std::swap(checkType_, check.checkType_);
    }

    const altintegration::ValidationState& getState() const { return state_; }

protected:
    std::shared_ptr<altintegration::PopData> popData_;
    altintegration::ValidationState state_;
    volatile std::atomic_bool* stop;
    size_t checkIndex_;
    PopCheckType checkType_;
};

class PopValidator
{
public:
    PopValidator() : popcheckqueue(128), control(&popcheckqueue)
    {
        init();
    }

    CCheckQueueControl<PopCheck>& getControl();

protected:
    CCheckQueue<PopCheck> popcheckqueue;
    CCheckQueueControl<PopCheck> control;
    boost::thread_group threadGroup;

    void init();
    void threadPopCheck(int worker_num);
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_POP_STATELESS_VALIDATOR_HPP
