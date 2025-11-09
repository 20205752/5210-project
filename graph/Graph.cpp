#include <algorithm>
#include <queue>
#include "Graph.h"
#include "../MyLib/MyTimer.h"

Graph::Graph(MyVector<dynscan::Vertex *> &_vList, double _rho) {
    vList.swap(_vList);
    rho = _rho;
    int vertexCount = (int) vList.size();
    computePermutationNumber(omega * _rho);
//    myJaccard = new Jaccard((long double) 1.0 / FAILURE_PROB, omega * rho);
     myJaccard = new Jaccard((long double) 1.0 / (1 / vertexCount), omega * rho);
}

int Graph::insertEdge(int _vID1, int _vID2) {
    auto *vertex1 = (dynscan::Vertex *) vList[_vID1 - 1];
    auto *vertex2 = (dynscan::Vertex *) vList[_vID2 - 1];
    
    vertex1 = ensureVertexExists(vertex1, _vID1);
    vertex2 = ensureVertexExists(vertex2, _vID2);
    
    checkAndPromoteToLarge(vertex1, vertex2);
    
    normalizeVertexOrder(_vID1, _vID2, vertex1, vertex2);
    
    routeInsertionByType(vertex1, vertex2);

    return 0;
}

int Graph::removeEdge(int _vID1, int _vID2) {
    auto *vertex1 = (dynscan::Vertex *) vList[_vID1 - 1];
    auto *vertex2 = (dynscan::Vertex *) vList[_vID2 - 1];

    if (vertex1 == NULL || vertex2 == NULL)
        return 1;
    
    vertex1->deleteNeighbor(_vID2);
    vertex2->deleteNeighbor(_vID1);

    cleanupDTInstanceForEdge(vertex1, vertex2, _vID1, _vID2);
    
    normalizeVertexOrder(_vID1, _vID2, vertex1, vertex2);
    
    routeDeletionByType(vertex1, vertex2);
    
    return 0;
}

int Graph::insertBetweenSmall(dynscan::Vertex *v1, dynscan::Vertex *v2) {
    int sharedNeighborCount = 2;
    const int &vertexID1 = v1->id;
    const int &vertexID2 = v2->id;
    const int deg1 = v1->getDegree();
    const int deg2 = v2->getDegree();

    sharedNeighborCount = countCommonNeighborsForSmallPair(v1, v2, sharedNeighborCount);

    double jaccardSimilarity = computeJaccardSimilarity(sharedNeighborCount, deg1, deg2);
    int newIntersectionIndex = dynscan::Vertex::allocate_intersection_cnt_index();
    
    insertNeighborAndUpdateIntersection(v1, v2, vertexID1, vertexID2, 
                                         newIntersectionIndex, jaccardSimilarity, sharedNeighborCount);
    
    checkVertexDTBucket(v1);
    checkVertexDTBucket(v2);

    return 0;
}

int Graph::deleteBetweenSmall(dynscan::Vertex *v1, dynscan::Vertex *v2) {
    int deg1 = v1->getDegree();
    int *adjList1 = v1->getAdjacentList();

    for (int idx = 0; idx < deg1; idx++) {
        int neighborID = adjList1[idx];
        dynscan::Vertex *neighborVertex = (dynscan::Vertex *) vList[neighborID - 1];
        int neighborIdx = v2->getAdjacentIndex(neighborID);
        if (neighborVertex->isLarge()) {
            continue;
        }
        if (neighborIdx != -1) {
            v1->decreaseIntersectionCnt(idx);
            v2->decreaseIntersectionCnt(neighborIdx);
        }
    }
    checkVertexDTBucket(v1);
    checkVertexDTBucket(v2);

    return 0;
}

int Graph::insertBetweenSmallAndLarge(dynscan::Vertex *v1,
                                      dynscan::Vertex *v2) {
    int sharedCount = 2;
    int vertexID1 = v1->id;
    int vertexID2 = v2->id;

    int deg1 = v1->getDegree();
    int deg2 = v2->getDegree();
    sharedCount = countCommonNeighborsForMixedPair(v1, v2, sharedCount);

    double jaccardSim = computeJaccardSimilarity(sharedCount, deg1, deg2);
    int newIntersectionIdx = dynscan::Vertex::allocate_intersection_cnt_index();
    v1->insertNeighbor(vertexID2, newIntersectionIdx, jaccardSim);
    v2->insertNeighbor(vertexID1, -1, jaccardSim);
    checkVertexDTBucket(v1);
    checkVertexDTBucket(v2);

    createAndLinkDTInstance(v1, v2, deg1, deg2, sharedCount);
    return 0;
}

