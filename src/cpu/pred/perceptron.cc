#include "cpu/pred/perceptron.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Fetch.hh"

namespace gem5
{

namespace branch_prediction
{

Perceptron::Perceptron(const PerceptronParams &params)
    : BPredUnit(params),
      perceptronHistoryLength(params.perceptronHistoryLength),
      trainingThreshold(params.trainingThreshold),
      weightNumBits(params.weightNumBits),
      weightCounters(perceptronHistoryLength + 1, SatCounter8(weightNumBits)),
      globalHistoryReg(params.numThreads, 0)
{
    if (weightNumBits > 8) {
        fatal("Number of weight bits too large!\n");
    }

    DPRINTF(Fetch, "Create Perceptron Predictor");

    DPRINTF(Fetch, "perceptron history length: %i\n",
            perceptronHistoryLength);

    DPRINTF(Fetch, "weight num bits: %i\n", weightNumBits);

    DPRINTF(Fetch, "training threshold: %i\n",
            trainingThreshold);
}

void
Perceptron::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    // Set most recent history reg entry to 0 upon this function being called.
    globalHistoryReg[tid] &= (~1ULL);
    // TODO: restore weights?
}


void
Perceptron::update(ThreadID tid, Addr branch_addr, bool taken,
                void *bp_history, bool squashed, const StaticInstPtr & inst,
                Addr corrTarget)
{
    assert(bp_history);
    PerceptronHistory *history = static_cast<PerceptronHistory*>(bp_history);

    // We do not update the counters speculatively on a squash. (...unless?)
    // We just restore the global history register.
    if (squashed) {
        globalHistoryReg[tid] = (history->globalHistoryReg << 1) | taken;
        return;
    }


    if (taken != history->pred ||
        std::abs(history->sum) <= trainingThreshold) {
        // train perceptron // TODO: break out into new function
        // in hardware, this would be done in parallel.
        for (int i = 0; i < perceptronHistoryLength; i++) {
            // if the branch history matches the prediction, increment counter.
            if (((globalHistoryReg[tid] >> i) & 1) == taken) {
                ++weightCounters[i];
            } else {
                --weightCounters[i];
            }
        }

        // the bias weight always has a "branch history" of taken.
        if (taken) {
            ++weightCounters[perceptronHistoryLength];
        } else {
            --weightCounters[perceptronHistoryLength];
        }
    }
}

bool
Perceptron::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    bool pred;
    int sum = 0;

    // loop through history, summing weights based on global history.
    for (int i = 0; i < perceptronHistoryLength; i++) {
        if ((globalHistoryReg[tid] >> i) & 1) {
            sum += weightCounters[i];
        } else {
            sum -= weightCounters[i];
        }
    }
    // add constant weight
    sum += weightCounters[perceptronHistoryLength];

    // predict taken if sum >= 0
    pred = (sum >= 0);

    PerceptronHistory* history = new PerceptronHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->pred = pred;
    history->sum = sum;
    bp_history = static_cast<void*>(history);

    // update global history reg for speculation purposes?
    updateGlobalHistReg(tid, pred);

    return pred;
}

void
Perceptron::squash(ThreadID tid, void *bp_history)
{
    PerceptronHistory *history = static_cast<PerceptronHistory*>(bp_history);
    globalHistoryReg[tid] = history->globalHistoryReg;
    // TODO: revert weight changes?

    delete history;
}

void
Perceptron::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
    // add a taken to the branch history.
    PerceptronHistory *history = new PerceptronHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->pred = true;
    // dummy value, may be good to select maximum positive value, however?
    history->sum = 0;
    bp_history = static_cast<void*>(history);
    updateGlobalHistReg(tid, true);
}

void
Perceptron::updateGlobalHistReg(ThreadID tid, bool taken) {
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 :
                               (globalHistoryReg[tid] << 1);
}

} // namespace branch_prediction
} // namespace gem5
