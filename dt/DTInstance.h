#ifndef DYNSCAN_DTINSTANCE_H
#define DYNSCAN_DTINSTANCE_H

#include "DTBucket.h"
#include <cstdio>
#include <math.h>


/*
 *  The class of DT instance for an edge.
 */
class DTInstance {
protected:
    // one DT instance will be related to two different heaps, for example DT(v1, v2) will be both in heap related to v1 and heap related to v2
    DTBucketElement element1;
    DTBucketElement element2;
    // The DT tau. We set tau to negative if the instance receives one report.
    int tauValue;
    int initialTau;

    int slackValue;
    int exponentValue;
    /**
     *  This is the sum of the counters from the two vertices,
     *  when the current round starts.
     */
    int initialCounterSum;

    int roundEndCounter;

    // The number of message received in the current round.
    // Since the value of msgCnt can only be either 0 or 1, actually, we can
    // save this space and use the sign of the entry index to record it.
    int messageCount;

private:
    inline void set_CntSum(const int &initialCntSum) {
        initialCounterSum = initialCntSum;
    }

public:
    DTBucketElement *get_element1() { return &element1; }

    DTBucketElement *get_element2() { return &element2; }

//    inline int get_my_bucket_index(const int &_neighborID) const {
//        return (_neighborID == element1.neighborID ? element1 : element2)
//                .bucket_index;
//    }

    inline int get_element_index(const int &neighborID) const {
        return (neighborID == element1.neighborID ? element1 : element2)
                .element_index;
    }

    inline DTBucketElement* get_element(const int &neighborID) {
        return neighborID == element1.neighborID ? &element1 : &element2;
    }

    inline DTBucketElement *
    get_Another_Bucket_Element(DTBucketElement* bucketElement) {
        return bucketElement == &element1 ? &element2 : &element1;
    }

    inline const int &get_exp() const { return exponentValue; };

    inline const int &get_slack() const { return slackValue; };

    inline const int &get_tau() const { return tauValue; };

    inline void receive_report() { ++messageCount; }

//    inline bool is_mature() const { return tau == 0; }

    inline bool is_mature() const { return tauValue <= initialTau / 10; }

    inline bool is_round_end() const { return messageCount == roundEndCounter; }

    // pow_2_slack
    inline void update_tau_and_slack(const int &updateCnt1, const int &updateCnt2) {
        tauValue = tauValue - (updateCnt1 + updateCnt2 - initialCounterSum);
        initialCounterSum = updateCnt1 + updateCnt2;
        if (tauValue >= 16) {
            roundEndCounter = 2;
            messageCount = 0;
            exponentValue = int(floor(log(tauValue / 4.0) / log(2)));
            slackValue = pow_2[exponentValue];
        } else if (tauValue > 0) {
            roundEndCounter = 1;
            messageCount = 0;
            exponentValue = 0;
            slackValue = 1;
        } else {
            slackValue = 0;
            roundEndCounter = 0;
            exponentValue = 0;
            messageCount = 0;
        }
    }

    inline DTInstance(const double &rhoParam,
                      const int &unionSizeLowerBound,
                      const int &updateCnt1, const int &updateCnt2,
                      const int &vertexID1, const int &vertexID2,
                      const int &dtIndex) {
        reset_status(rhoParam, unionSizeLowerBound, updateCnt1, updateCnt2);

        element1.cnt = updateCnt1;
        element1.neighborID = vertexID2;
        element1.dtIndex = dtIndex;
        element2.cnt = updateCnt2;
        element2.neighborID = vertexID1;
        element2.dtIndex = dtIndex;
    }

    inline void reset_status(const double &rhoParam,
                             const int &unionSizeLowerBound,
                             const int &updateCnt1, const int &updateCnt2) {
        tauValue = floor(rhoParam * unionSizeLowerBound / 2) + 1;
        initialTau = tauValue;
        set_CntSum(updateCnt1 + updateCnt2);
        update_tau_and_slack(updateCnt1, updateCnt2);
    }
};

#endif // DYNSCAN_DTINSTANCE_H
