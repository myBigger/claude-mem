#!/bin/bash
# PrisonSIS Push Script
# 在有网络的机器上运行此脚本

set -e

REPO="https://github.com/myBigger/PrisonSIS.git"
BRANCH="master"

echo "=== PrisonSIS Git Push ==="

# 如果远程不存在，先创建
echo "检查远程仓库..."
if ! git remote | grep -q origin; then
    echo "添加远程仓库..."
    git remote add origin "$REPO"
fi

# 推送主分支
echo "推送到 GitHub..."
git push -u origin "$BRANCH"

# 推送所有标签
echo "推送标签..."
git push --tags

echo ""
echo "=== 推送完成！==="
echo "仓库地址: https://github.com/myBigger/PrisonSIS"
