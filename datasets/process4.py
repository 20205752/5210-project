#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
as-skitter 数据集预处理脚本
将 as-skitter.txt 转换为程序需要的二进制格式，并生成更新文件
"""

import struct
import argparse
import random
import os
import sys
from collections import defaultdict


def read_graph(input_file, zero_index=True, skip_comments=True):
    """
    读取图文件

    Args:
        input_file: 输入文件路径
        zero_index: 顶点编号是否从0开始
        skip_comments: 是否跳过注释行

    Returns:
        edges: 边列表
        vertices: 顶点集合
        stats: 统计信息
    """
    edges_set = set()
    vertices = set()
    stats = {
        'total_lines': 0,
        'comment_lines': 0,
        'empty_lines': 0,
        'self_loops': 0,
        'invalid_lines': 0
    }

    print(f"正在读取图文件: {input_file}")

    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                stats['total_lines'] += 1

                # 跳过空行
                line = line.strip()
                if not line:
                    stats['empty_lines'] += 1
                    continue

                # 跳过注释行
                if skip_comments and line.startswith('#'):
                    stats['comment_lines'] += 1
                    continue

                # 解析边
                parts = line.split()
                if len(parts) < 2:
                    stats['invalid_lines'] += 1
                    continue

                try:
                    u = int(parts[0])
                    v = int(parts[1])
                except ValueError:
                    stats['invalid_lines'] += 1
                    continue

                # 转换为从1开始
                if zero_index:
                    u += 1
                    v += 1

                # 跳过自环
                if u == v:
                    stats['self_loops'] += 1
                    continue

                # 无向图：统一边的方向（u < v）
                if u > v:
                    u, v = v, u

                edges_set.add((u, v))
                vertices.add(u)
                vertices.add(v)

                # 进度输出
                if line_num % 1000000 == 0:
                    print(f"  已处理 {line_num:,} 行, 找到 {len(edges_set):,} 条边...")

    except FileNotFoundError:
        print(f"错误: 找不到文件 {input_file}")
        sys.exit(1)
    except Exception as e:
        print(f"错误: 读取文件时出现问题: {e}")
        sys.exit(1)

    edges = sorted(edges_set)  # 排序以便输出
    stats['vertices'] = len(vertices)
    stats['edges'] = len(edges)
    stats['max_vertex_id'] = max(vertices) if vertices else 0

    return edges, vertices, stats


def write_binary_graph(output_file, edges, vertices):
    """
    写入二进制图文件

    格式:
    - 前4字节: 顶点数 n (unsigned int)
    - 接着4字节: 边数 m*2 (unsigned int, 每条边用两个int表示)
    - 接着 m*2 个 int: 边的列表
    """
    n = len(vertices)
    m = len(edges)

    print(f"\n正在写入二进制文件: {output_file}")
    print(f"  顶点数: {n:,}")
    print(f"  边数: {m:,}")

    try:
        with open(output_file, 'wb') as f:
            # 写入顶点数
            f.write(struct.pack('I', n))
            # 写入边数（每条边用两个int表示）
            f.write(struct.pack('I', m * 2))

            # 写入边列表
            for i, (u, v) in enumerate(edges):
                f.write(struct.pack('i', u))
                f.write(struct.pack('i', v))

                if (i + 1) % 1000000 == 0:
                    print(f"  已写入 {i + 1:,} 条边...")

        print(f"✓ 二进制文件写入完成: {output_file}")
        return True

    except Exception as e:
        print(f"错误: 写入文件时出现问题: {e}")
        return False


# def generate_updates(graph_file, output_file, update_count, insert_ratio=0.5, seed=None):
#     """
#     生成更新文件
#
#     Args:
#         graph_file: 二进制图文件路径
#         output_file: 输出更新文件路径
#         update_count: 更新操作数量
#         insert_ratio: 插入操作的比例 (0-1)
#         seed: 随机种子
#     """
#     if seed is not None:
#         random.seed(seed)
#
#     print(f"\n正在生成更新文件: {output_file}")
#     print(f"  更新数量: {update_count:,}")
#     print(f"  插入比例: {insert_ratio:.2%}")
#
#     # 读取图文件获取顶点和边信息
#     try:
#         with open(graph_file, 'rb') as f:
#             n = struct.unpack('I', f.read(4))[0]
#             m = struct.unpack('I', f.read(4))[0]
#             edges = []
#             for _ in range(m // 2):
#                 u = struct.unpack('i', f.read(4))[0]
#                 v = struct.unpack('i', f.read(4))[0]
#                 edges.append((u, v))
#
#         print(f"  图信息: {n:,} 个顶点, {len(edges):,} 条边")
#
#     except Exception as e:
#         print(f"错误: 读取图文件时出现问题: {e}")
#         return False
#
#     # 使用集合存储现有的边，以便快速查找
#     existing_edges = set(edges)
#
#     # 生成更新序列
#     insert_count = 0
#     delete_count = 0
#
#     try:
#         with open(output_file, 'w') as f:
#             for i in range(update_count):
#                 if random.random() < insert_ratio:
#                     # 插入操作：随机生成一条边
#                     u = random.randint(1, n)
#                     v = random.randint(1, n)
#                     while u == v:
#                         v = random.randint(1, n)
#                     if u > v:
#                         u, v = v, u
#                     f.write(f"1 {u} {v}\n")
#                     insert_count += 1
#                 else:
#                     # 删除操作：从现有边中随机选择
#                     if edges:
#                         u, v = random.choice(edges)
#                         # 如果删除的边不存在于现有边集合中，则跳过
#                         if (u, v) not in existing_edges:
#                             continue
#                         f.write(f"0 {u} {v}\n")
#                         delete_count += 1
#                         # 删除该边
#                         existing_edges.remove((u, v))
#                     else:
#                         # 如果没有边可删，改为插入
#                         u = random.randint(1, n)
#                         v = random.randint(1, n)
#                         while u == v:
#                             v = random.randint(1, n)
#                         if u > v:
#                             u, v = v, u
#                         f.write(f"1 {u} {v}\n")
#                         insert_count += 1
#
#                 if (i + 1) % 100000 == 0:
#                     print(f"  已生成 {i + 1:,} 个更新...")
#
#         print(f"✓ 更新文件生成完成: {output_file}")
#         print(f"  插入操作: {insert_count:,}")
#         print(f"  删除操作: {delete_count:,}")
#         return True
#
#     except Exception as e:
#         print(f"错误: 生成更新文件时出现问题: {e}")
#         return False


