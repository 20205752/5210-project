# 合理的代码变化方案 - 用于产生可分析的差异

本文档总结了可以合理引入代码中的变化，这些变化会导致性能或聚类结果产生轻微但可分析的差异，便于与原始代码进行对比分析。

## 变化目标

1. **性能差异**：使执行时间略长（约2-5%）
2. **结果差异**：使聚类结果略有不同（边界情况处理不同）

---

## 一、性能相关的变化（使时间略长）

### 方案A：改变循环遍历顺序（影响缓存局部性）

**位置**：`Graph.cpp` - `countCommonNeighborsForSmallPair()`

**变化**：
```cpp
// 原代码（正向遍历）
for (int i = 0; i < deg1; ++i) {
    const int &neighborID = adjList1[i];
    // ...
}

// 改为（反向遍历）
for (int i = deg1 - 1; i >= 0; --i) {
    const int &neighborID = adjList1[i];
    // ...
}
```

**影响**：
- 反向遍历可能降低CPU缓存命中率
- 预期性能下降：1-3%
- 不影响算法正确性

**适用场景**：需要产生轻微性能差异时

---

### 方案B：添加不必要的中间变量（增加内存访问）

**位置**：`Graph.cpp` - `computeJaccardSimilarity()`

**变化**：
```cpp
// 原代码（直接计算）
double Graph::computeJaccardSimilarity(int commonCount, int deg1, int deg2) {
    return commonCount / (double) (deg1 + deg2 + 4 - commonCount);
}

// 改为（添加中间变量）
double Graph::computeJaccardSimilarity(int commonCount, int deg1, int deg2) {
    double numerator = (double)commonCount;      // 添加中间变量
    double deg1_d = (double)deg1;                // 增加内存访问
    double deg2_d = (double)deg2;
    double constant = 4.0;
    double denominator = deg1_d + deg2_d + constant - numerator;
    return numerator / denominator;
}
```

**影响**：
- 增加内存访问次数和临时变量
- 预期性能下降：0.5-1%
- 不影响计算结果（理论上）

**适用场景**：需要产生轻微性能差异时

---

### 方案C：在关键循环中添加边界检查

**位置**：`Graph.cpp` - `collectInstancesForNewRound()`

**变化**：
```cpp
// 原代码
for (int j = 0; j < curVertex->sizeByIndex(bucketIdx); j++) {
    DTBucketElement *bucketElem = curVertex->getDTBucketElement(bucketIdx, j);
    // ...
}

// 改为（添加冗余检查）
for (int j = 0; j < curVertex->sizeByIndex(bucketIdx); j++) {
    // 添加边界检查（虽然冗余，但增加开销）
    if (j < 0 || j >= curVertex->sizeByIndex(bucketIdx)) {
        continue;  // 冗余检查，增加条件判断开销
    }
    DTBucketElement *bucketElem = curVertex->getDTBucketElement(bucketIdx, j);
    // ...
}
```

**影响**：
- 增加条件判断开销
- 预期性能下降：0.5-1%
- 提升代码安全性（虽然检查是冗余的）

**适用场景**：需要产生轻微性能差异时

---

### 方案D：改变处理顺序（影响缓存和分支预测）

**位置**：`Graph.cpp` - `processNewRoundInstances()`

**变化**：
```cpp
// 原代码（正向处理）
for (int j = 0; j < newRounds.size(); j++) {
    DTInstance *dtInst = newRounds[j];
    // ...
}

// 改为（反向处理）
for (int j = newRounds.size() - 1; j >= 0; --j) {
    DTInstance *dtInst = newRounds[j];
    // ...
}
```

**影响**：
- 可能影响CPU分支预测和缓存行为
- 预期性能下降：1-2%
- 不影响算法正确性

**适用场景**：需要产生轻微性能差异时

---

## 二、聚类结果相关的变化（使结果略有不同）

### 方案E：改变顶点处理顺序（影响聚类边界）

**位置**：`Graph.cpp` - `identifyCoreVertices()`

**变化**：
```cpp
// 原代码（正向遍历）
for (int i = 0, vertexNum = vList.size(); i < vertexNum; i++) {
    dynscan::Vertex *v = (dynscan::Vertex *) vList[i];
    // ...
}

// 改为（反向遍历）
int vertexNum = vList.size();
for (int i = vertexNum - 1; i >= 0; --i) {
    dynscan::Vertex *v = (dynscan::Vertex *) vList[i];
    // ...
}
```

**影响**：
- 改变顶点处理顺序，可能影响边界顶点的聚类归属
- 预期结果差异：边界情况下的聚类可能略有不同
- 不影响算法正确性，但可能改变聚类边界

**适用场景**：需要产生可分析的结果差异时

---

### 方案F：在相似度计算中添加小的epsilon（改变边界情况）

**位置**：`Graph.cpp` - `handleMatureInstance()`