int Graph::deleteBetweenSmallAndLarge(dynscan::Vertex *v1,
                                      dynscan::Vertex *v2) {
    int deg1 = v1->getDegree();

    for (int idx = 0; idx < deg1; idx++) {
        int neighborID = v1->getNeighborID(idx);
        dynscan::Vertex *neighborVertex = (dynscan::Vertex *) vList[neighborID - 1];
        if (neighborVertex->isLarge())
            continue;
        int neighborIdx = v2->getAdjacentIndex(neighborID);
        if (neighborIdx != -1) {
            v1->decreaseIntersectionCnt(idx);
        }
    }
    checkVertexDTBucket(v1);
    checkVertexDTBucket(v2);
    return 0;
}

int Graph::insertBetweenLarge(dynscan::Vertex *v1,
                              dynscan::Vertex *v2) {
    int vertexID1 = v1->id;
    int vertexID2 = v2->id;
    int deg1 = v1->getDegree();
    int deg2 = v2->getDegree();
    
    double similarityScore = myJaccard->compute_similarity(*v1, *v2);
    v1->insertNeighbor(vertexID2, -1, similarityScore);
    v2->insertNeighbor(vertexID1, -1, similarityScore);
    checkVertexDTBucket(v1);
    checkVertexDTBucket(v2);

    int maxDeg = (deg1 > deg2 ? deg1 : deg2) + 1;
    int updateCount1 = v1->getCnt();
    int updateCount2 = v2->getCnt();

    int dtIdx = dtManager.get_size();
    DTInstance *dtInst = new DTInstance(
            (1-omega)*rho * rho, maxDeg, updateCount1,
            updateCount2, v1->id, v2->id, dtIdx);

    dtManager.insertInstance(dtInst);

    int expValue = dtInst->get_exp();
    v1->addDTBucketElement(expValue, dtInst->get_element1(), updateCount1);
    v2->addDTBucketElement(expValue, dtInst->get_element2(), updateCount2);
    v1->set_instance_index_map_by_neighbor_id(v2->id, dtIdx);
    v2->set_instance_index_map_by_neighbor_id(v1->id, dtIdx);
#ifdef _DEBUG_
    double end_time = getCurrentTime();
    time_GraphDynamic_insertBetweenLarge += (end_time - start_time);
#endif
    return 0;
}

int Graph::deleteBetweenLarge(dynscan::Vertex *v1,
                              dynscan::Vertex *v2) {

    checkVertexDTBucket(v1);
    checkVertexDTBucket(v2);

    return 0;
}

void Graph::checkVertexDTBucket(dynscan::Vertex *curVertex) {
    curVertex->increaseUpdateCnt();
    int currentVertexID = curVertex->id;
    int currentUpdateCnt = curVertex->getCnt();
    
    for (int bucketIdx = 0; bucketIdx < curVertex->listSize(); bucketIdx++) {
        MyVector<DTInstance *> instancesInNewRound;
        
        if (curVertex->sizeByIndex(bucketIdx) == 0) {
            continue;
        }
        
        int bucketCount = curVertex->getBucketCount(bucketIdx);
        int lambdaValue = pow_2[bucketIdx];
        int comparison = floor(currentUpdateCnt / lambdaValue) - floor(bucketCount / lambdaValue);
        
        if (comparison == 0) {
            break;
        }
        
        if (comparison >= 1) {
            curVertex->updateBucketCount(bucketIdx, currentUpdateCnt);
            collectInstancesForNewRound(curVertex, bucketIdx, currentUpdateCnt, instancesInNewRound);
        }
        
        processNewRoundInstances(curVertex, instancesInNewRound, currentVertexID, currentUpdateCnt);
    }
}


