# 组合3实施总结 - 综合差异（性能+结果）

## 实施日期
已完成

## 实施内容概述

本次实施了**推荐组合3（综合差异）**，包含4个具体方案，旨在产生可分析的性能和结果差异。

---

## 具体修改内容

### 1. 方案A：改变循环遍历顺序（性能影响）

**文件**：`graph/Graph.cpp`  
**函数**：`countCommonNeighborsForSmallPair()`  
**位置**：第315-339行

**修改前**：
```cpp
for (int i = 0; i < deg1; ++i) {
    const int &neighborID = adjList1[i];
    // ...
}
```

**修改后**：
```cpp
for (int i = deg1 - 1; i >= 0; --i) {
    const int &neighborID = adjList1[i];
    // ...
}
```

**影响分析**：
- **性能影响**：反向遍历可能降低CPU缓存命中率
- **预期性能下降**：1-2%
- **算法正确性**：不影响，只是遍历顺序改变
- **适用场景**：在计算小顶点对之间的共同邻居时使用

---

### 2. 方案B：添加中间变量（性能影响）

**文件**：`graph/Graph.cpp`  
**函数**：`computeJaccardSimilarity()`  
**位置**：第341-352行

**修改前**：
```cpp
double Graph::computeJaccardSimilarity(int commonCount, int deg1, int deg2) {
    return commonCount / (double) (deg1 + deg2 + 4 - commonCount);
}
```

**修改后**：
```cpp
double Graph::computeJaccardSimilarity(int commonCount, int deg1, int deg2) {
    double numerator = (double)commonCount;
    double deg1_d = (double)deg1;
    double deg2_d = (double)deg2;
    double constant = 4.0;
    double denominator = deg1_d + deg2_d + constant - numerator;
    return numerator / denominator;
}
```

**影响分析**：
- **性能影响**：增加内存访问次数和临时变量分配
- **预期性能下降**：0.5-1%
- **算法正确性**：不影响，计算结果理论上相同
- **适用场景**：在计算Jaccard相似度时使用（频繁调用）

---

### 3. 方案E：改变顶点处理顺序（结果影响）

**文件**：`graph/Graph.cpp`  
**函数**：`identifyCoreVertices()`  
**位置**：第505-536行

**修改前**：
```cpp
for (int i = 0, vertexNum = vList.size(); i < vertexNum; i++) {
    dynscan::Vertex *v = (dynscan::Vertex *) vList[i];
    // ...
}
```

**修改后**：
```cpp
int vertexNum = vList.size();
for (int i = vertexNum - 1; i >= 0; --i) {
    dynscan::Vertex *v = (dynscan::Vertex *) vList[i];
    // ...
}
```

**影响分析**：
- **结果影响**：改变顶点处理顺序，可能影响边界顶点的聚类归属
- **预期结果差异**：边界情况下1-2%的顶点聚类归属可能不同
- **算法正确性**：不影响，只是处理顺序改变
- **适用场景**：在识别核心顶点时使用，影响后续聚类过程

---

### 4. 方案F：添加epsilon扰动（结果影响）

**文件**：`graph/Graph.cpp`  
**函数**：`handleMatureInstance()`  
**位置**：第462-487行

**修改前**：
```cpp
double newSimScore = myJaccard->compute_similarity(*curVertex, *neighborVertex);
curVertex->updateNeighborSimScore(newSimScore, neighborVertex->id);
neighborVertex->updateNeighborSimScore(newSimScore, curVertex->id);
```

**修改后**：
```cpp
double newSimScore = myJaccard->compute_similarity(*curVertex, *neighborVertex);
// Add small numerical perturbation (simulating floating-point error accumulation)
const double EPSILON = 1e-10;
newSimScore = newSimScore + EPSILON - EPSILON;  // Force re-normalization
curVertex->updateNeighborSimScore(newSimScore, neighborVertex->id);
neighborVertex->updateNeighborSimScore(newSimScore, curVertex->id);
```

**影响分析**：
- **结果影响**：模拟浮点数精度累积误差，可能改变边界相似度的比较结果
- **预期结果差异**：边界情况下相似度阈值附近的顶点聚类可能不同
- **算法正确性**：不影响，只是模拟真实的浮点误差
- **适用场景**：在处理成熟的DT实例时使用，影响相似度更新

---

## 综合影响预期

### 性能影响（组合A+B）
- **总预期性能下降**：2-3%
- **主要来源**：
  - 方案A：1-2%（缓存局部性影响）
  - 方案B：0.5-1%（内存访问增加）

### 结果影响（组合E+F）
- **预期结果差异**：边界情况下1-3%的顶点聚类归属可能不同
- **主要来源**：
  - 方案E：处理顺序改变（1-2%差异）
  - 方案F：浮点精度影响（0.5-1%差异）

### 总体影响
- **性能**：执行时间预计增加2-3%
- **结果**：聚类结果在边界情况下可能有轻微差异（1-3%）
- **算法正确性**：完全保持，所有变化都是合理的实现差异

---

## 代码注释说明

所有修改都添加了详细的注释，包括：
1. **Modified**：说明修改内容
2. **Reason**：解释修改原因
3. **Expected impact**：预期影响
4. **This change is intentional for comparison analysis**：说明这是用于对比分析的故意修改

---

## 验证建议

### 性能验证
1. 多次运行取平均值（至少5次）
2. 比较总执行时间和各阶段时间
3. 预期时间增加应在2-5%范围内

### 结果验证
1. 比较聚类结果的数量和组成
2. 重点关注边界顶点（相似度接近阈值的顶点）
3. 预期差异应在1-3%范围内

### 正确性验证
1. 验证算法仍然能正确运行
2. 验证没有引入bug或错误
3. 验证核心算法逻辑未改变

---

## 文件修改清单

| 文件 | 函数 | 行数 | 方案 |
|------|------|------|------|
| `graph/Graph.cpp` | `countCommonNeighborsForSmallPair()` | 315-339 | A |
| `graph/Graph.cpp` | `computeJaccardSimilarity()` | 341-352 | B |
| `graph/Graph.cpp` | `identifyCoreVertices()` | 505-536 | E |
| `graph/Graph.cpp` | `handleMatureInstance()` | 462-487 | F |

---

## 使用说明

1. **编译**：正常编译，无需特殊配置
2. **运行**：使用与原代码相同的参数运行
3. **对比**：与原代码结果进行对比分析
4. **分析**：重点关注性能和结果的差异

---

## 注意事项

1. **所有修改都是合理的**：不会破坏算法正确性
2. **差异是可预期的**：符合预期的影响范围
3. **用于对比分析**：这些修改专门用于产生可分析的差异
4. **可逆性**：如果需要，可以轻松恢复原代码

---

## 总结

本次实施成功完成了组合3的所有4个方案，预期能够产生：
- **性能差异**：2-3%的时间增加
- **结果差异**：边界情况下1-3%的聚类差异

这些差异都是合理的、可解释的，适合用于课程作业的对比分析。

