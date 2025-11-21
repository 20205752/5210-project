import re
import itertools
import warnings
from sklearn.metrics import adjusted_rand_score

# Suppress sklearn warnings
warnings.filterwarnings('ignore', category=UserWarning)


# ======================================================
# 解析 txt 文件，提取聚类结果
# ======================================================
def load_clusters_from_txt(filepath):
    """
    从聚类结果txt文件解析出 clusters = [set(int)]
    支持多行 Vertices: 及数字换行
    """
    clusters = []
    current_vertices = None

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith("Cluster"):
                if current_vertices:  # 存上一个簇
                    clusters.append(current_vertices)
                current_vertices = set()
            elif line.startswith("Vertices:"):
                nums = re.findall(r'\d+', line)
                current_vertices |= set(map(int, nums))
            elif re.match(r'^\d', line):  # 数字起始行：顶点续行
                nums = re.findall(r'\d+', line)
                current_vertices |= set(map(int, nums))
        if current_vertices:
            clusters.append(current_vertices)
    return clusters


# ======================================================
# 工具：将簇集合转成 {vertex -> cluster_id}
# ======================================================
def cluster_to_dict(clusters):
    mapping = {}
    for label, c in enumerate(clusters):
        for v in c:
            mapping[v] = label
    return mapping


# ======================================================
# 计算 ARI
# ======================================================
def calc_ARI(truth_clusters, pred_clusters):
    truth_dict = cluster_to_dict(truth_clusters)
    pred_dict  = cluster_to_dict(pred_clusters)

    all_vertices = sorted(set(truth_dict.keys()) | set(pred_dict.keys()))
    true_labels = [truth_dict.get(v, -1) for v in all_vertices]
    pred_labels = [pred_dict.get(v, -1) for v in all_vertices]
    return adjusted_rand_score(true_labels, pred_labels)


# ======================================================
# 计算 MLR
# ======================================================
def calc_MLR(truth_clusters, pred_clusters, max_pairs=None):
    """
    Mis‑Labeled Rate = (# 不一致点对) / (# 总点对)
    可设置 max_pairs 采样比较加速
    """
    truth_dict = cluster_to_dict(truth_clusters)
    pred_dict  = cluster_to_dict(pred_clusters)

    all_vertices = sorted(set(truth_dict.keys()) | set(pred_dict.keys()))
    n = len(all_vertices)
    if n <= 1:
        return 0.0

    total_pairs = n * (n - 1) // 2
    mis_label = 0
    
    # 使用高效的随机采样方法
    if max_pairs and total_pairs > max_pairs:
        import random
        # 使用集合来避免重复采样，直接随机选择顶点对
        sampled_pairs = set()
        while len(sampled_pairs) < max_pairs:
            i_idx = random.randint(0, n - 1)
            j_idx = random.randint(0, n - 1)
            if i_idx != j_idx:
                # 确保 (i,j) 和 (j,i) 被视为同一个点对
                vi, vj = all_vertices[i_idx], all_vertices[j_idx]
                pair = (min(vi, vj), max(vi, vj))
                sampled_pairs.add(pair)
        
        # 计算采样点对的不一致数
        for vi, vj in sampled_pairs:
            same_truth = truth_dict.get(vi) == truth_dict.get(vj)
            same_pred  = pred_dict.get(vi) == pred_dict.get(vj)
            if same_truth != same_pred:
                mis_label += 1
        
        return mis_label / max_pairs
    else:
        # 对于小数据集，直接遍历所有点对
        for i in range(n):
            for j in range(i + 1, n):
                vi, vj = all_vertices[i], all_vertices[j]
                same_truth = truth_dict.get(vi) == truth_dict.get(vj)
                same_pred  = pred_dict.get(vi) == pred_dict.get(vj)
                if same_truth != same_pred:
                    mis_label += 1
        
        return mis_label / total_pairs


# ======================================================
# 主入口：从文件读取并输出两项指标
# ======================================================
def evaluate_clusters(file_true, file_pred, sample_pairs=None):
    print(f"[Loading] Ground truth file: {file_true}")
    print(f"[Loading] Predicted file:    {file_pred}")
    truth_clusters = load_clusters_from_txt(file_true)
    pred_clusters  = load_clusters_from_txt(file_pred)
    print(f"→ Groundtruth clusters: {len(truth_clusters)}")
    print(f"→ Predicted clusters:   {len(pred_clusters)}")

    print("\n[Computing] ARI...")
    ari_value = calc_ARI(truth_clusters, pred_clusters)
    
    if sample_pairs:
        print(f"[Computing] MLR (sampling {sample_pairs} pairs)...")
    else:
        print("[Computing] MLR (computing all pairs, may be slow)...")
    mlr_value = calc_MLR(truth_clusters, pred_clusters, sample_pairs)
    
    print("\n===== Evaluation Results =====")
    print(f"Adjusted Rand Index (ARI): {ari_value:.6f}   ↑ Closer to 1 is better")
    if sample_pairs:
        print(f"Mis‑Labeled Rate  (MLR):  {mlr_value:.6f}   ↓ Closer to 0 is better (sampled estimate)")
    else:
        print(f"Mis‑Labeled Rate  (MLR):  {mlr_value:.6f}   ↓ Closer to 0 is better")
    print("==============================")
    return ari_value, mlr_value


# ======================================================
# 示例运行
# ======================================================
if __name__ == "__main__":
    # Modify to your actual file paths
    gt_path = "results/as-skitter-2m_author_0.5_2.txt"
    pred_path = "results/as-skitter-2m_our_0.5_2.txt"
    # For large datasets, use sampling to speed up MLR computation
    # Larger sample size gives more accurate results but takes longer
    # Recommended range: 100000-1000000
    evaluate_clusters(gt_path, pred_path, sample_pairs=500000)