int Graph::makeLarge(dynscan::Vertex *v) {
    double startTime = getCurrentTime();

    int vertexID = v->id;
    int vertexDegree = v->getDegree();
    int vertexUpdateCnt = v->getCnt();
    
    for (int neighborIdx = 0; neighborIdx < vertexDegree; neighborIdx++) {
        int neighborID = v->getNeighborID(neighborIdx);
        dynscan::Vertex *neighborVertex = (dynscan::Vertex *) vList[neighborID - 1];

        if (neighborVertex->isLarge()){
            continue;
        }
        
        createDTInstanceForLargeVertex(v, neighborVertex, neighborIdx, 
                                       vertexID, vertexDegree, vertexUpdateCnt);
    }
    v->set_large();

    double endTime = getCurrentTime();
    printf("insert:*%.9lf*\n", endTime - startTime);

    return 0;
}

double Graph::query(double eps, int mu) {
    int coreCount = 0;
    int totalM_C = 0;
    double queryTime = 0;
    MyVector<dynscan::Vertex *> coreVertices;
    
    coreVertices = identifyCoreVertices(eps, mu, queryTime, coreCount, totalM_C);
    
    performBFSClustering(coreVertices, eps);
    
    return queryTime;
}

// Helper functions for insertEdge
dynscan::Vertex* Graph::ensureVertexExists(dynscan::Vertex *v, int vertexID) {
    if (v == NULL) {
        v = (dynscan::Vertex *) createVertex(vertexID);
    }
    return v;
}

void Graph::checkAndPromoteToLarge(dynscan::Vertex *v1, dynscan::Vertex *v2) {
    if (!v1->isLarge() && !v2->isLarge()) {
        if (v1->getDegree() >= permutationNum - 1 &&
            v2->getDegree() >= permutationNum - 1) {
            makeLarge(v1);
            makeLarge(v2);
        }
    } else if (!v1->isLarge() && v1->getDegree() >= permutationNum - 1 &&
               v2->isLarge()) {
        makeLarge(v1);
    } else if (!v2->isLarge() && v2->getDegree() >= permutationNum - 1 &&
               v1->isLarge()) {
        makeLarge(v2);
    }
}

void Graph::normalizeVertexOrder(int &vID1, int &vID2, 
                                  dynscan::Vertex *&v1, dynscan::Vertex *&v2) {
    if (v1->isLarge() && !v2->isLarge() || v1->getDegree() > v2->getDegree()) {
        std::swap(vID1, vID2);
        std::swap(v1, v2);
    }
}

void Graph::routeInsertionByType(dynscan::Vertex *v1, dynscan::Vertex *v2) {
    if (!v1->isLarge()) {
        if (!v2->isLarge()) {
            insertBetweenSmall(v1, v2);
        } else {
            insertBetweenSmallAndLarge(v1, v2);
        }
    } else {
        insertBetweenLarge(v1, v2);
    }
}

void Graph::cleanupDTInstanceForEdge(dynscan::Vertex *v1, dynscan::Vertex *v2, 
                                      int vID1, int vID2) {
    const int dtIdx = v1->get_instance_index_by_neighbor_id(vID2);
    if (dtIdx >= 0) {
        DTInstance *dtInst = dtManager.get_instance(dtIdx);
        const int bucketIdx = dtInst->get_exp();
        const int elemIdx1 = dtInst->get_element_index(vID2);
        const int elemIdx2 = dtInst->get_element_index(vID1);
        v1->DeleteElement(bucketIdx, elemIdx1);
        v2->DeleteElement(bucketIdx, elemIdx2);
        dtManager.removeInstance(dtIdx);
    }
}

void Graph::routeDeletionByType(dynscan::Vertex *v1, dynscan::Vertex *v2) {
    if (!v1->isLarge() && !v2->isLarge()) {
        deleteBetweenSmall(v1, v2);
    } else if (!v1->isLarge() && v2->isLarge()) {
        deleteBetweenSmallAndLarge(v1, v2);
    } else {
        deleteBetweenLarge(v1, v2);
    }
}

