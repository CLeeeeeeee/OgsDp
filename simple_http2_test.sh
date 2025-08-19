#!/usr/bin/env bash

# 优先使用本仓库 build 目录下的共享库，避免链接到系统已安装的旧版库
export LD_LIBRARY_PATH="/home/lyj/open5gs/build/lib:/home/lyj/open5gs/build/lib/sbi:/home/lyj/open5gs/build/lib/core:/home/lyj/open5gs/build/lib/gtp:/home/lyj/open5gs/build/lib/diameter:${LD_LIBRARY_PATH}"

set -euo pipefail

echo "=== 简化 HTTP/2 测试 ==="

echo "清理之前的进程..."
pkill -f open5gs-dmfd || true
sleep 1

echo "启动 DMF 组件..."
./build/src/dmf/open5gs-dmfd -c /home/lyj/open5gs/install/etc/open5gs/dmf.yaml > /tmp/dmf.log 2>&1 &
DMF_PID=$!
echo "DMF PID: $DMF_PID"
sleep 1
if ! kill -0 $DMF_PID 2>/dev/null; then
    echo "DMF 启动失败"
    cat /tmp/dmf.log || true
    exit 1
fi

echo "DMF 启动成功"

echo "\n=== 测试 HTTP/2 请求 ==="
curl -v --http2-prior-knowledge \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -X POST \
  --data '{"gnb_id":"gnb-1","action":"register","session_id":"sess-1","ran_addr":"127.0.0.100","ran_port":2152,"teid":1001}' \
  http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync || true

sleep 3

echo "=== DMF 日志 ==="
sed -n '1,200p' /tmp/dmf.log || true

echo "\n清理测试环境..."
kill $DMF_PID 2>/dev/null || true
sleep 1

echo "=== 测试完成 ===" 