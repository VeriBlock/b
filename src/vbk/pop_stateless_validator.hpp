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
        const std::shared_ptr<altintegration::ValidationState>& state,
        const std::shared_ptr<boost::mutex>& mutex,
        size_t checkIndex = 0,
        PopCheckType checkType = PopCheckType::POP_CHECK_CONTEXT) : popData_(popData),
                                                                    state_(state), mutex_(mutex), checkIndex_(checkIndex), checkType_(checkType) {}

    // worker executes here
    bool operator()();

    void swap(PopCheck& check)
    {
        std::swap(popData_, check.popData_);
        std::swap(state_, check.state_);
        std::swap(mutex_, check.mutex_);
        std::swap(checkIndex_, check.checkIndex_);
        std::swap(checkType_, check.checkType_);
    }

    const altintegration::ValidationState& getState() const { return *state_; }

protected:
    std::shared_ptr<altintegration::PopData> popData_ = nullptr;
    std::shared_ptr<altintegration::ValidationState> state_ = nullptr;
    std::shared_ptr<boost::mutex> mutex_ = nullptr;
    size_t checkIndex_ = 0;
    PopCheckType checkType_ = PopCheckType::POP_CHECK_CONTEXT;
};

struct PopValidator {
    PopValidator() : popcheckqueue(128), control(&popcheckqueue) { start(); }
    ~PopValidator() { stop(); };

    CCheckQueue<PopCheck> popcheckqueue;
    CCheckQueueControl<PopCheck> control;
    boost::thread_group threadGroup;

    // start thread pool
    void start();
    // stop thread pool
    void stop();

protected:
    void threadPopCheck(int worker_num);
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_POP_STATELESS_VALIDATOR_HPP