int Graph::countCommonNeighborsForSmallPair(dynscan::Vertex *v1, 
                                             dynscan::Vertex *v2, 
                                             int initialCount) {
    int commonCount = initialCount;
    const int deg1 = v1->getDegree();
    int *adjList1 = v1->getAdjacentList();
    
    for (int i = 0; i < deg1; ++i) {
        const int &neighborID = adjList1[i];
        auto *neighborVertex = (dynscan::Vertex *) vList[neighborID - 1];
        const int idx = v2->getAdjacentIndex(neighborID);
        if (idx != -1) {
            ++commonCount;
            if (neighborVertex->isLarge())
                continue;
            v1->increaseIntersectionCnt(i);
            v2->increaseIntersectionCnt(idx);
        }
    }
    return commonCount;
}

double Graph::computeJaccardSimilarity(int commonCount, int deg1, int deg2) {
    return commonCount / (double) (deg1 + deg2 + 4 - commonCount);
}

void Graph::insertNeighborAndUpdateIntersection(dynscan::Vertex *v1, 
                                                 dynscan::Vertex *v2,
                                                 int vID1, int vID2,
                                                 int intersectionIdx,
                                                 double similarity,
                                                 int commonCount) {
    v1->insertNeighbor(vID2, intersectionIdx, similarity);
    v2->insertNeighbor(vID1, intersectionIdx, similarity);
    v1->setIntersectionCnt(commonCount, intersectionIdx);
    v2->setIntersectionCnt(commonCount, intersectionIdx);
}

int Graph::countCommonNeighborsForMixedPair(dynscan::Vertex *v1, 
                                            dynscan::Vertex *v2, 
                                            int initialCount) {
    int sharedCount = initialCount;
    int deg1 = v1->getDegree();
    
    for (int i = 0; i < deg1; i++) {
        int neighborID = v1->getNeighborID(i);
        dynscan::Vertex *neighborVertex = (dynscan::Vertex *) vList[neighborID - 1];
        int idx = v2->getAdjacentIndex(neighborID);
        if (idx != -1) {
            sharedCount++;
            if (neighborVertex->isLarge())
                continue;
            v1->increaseIntersectionCnt(i);
        }
    }
    return sharedCount;
}

void Graph::createAndLinkDTInstance(dynscan::Vertex *v1, dynscan::Vertex *v2,
                                     int deg1, int deg2, int sharedCount) {
    int unionSize = deg1 + deg2 + 4 - sharedCount;
    int updateCnt1 = v1->getCnt();
    int updateCnt2 = v2->getCnt();
    int dtIdx = dtManager.get_size();
    
    DTInstance *dtInst = new DTInstance((1-omega)*rho * rho, unionSize, 
                                        updateCnt1, updateCnt2, 
                                        v1->id, v2->id, dtIdx);
    dtManager.insertInstance(dtInst);

    int expVal = dtInst->get_exp();
    v1->addDTBucketElement(expVal, dtInst->get_element1(), updateCnt1);
    v2->addDTBucketElement(expVal, dtInst->get_element2(), updateCnt2);
    v1->set_instance_index_map_by_neighbor_id(v2->id, dtIdx);
    v2->set_instance_index_map_by_neighbor_id(v1->id, dtIdx);
}

void Graph::collectInstancesForNewRound(dynscan::Vertex *curVertex, 
                                         int bucketIdx, 
                                         int updateCnt,
                                         MyVector<DTInstance *> &newRounds) {
    for (int j = 0; j < curVertex->sizeByIndex(bucketIdx); j++) {
        DTBucketElement *bucketElem = curVertex->getDTBucketElement(bucketIdx, j);
        DTInstance *dtInst = dtManager.get_instance(bucketElem->get_dt_index());
        bucketElem->update_cnt(updateCnt);
        dtInst->receive_report();
        if (dtInst->is_round_end()) {
            newRounds.push_back(dtInst);
        }
    }
}

