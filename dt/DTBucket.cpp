#include "DTBucket.h"

MyVector<DTBucketElement*> DTBucket::EmptyBucket;

void DTBucket::DeleteElement(int bucketIdx, int elemIdx) {
    MyVector<DTBucketElement*>& bucketList = buckList[bucketIdx];
    if (bucketList.size() == 1) {
        bucketList.pop_back();
        //this->shrinkToFit();
        return;
    }
    int previousSize = bucketList.size();
    DTBucketElement* swapElement = bucketList[previousSize - 1];
    bucketList[elemIdx] = swapElement;
    swapElement->element_index = elemIdx;
    bucketList.pop_back();
    if (bucketList.size() < bucketList.capacity() * 0.5) {
        bucketList.shrink_to_fit();
    }
}

DTBucket::~DTBucket() {
    for (int i = 0; i < buckList.size(); ++i) {
        buckList[i].release_space();
    }
    buckList.release_space();

}

int DTBucket::listSize() {
    return buckList.size();
}

void DTBucket::extendListSize(int index) {
    while (buckList.size() < index) {
        buckList.push_back(EmptyBucket);
        cnt.push_back(0);
    }
}

int DTBucket::InsertNewELement(int bucketIdx, DTBucketElement* elem, int updateCount) {
    this->extendListSize(bucketIdx + 1);
    buckList[bucketIdx].push_back(elem);
    if(buckList[bucketIdx].size() == 1){
        cnt[bucketIdx] = updateCount;
    }
    elem->element_index = buckList[bucketIdx].size() - 1;
    return buckList[bucketIdx].size();
}

bool DTBucket::CheckEmptyByIndex(int bucketIdx) {
    if (buckList.size() < bucketIdx) {
        return true;
    } else if (buckList[bucketIdx].size() == 0) {
        return true;
    }
    return false;

}

int DTBucket::sizeByIndex(int bucketIdx) {
    if (buckList.size() <= bucketIdx) {
        return 0;
    }
    return buckList[bucketIdx].size();
}

void DTBucket::shrinkToFit() {
    int bucketIdx = buckList.size() - 1;
    while (buckList[bucketIdx].size() == 0) {
        buckList.pop_back();
        cnt.pop_back();
        bucketIdx--;
    }
}

void DTBucket::ReleaseSpace() {
    for (int i = 0; i < buckList.size(); ++i) {
        buckList[i].release_space();
    }
    buckList.release_space();
}

DTBucketElement* DTBucket::getElement(int bucketIdx, int elemIdx) const {
    return buckList[bucketIdx][elemIdx];
}

int DTBucket::getCnt(int bucketIdx) const {
    return cnt[bucketIdx];
}

void DTBucket::updateCnt(int bucketIdx, int updateCount) const {
    cnt[bucketIdx] = updateCount;
}