def generate_updates(graph_file, output_file, update_count, insert_ratio=0.5, seed=None):
    """
    生成更新文件

    Args:
        graph_file: 二进制图文件路径
        output_file: 输出更新文件路径
        update_count: 更新操作数量
        insert_ratio: 插入操作的比例 (0-1)
        seed: 随机种子
    """
    if seed is not None:
        random.seed(seed)

    print(f"\n正在生成更新文件: {output_file}")
    print(f"  更新数量: {update_count:,}")
    print(f"  插入比例: {insert_ratio:.2%}")

    # 读取图文件获取顶点和边信息
    try:
        with open(graph_file, 'rb') as f:
            n = struct.unpack('I', f.read(4))[0]
            m = struct.unpack('I', f.read(4))[0]
            edges = []
            for _ in range(m // 2):
                u = struct.unpack('i', f.read(4))[0]
                v = struct.unpack('i', f.read(4))[0]
                edges.append((u, v))

        print(f"  图信息: {n:,} 个顶点, {len(edges):,} 条边")

    except Exception as e:
        print(f"错误: 读取图文件时出现问题: {e}")
        return False

    # 使用集合存储现有的边，以便快速查找
    existing_edges = set(edges)

    # 生成更新序列
    insert_count = 0
    delete_count = 0

    try:
        with open(output_file, 'w') as f:
            for i in range(update_count):
                if random.random() < insert_ratio:
                    # 插入操作：随机生成一条边
                    u = random.randint(1, n)
                    v = random.randint(1, n)
                    while u == v:
                        v = random.randint(1, n)
                    if u > v:
                        u, v = v, u
                    # 检查该边是否已经存在，若存在则跳过
                    if (u, v) in existing_edges:
                        continue  # 跳过已存在的边
                    f.write(f"1 {u} {v}\n")
                    insert_count += 1
                    existing_edges.add((u, v))  # 将新的边添加到集合中
                else:
                    # 删除操作：从现有边中随机选择
                    if edges:
                        u, v = random.choice(edges)
                        # 如果删除的边不存在于现有边集合中，则跳过
                        if (u, v) not in existing_edges:
                            continue  # 跳过不存在的边
                        f.write(f"0 {u} {v}\n")
                        delete_count += 1
                        # 删除该边
                        existing_edges.remove((u, v))
                    else:
                        # 如果没有边可删，改为插入
                        u = random.randint(1, n)
                        v = random.randint(1, n)
                        while u == v:
                            v = random.randint(1, n)
                        if u > v:
                            u, v = v, u
                        f.write(f"1 {u} {v}\n")
                        insert_count += 1
                        existing_edges.add((u, v))  # 将新的边添加到集合中

                if (i + 1) % 100000 == 0:
                    print(f"  已生成 {i + 1:,} 个更新...")

        print(f"✓ 更新文件生成完成: {output_file}")
        print(f"  插入操作: {insert_count:,}")
        print(f"  删除操作: {delete_count:,}")
        return True

    except Exception as e:
        print(f"错误: 生成更新文件时出现问题: {e}")
        return False



def print_stats(stats):
    """打印统计信息"""
    print("\n" + "=" * 60)
    print("统计信息:")
    print("=" * 60)
    print(f"总行数: {stats['total_lines']:,}")
    print(f"注释行: {stats['comment_lines']:,}")
    print(f"空行: {stats['empty_lines']:,}")
    print(f"无效行: {stats['invalid_lines']:,}")
    print(f"自环: {stats['self_loops']:,}")
    print(f"顶点数: {stats['vertices']:,}")
    print(f"边数: {stats['edges']:,}")
    print(f"最大顶点ID: {stats['max_vertex_id']:,}")

    # 检查顶点编号连续性
    if stats['max_vertex_id'] != stats['vertices']:
        print(f"\n⚠ 警告: 顶点编号不连续！")
        print(f"   最大ID: {stats['max_vertex_id']:,}")
        print(f"   实际顶点数: {stats['vertices']:,}")
        print(f"   缺失顶点: {stats['max_vertex_id'] - stats['vertices']:,}")
        print(f"   程序假设顶点编号从1到n连续，可能影响部分功能")
    else:
        print(f"\n✓ 顶点编号连续")


def main():
    parser = argparse.ArgumentParser(
        description='as-skitter 数据集预处理工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""示例:
  # 基本用法
  python preprocess_askitter.py as-skitter.txt as-skitter.bin
  # 生成更新文件
  python preprocess_askitter.py as-skitter.txt as-skitter.bin --generate-updates 100000
  # 自定义插入比例
  python preprocess_askitter.py as-skitter.txt as-skitter.bin --generate-updates 100000 --insert-ratio 0.7
        """
    )

    parser.add_argument('input_file', help='输入图文件路径 (as-skitter.txt)')
    parser.add_argument('output_file', help='输出二进制图文件路径 (as-skitter.bin)')
    parser.add_argument('--zero-index', action='store_true', default=True,
                        help='顶点编号从0开始（默认: True）')
    parser.add_argument('--skip-comments', action='store_true', default=True,
                        help='跳过注释行（默认: True）')
    parser.add_argument('--generate-updates', type=int, metavar='N',
                        help='生成更新文件，指定更新操作数量')
    parser.add_argument('--update-file', default=None,
                        help='更新文件输出路径（默认: <output_file>_updates.txt）')
    parser.add_argument('--insert-ratio', type=float, default=0.5,
                        help='插入操作的比例 (0-1, 默认: 0.5)')
    parser.add_argument('--seed', type=int, default=None,
                        help='随机种子（用于生成更新文件）')

    args = parser.parse_args()

    # 检查输入文件
    if not os.path.exists(args.input_file):
        print(f"错误: 找不到输入文件 {args.input_file}")
        sys.exit(1)

    # 步骤1: 读取并转换图文件
    print("=" * 60)
    print("步骤1: 读取图文件")
    print("=" * 60)
    edges, vertices, stats = read_graph(
        args.input_file,
        zero_index=args.zero_index,
        skip_comments=args.skip_comments
    )

    print_stats(stats)

    # 步骤2: 写入二进制文件
    print("\n" + "=" * 60)
    print("步骤2: 写入二进制文件")
    print("=" * 60)
    if not write_binary_graph(args.output_file, edges, vertices):
        sys.exit(1)

    # 步骤3: 生成更新文件（如果指定）
    if args.generate_updates:
        print("\n" + "=" * 60)
        print("步骤3: 生成更新文件")
        print("=" * 60)
        update_file = args.update_file
        if update_file is None:
            # 默认更新文件名
            base_name = os.path.splitext(args.output_file)[0]
            update_file = f"{base_name}_updates.txt"

        if not generate_updates(
                args.output_file,
                update_file,
                args.generate_updates,
                args.insert_ratio,
                args.seed
        ):
            sys.exit(1)

    # 完成
    print("\n" + "=" * 60)
    print("✓ 预处理完成！")
    print("=" * 60)
    print(f"输出文件: {args.output_file}")
    if args.generate_updates:
        update_file = args.update_file or f"{os.path.splitext(args.output_file)[0]}_updates.txt"
        print(f"更新文件: {update_file}")
    print("\n运行程序:")
    update_file = args.update_file or (
        f"{os.path.splitext(args.output_file)[0]}_updates.txt" if args.generate_updates else "updates.txt")
    if args.generate_updates:
        print(f"  ./DynamicStrClu_ours -graph {args.output_file} -update {update_file} -rho 0.01")
    else:
        print(f"  ./DynamicStrClu_ours -graph {args.output_file} -update <update_file> -rho 0.01")


if __name__ == '__main__':
    main()
