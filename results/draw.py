import matplotlib.pyplot as plt
import numpy as np

# 数据
datasets = ['original', 'wiki-topcats', 'WikiTalk-2m', 'as-skitter-2m']

# Update时间数据 (单位：秒)
our_update = [0.000002803, 0.000005875, 0.000003571, 0.000005872]
author_update = [0.000002735, 0.000006035, 0.000003763, 0.000005231]

# Query时间数据 (单位：秒)
our_query = [0.009258938, 0.256123582, 0.11383811, 0.28171243]
author_query = [0.007450875, 0.22022063, 0.118039222, 0.289607382]

# 创建图形和子图
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

# 设置柱状图的位置和宽度
x = np.arange(len(datasets))
width = 0.35

# 第一个子图：Update时间比较
bars1 = ax1.bar(x - width/2, our_update, width, label='Our', color='skyblue', edgecolor='black')
bars2 = ax1.bar(x + width/2, author_update, width, label='Author', color='lightcoral', edgecolor='black')

ax1.set_xlabel('Dataset')
ax1.set_ylabel('Time (seconds)')
ax1.set_title('Average Update Time Comparison')
ax1.set_xticks(x)
ax1.set_xticklabels(datasets, rotation=45)
ax1.legend()

# 在柱子上添加数值标签（科学计数法显示）
for bar in bars1:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height,
             f'{height:.2e}', ha='center', va='bottom', fontsize=8)

for bar in bars2:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height,
             f'{height:.2e}', ha='center', va='bottom', fontsize=8)

# 第二个子图：Query时间比较
bars3 = ax2.bar(x - width/2, our_query, width, label='Our', color='skyblue', edgecolor='black')
bars4 = ax2.bar(x + width/2, author_query, width, label='Author', color='lightcoral', edgecolor='black')

ax2.set_xlabel('Dataset')
ax2.set_ylabel('Time (seconds)')
ax2.set_title('Average Query Time Comparison')
ax2.set_xticks(x)
ax2.set_xticklabels(datasets, rotation=45)
ax2.legend()

# 在柱子上添加数值标签
for bar in bars3:
    height = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2., height,
             f'{height:.3f}', ha='center', va='bottom', fontsize=8)

for bar in bars4:
    height = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2., height,
             f'{height:.3f}', ha='center', va='bottom', fontsize=8)

# 调整布局
plt.tight_layout()

# 显示图形
plt.show()

# 可选：保存图片
# plt.savefig('performance_comparison.png', dpi=300, bbox_inches='tight')