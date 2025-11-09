#include "DTManager.h"

void DTManager::removeInstance(int indexToRemove) {
    int listSize = dtInstanceList.size();
    if (indexToRemove == listSize - 1) {
        dtInstanceList.pop_back();
    } else {
        DTInstance *lastInstance = dtInstanceList[listSize - 1];
        dtInstanceList[listSize - 1] = dtInstanceList[indexToRemove];
        dtInstanceList[indexToRemove] = lastInstance;
        dtInstanceList.pop_back();
    }
}
