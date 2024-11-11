#ifndef __CPU_PRED_2BIT_LOCAL_PRED_HH__
#define __CPU_PRED_2BIT_LOCAL_PRED_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/Perceptron.hh"

namespace gem5
{

namespace branch_prediction
{

/**
 * Implements a global predictor that assigns a weight to
 * each branch in history
 * Decides the outcome of a prediction by summing the weights,
 * with negative results indicating not taken.
 * "Trains" the perceptron by incrementing/decrementing weights
 * for matching the correct prediction on a misprediction or when
 * the returned value for this branch is less than a certain threshold.
 */

class Perceptron : public BPredUnit
{
  public:
    /**
     * Default branch predictor constructor.
     */
    Perceptron(const PerceptronParams &params);

    virtual void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);

    /**
     * Looks up the given address in the branch predictor and returns
     * a true/false value as to whether it is taken.
     * @param branch_addr The address of the branch to look up.
     * @param bp_history Pointer to any bp history state.
     * @return Whether or not the branch is taken.
     */
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);

    /**
     * Updates the branch predictor to Not Taken if a BTB entry is
     * invalid or not found.
     * @param branch_addr The address of the branch to look up.
     * @param bp_history Pointer to any bp history state.
     * @return Whether or not the branch is taken.
     */
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);

    /**
     * Updates the BP with taken/not taken information.
     * @param inst_PC The branch's PC that will be updated.
     * @param taken Whether the branch was taken or not taken.
     * @param bp_history Pointer to the branch predictor state that is
     * associated with the branch lookup that is being updated.
     * @param squashed Set to true when this function is called during a
     * squash operation.
     * @param inst Static instruction information
     * @param corrTarget The resolved target of the branch (only needed
     * for squashed branches)
     */
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);


    /**
     * @param bp_history Pointer to the history object.  The predictor
     * will need to update any state and delete the object.
     * I think this means replace speculative history
     * (stored in current predictor?) with that of bp_history?
     */
    void squash(ThreadID tid, void *bp_history);


  private:
    // /**
    //  *  Returns the taken/not taken prediction given the value of the
    //  *  counter.
    //  *  @param count The value of the counter.
    //  *  @return The prediction based on the counter value.
    //  */
    // inline bool getPrediction(uint8_t &count);

    // /** Calculates the local index based on the PC. */
    // inline unsigned getLocalIndex(Addr &PC);

    /** Length of the history buffer */
    const unsigned perceptronHistoryLength;

    /** Threshold for training */
    // should be floor(1.93 * history length + 14)
    const unsigned trainingThreshold;

    /** Number of bits per counter */
    const unsigned weightNumBits;

    /** Array of counters that make up the weight vector. */
    // should be log2(theta). Here, we just use 8.
    std::vector<SatCounter8> weightCounters;

    /** Storing branch history as unsigned int, may need to change length */
    std::vector<unsigned> globalHistoryReg;


    /**
     * Shifts a branch outcome in to the GHR. This is likely to be speculative.
     */
    void updateGlobalHistReg(ThreadID tid, bool taken);


  struct PerceptronHistory
  {
    unsigned globalHistoryReg;

    // prediction of the perceptron
    // true: predict taken
    // false: predict not-taken
    bool pred;

    // raw sum result of predictor
    int sum;
  };
};


} // namespace branch_prediction
} // namespace gem5

#endif // __CPU_PRED_2BIT_LOCAL_PRED_HH__