**变化**：
```cpp
// 原代码
double newSimScore = myJaccard->compute_similarity(*curVertex, *neighborVertex);
curVertex->updateNeighborSimScore(newSimScore, neighborVertex->id);

// 改为（添加小的数值扰动）
double newSimScore = myJaccard->compute_similarity(*curVertex, *neighborVertex);
// 添加小的数值扰动（模拟浮点误差累积）
const double EPSILON = 1e-10;
newSimScore = newSimScore + EPSILON - EPSILON;  // 强制重新规范化，可能改变边界值
curVertex->updateNeighborSimScore(newSimScore, neighborVertex->id);
```

**影响**：
- 可能改变边界相似度的比较结果
- 预期结果差异：边界情况下的聚类可能略有不同
- 模拟浮点数精度累积误差

**适用场景**：需要产生可分析的结果差异时

---

### 方案G：改变BFS遍历顺序（影响聚类扩展）

**位置**：`Graph.cpp` - `performBFSClustering()`

**变化**：
```cpp
// 原代码（反向迭代器，从高相似度到低相似度）
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

// 改为（正向迭代器，从低相似度到高相似度）
// 需要先收集所有满足条件的邻居，然后处理
MyVector<int> validNeighbors;
for (auto it = u->NOPtr->begin(); it != u->NOPtr->end(); ++it) {
    if(it->first >= eps){
        validNeighbors.push_back(it->second);
    }
}
// 然后处理收集到的邻居
for (int k = 0; k < validNeighbors.size(); k++) {
    int w = validNeighbors[k];
    if(visited[w] == 0) {
        visited[w] = 1;
        cluster.push_back(w);
    }
}
```

**影响**：
- 改变聚类扩展的顺序，可能影响最终聚类结果
- 预期结果差异：边界情况下的聚类可能略有不同
- 不影响算法正确性，但改变处理顺序

**适用场景**：需要产生可分析的结果差异时

---

### 方案H：改变核心顶点处理顺序

**位置**：`Graph.cpp` - `performBFSClustering()`

**变化**：
```cpp
// 原代码（正向处理核心顶点）
for (int i = 0, coreNum = coreVertices.size(); i < coreNum; i++) {
    dynscan::Vertex *v = coreVertices[i];
    // ...
}

// 改为（反向处理核心顶点）
int coreNum = coreVertices.size();
for (int i = coreNum - 1; i >= 0; --i) {
    dynscan::Vertex *v = coreVertices[i];
    // ...
}
```

**影响**：
- 改变核心顶点的处理顺序，可能影响聚类结果
- 预期结果差异：边界情况下的聚类可能略有不同
- 不影响算法正确性

**适用场景**：需要产生可分析的结果差异时

---

## 三、综合方案推荐

### 推荐组合1：轻微性能差异（2-3%时间增加）

**组合内容**：
- 方案A：反向遍历（1-2%性能下降）
- 方案B：添加中间变量（0.5-1%性能下降）

**预期效果**：
- 执行时间增加约2-3%
- 聚类结果完全相同
- 适合分析性能差异

---

### 推荐组合2：结果差异（聚类结果略有不同）

**组合内容**：
- 方案E：反向处理顶点（影响处理顺序）
- 方案F：添加epsilon（影响边界情况）
- 方案G：改变BFS顺序（影响聚类扩展）

**预期效果**：
- 执行时间基本不变
- 聚类结果在边界情况下略有不同
- 适合分析算法鲁棒性

---

### 推荐组合3：综合差异（性能+结果）

**组合内容**：
- 方案A + 方案B（性能影响）
- 方案E + 方案F（结果影响）

**预期效果**：
- 执行时间增加约2-3%
- 聚类结果在边界情况下略有不同
- 适合全面对比分析

---

## 实施注意事项

1. **保持算法正确性**：所有变化都不应改变算法的基本正确性
2. **变化要合理**：变化应该有合理的解释（如缓存行为、浮点精度等）
3. **差异要可分析**：差异应该足够明显可以分析，但不要太大
4. **文档化**：在代码中添加注释说明变化的原因

---

## 预期差异范围

### 性能差异
- **轻微变化**：1-3% 时间增加
- **中等变化**：3-5% 时间增加
- **明显变化**：5-10% 时间增加（不推荐）

### 结果差异
- **轻微差异**：边界情况下1-2%的顶点聚类归属不同
- **中等差异**：边界情况下3-5%的顶点聚类归属不同
- **明显差异**：>5%的差异（不推荐，可能影响算法正确性）

---

## 使用建议

1. **课程作业场景**：建议使用**推荐组合3**（综合差异），既能分析性能，又能分析结果
2. **性能分析场景**：使用**推荐组合1**（仅性能差异）
3. **算法鲁棒性分析**：使用**推荐组合2**（仅结果差异）

---

## 代码注释模板

在实施变化时，建议添加如下注释：

```cpp
// Modified: [变化描述]
// Reason: [变化原因，如：影响缓存局部性、改变处理顺序等]
// Expected impact: [预期影响，如：性能下降1-2%、可能影响边界聚类等]
// This change is intentional for comparison analysis
```

---

## 总结

这些变化方案都是合理的、可解释的，不会破坏算法的正确性，但会产生可分析的差异。选择合适的组合可以根据你的分析需求来决定。

**建议优先使用推荐组合3**，因为它能同时产生性能和结果差异，更适合全面的对比分析。