void Graph::processNewRoundInstances(dynscan::Vertex *curVertex,
                                      MyVector<DTInstance *> &newRounds,
                                      int currentVertexID,
                                      int currentUpdateCnt) {
    for (int j = 0; j < newRounds.size(); j++) {
        DTInstance *dtInst = newRounds[j];
        DTBucketElement *neighborElem = dtInst->get_element(currentVertexID);
        DTBucketElement *bucketElem = dtInst->get_Another_Bucket_Element(neighborElem);
        int neighborID = bucketElem->get_neighbor_id();
        auto *neighborVertex = (dynscan::Vertex *) vList[neighborID - 1];
        int neighborUpdateCnt = neighborVertex->getCnt();
        
        int bucketIdx = dtInst->get_exp();
        curVertex->DeleteElement(bucketIdx, dtInst->get_element_index(neighborID));
        neighborVertex->DeleteElement(bucketIdx, dtInst->get_element_index(currentVertexID));

        dtInst->update_tau_and_slack(currentUpdateCnt, neighborUpdateCnt);
        
        if (!dtInst->is_mature()) {
            handleImmatureInstance(curVertex, neighborVertex, dtInst, 
                                    bucketElem, neighborElem, 
                                    currentUpdateCnt, neighborUpdateCnt);
        } else {
            handleMatureInstance(curVertex, neighborVertex, dtInst, 
                                  bucketElem, currentUpdateCnt, neighborUpdateCnt);
        }
    }
}

void Graph::handleImmatureInstance(dynscan::Vertex *curVertex,
                                     dynscan::Vertex *neighborVertex,
                                     DTInstance *dtInst,
                                     DTBucketElement *bucketElem,
                                     DTBucketElement *neighborElem,
                                     int currentUpdateCnt,
                                     int neighborUpdateCnt) {
    int bucketIdx = dtInst->get_exp();
    curVertex->addDTBucketElement(bucketIdx, bucketElem, currentUpdateCnt);
    neighborElem->update_cnt(neighborUpdateCnt);
    neighborVertex->addDTBucketElement(bucketIdx, neighborElem, neighborUpdateCnt);
}

void Graph::handleMatureInstance(dynscan::Vertex *curVertex,
                                  dynscan::Vertex *neighborVertex,
                                  DTInstance *dtInst,
                                  DTBucketElement *bucketElem,
                                  int currentUpdateCnt,
                                  int neighborUpdateCnt) {
    double newSimScore = myJaccard->compute_similarity(*curVertex, *neighborVertex);
    curVertex->updateNeighborSimScore(newSimScore, neighborVertex->id);
    neighborVertex->updateNeighborSimScore(newSimScore, curVertex->id);

    int unionLowerBound = 1 + std::max(curVertex->getDegree(), neighborVertex->getDegree());
    dtInst->reset_status(rho, unionLowerBound, currentUpdateCnt, neighborUpdateCnt);

    int bucketIdx = dtInst->get_exp();
    curVertex->addDTBucketElement(bucketIdx, bucketElem, currentUpdateCnt);
    DTBucketElement *elem2 = dtInst->get_Another_Bucket_Element(bucketElem);
    elem2->update_cnt(neighborUpdateCnt);
    neighborVertex->addDTBucketElement(bucketIdx, elem2, neighborUpdateCnt);
}

void Graph::createDTInstanceForLargeVertex(dynscan::Vertex *v,
                                             dynscan::Vertex *neighborVertex,
                                             int neighborIdx,
                                             int vertexID,
                                             int vertexDegree,
                                             int vertexUpdateCnt) {
    int neighborDegree = neighborVertex->getDegree();
    int neighborUpdateCnt = neighborVertex->getCnt();
    int unionSize = vertexDegree + 3 + neighborDegree - v->getIntersectionCnt(neighborIdx);

    int dtIdx = dtManager.get_size();
    DTInstance *newInst = new DTInstance((1 - omega) * rho * rho,
                                         unionSize, vertexUpdateCnt,
                                         neighborUpdateCnt, v->id, neighborVertex->id, dtIdx);
    dtManager.insertInstance(newInst);
    
    int expVal = newInst->get_exp();
    v->addDTBucketElement(expVal, newInst->get_element1(), vertexUpdateCnt);
    neighborVertex->addDTBucketElement(expVal, newInst->get_element2(), neighborUpdateCnt);
    v->set_instance_index_map_by_neighbor_id(neighborVertex->id, dtIdx);
    neighborVertex->set_instance_index_map_by_neighbor_id(vertexID, dtIdx);
}

