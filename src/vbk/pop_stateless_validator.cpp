// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tinyformat.h>
#include <util/threadnames.h>
#include <validation.h>

#include <vbk/pop_common.hpp>

#include "pop_stateless_validator.hpp"

namespace VeriBlock {

bool PopCheck::operator()()
{
    if (*stop) {
        return false;
    }
    bool ret = false;
    switch (checkType_) {
    case PopCheckType::POP_CHECK_CONTEXT: {
        const auto& params = GetPop().config->getVbkParams();
        VBK_ASSERT_MSG(popData_->context.size() > checkIndex_, "Invalid context block index for validation");
        ret = checkBlock(popData_->context.at(checkIndex_), state_, params);
        break;
    }
    case PopCheckType::POP_CHECK_VTB: {
        const auto& params = GetPop().config->getBtcParams();
        VBK_ASSERT_MSG(popData_->vtbs.size() > checkIndex_, "Invalid VTB index for validation");
        ret = checkVTB(popData_->vtbs.at(checkIndex_), state_, params);
        break;
    }
    case PopCheckType::POP_CHECK_ATV: {
        const auto& params = GetPop().config->getAltParams();
        VBK_ASSERT_MSG(popData_->atvs.size() > checkIndex_, "Invalid ATV index for validation");
        ret = checkATV(popData_->atvs.at(checkIndex_), state_, params);
        break;
    }
    default: break;
    }

    if (ret) return true;
    *stop = true;
    return false;
}

void PopValidator::threadPopCheck(int worker_num)
{
    util::ThreadRename(strprintf("popch.%i", worker_num));
    popcheckqueue.Thread();
}

void PopValidator::init()
{
    if (threadGroup.size() > 0) {
        LogPrintf("POP validation is already initialized\n");
        return;
    }

    int script_threads = gArgs.GetArg("-par", DEFAULT_SCRIPTCHECK_THREADS);
    if (script_threads <= 0) {
        // -par=0 means autodetect (number of cores - 1 script threads)
        // -par=-n means "leave n cores free" (number of cores - n - 1 script threads)
        script_threads += GetNumCores();
    }

    // Subtract 1 because the main thread counts towards the par threads
    script_threads = std::max(script_threads - 1, 0);

    // Number of script-checking threads <= MAX_SCRIPTCHECK_THREADS
    script_threads = std::min(script_threads, MAX_SCRIPTCHECK_THREADS);

    LogPrintf("POP validation uses %d additional threads\n", script_threads);
    if (script_threads >= 1) {
        for (int i = 0; i < script_threads; ++i) {
            threadGroup.create_thread([&]() { return this->threadPopCheck(i); });
        }
    }
}

CCheckQueueControl<PopCheck>& PopValidator::getControl()
{
    return control;
}

} // namespace VeriBlock