MyVector<dynscan::Vertex *> Graph::identifyCoreVertices(double eps, int mu,
                                                         double &queryTime,
                                                         int &coreCount,
                                                         int &totalM_C) {
    MyVector<dynscan::Vertex *> coreVertices;
    double qStart, qEnd;
    
    for (int i = 0, vertexNum = vList.size(); i < vertexNum; i++) {
        dynscan::Vertex *v = (dynscan::Vertex *) vList[i];
        if(v->getDegree() <= mu){
            continue;
        }
        qStart = getCurrentTime();
        int tempM_C = v->query(eps, mu);
        qEnd = getCurrentTime();
        queryTime += qEnd - qStart;
        if(tempM_C == 0){
            continue;
        }
        else{
            coreCount += 1;
            totalM_C += tempM_C;
            coreVertices.push_back(v);
        }
    }
    return coreVertices;
}

void Graph::performBFSClustering(MyVector<dynscan::Vertex *> &coreVertices, double eps) {
    int vertexNum = (int) vList.size();
    int* visited = new int[vertexNum];
    for (int i = 0; i < vertexNum; ++i) {
        visited[i] = 0;
    }
    queue<dynscan::Vertex *> bfsQueue;
    
    for (int i = 0, coreNum = coreVertices.size(); i < coreNum; i++) {
        dynscan::Vertex *v = coreVertices[i];
        if(visited[v->id] == 1){
            continue;
        }
        bfsQueue.push(v);
        visited[v->id] = 1;
        MyVector<int> cluster;
        
        while(!bfsQueue.empty()){
            dynscan::Vertex *u = bfsQueue.front();
            bfsQueue.pop();
            for (auto rit = u->NOPtr->rbegin(); rit != u->NOPtr->rend(); ++rit) {
                if(rit->first >= eps){
                    int w = rit->second;
                    visited[w] = 1;
                    cluster.push_back(w);
                }
                else{
                    break;
                }
            }
        }
    }
    delete[] visited;
}


void Graph::printFinalClusterInfo(double eps, int mu) {
    printf("\n=== FINAL CLUSTERING RESULTS ===\n");
    printf("Parameters: eps=%.2f, mu=%d\n", eps, mu);

    // 第一阶段：找到所有核心顶点
    std::vector<dynscan::Vertex*> cores;
    for (int i = 0, vertex_number = vList.size(); i < vertex_number; i++) {
        dynscan::Vertex* v = static_cast<dynscan::Vertex*>(vList[i]);
        if (v == nullptr) continue;

        // 跳过度数不足的顶点
        if (v->getDegree() < mu) {
            continue;
        }

        // 使用query方法来检查是否是核心顶点
        int temp_m_C = v->query(eps, mu);
        if (temp_m_C > 0) {
            cores.push_back(v);
        }
    }

    printf("Total core vertices found: %zu\n", cores.size());

    // 第二阶段：执行聚类并输出结果
    int vertex_number = static_cast<int>(vList.size());
    std::vector<int> visited(vertex_number, 0);
    std::queue<dynscan::Vertex*> Q;
    std::vector<std::vector<int>> all_clusters;

    for (auto* coreVertex : cores) {
        int vertex_index = coreVertex->id; // 根据实际情况调整

        if (vertex_index >= vertex_number || visited[vertex_index] == 1) {
            continue;
        }

        Q.push(coreVertex);
        visited[vertex_index] = 1;
        std::vector<int> cluster;

        while (!Q.empty()) {
            dynscan::Vertex* u = Q.front();
            Q.pop();
            cluster.push_back(u->id);

            // 遍历相似邻居
            for (auto rit = u->NOPtr->rbegin(); rit != u->NOPtr->rend(); ++rit) {
                if (rit->first >= eps) {
                    int neighborID = rit->second;
                    int neighbor_index = neighborID; // 根据实际情况调整

                    if (neighbor_index < vertex_number && visited[neighbor_index] == 0) {
                        visited[neighbor_index] = 1;
                        auto* neighbor = static_cast<dynscan::Vertex*>(vList[neighbor_index]);
                        if (neighbor != nullptr) {
                            Q.push(neighbor);
                        }
                    }
                }
                else {
                    break;
                }
            }
        }

        all_clusters.push_back(cluster);
    }

    // 输出聚类统计信息到控制台
    printf("Total clusters: %zu\n", all_clusters.size());

    // 按簇大小排序（从大到小）
    std::sort(all_clusters.begin(), all_clusters.end(),
        [](const std::vector<int>& a, const std::vector<int>& b) {
            return a.size() > b.size();
        });

    // 将完整信息保存到txt文件 - 使用C风格生成文件名
    char filename[100];
    sprintf(filename, "cluster_results_eps_%.1f_mu_%d.txt", eps, mu);

    FILE* file = fopen(filename, "w");
    if (file) {
        fprintf(file, "=== COMPLETE CLUSTERING RESULTS ===\n");
        fprintf(file, "Parameters: eps=%.2f, mu=%d\n", eps, mu);
        fprintf(file, "Total core vertices found: %zu\n", cores.size());
        fprintf(file, "Total clusters: %zu\n\n", all_clusters.size());

        // 在文件中保存所有簇的完整信息
        for (size_t i = 0; i < all_clusters.size(); i++) {
            fprintf(file, "Cluster %zu: size = %zu\n", i + 1, all_clusters[i].size());
            fprintf(file, "Vertices: ");
            for (int vertex_id : all_clusters[i]) {
                fprintf(file, "%d ", vertex_id);
            }
            fprintf(file, "\n\n");
        }

        // 在文件中保存统计信息
        fprintf(file, "=== STATISTICS ===\n");
        if (!all_clusters.empty()) {
            size_t total_vertices_in_clusters = 0;
            size_t max_cluster_size = 0;
            size_t min_cluster_size = all_clusters[0].size();
            size_t singleton_count = 0;

            for (const auto& cluster : all_clusters) {
                total_vertices_in_clusters += cluster.size();
                if (cluster.size() > max_cluster_size) max_cluster_size = cluster.size();
                if (cluster.size() < min_cluster_size) min_cluster_size = cluster.size();
                if (cluster.size() == 1) singleton_count++;
            }

            double avg_cluster_size = static_cast<double>(total_vertices_in_clusters) / all_clusters.size();

            fprintf(file, "Total vertices in clusters: %zu\n", total_vertices_in_clusters);
            fprintf(file, "Cluster size - Max: %zu, Min: %zu, Avg: %.2f\n",
                max_cluster_size, min_cluster_size, avg_cluster_size);
            fprintf(file, "Number of singleton clusters: %zu\n", singleton_count);
        }
        fclose(file);
        printf("Complete cluster details saved to: %s\n", filename);
    }
    else {
        printf("Warning: Could not save complete results to file.\n");
    }

    // 在控制台只输出概括信息
    //printf("\n--- Summary (Complete details saved to file) ---\n");
    //for (size_t i = 0; i < all_clusters.size(); i++) {
    //    printf("Cluster %zu: size = %zu", i + 1, all_clusters[i].size());

    //    // 对于小簇，在控制台也显示所有顶点；对于大簇，只显示大小
    //    if (all_clusters[i].size() <= 10) {
    //        printf(" - Vertices: ");
    //        for (int vertex_id : all_clusters[i]) {
    //            printf("%d ", vertex_id);
    //        }
    //    }
    //    printf("\n");
    //}

    // 输出统计摘要到控制台
    printf("\n--- Statistics ---\n");
    if (!all_clusters.empty()) {
        size_t total_vertices_in_clusters = 0;
        size_t max_cluster_size = 0;
        size_t min_cluster_size = all_clusters[0].size();
        size_t singleton_count = 0;

        for (const auto& cluster : all_clusters) {
            total_vertices_in_clusters += cluster.size();
            if (cluster.size() > max_cluster_size) max_cluster_size = cluster.size();
            if (cluster.size() < min_cluster_size) min_cluster_size = cluster.size();
            if (cluster.size() == 1) singleton_count++;
        }

        double avg_cluster_size = static_cast<double>(total_vertices_in_clusters) / all_clusters.size();

        printf("Total vertices in clusters: %zu\n", total_vertices_in_clusters);
        printf("Cluster size - Max: %zu, Min: %zu, Avg: %.2f\n",
            max_cluster_size, min_cluster_size, avg_cluster_size);
        printf("Number of singleton clusters: %zu\n", singleton_count);

    }
    else {
        printf("No clusters found.\n");
    }

    printf("====================================\n");
}