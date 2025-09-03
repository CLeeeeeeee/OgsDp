#!/bin/bash

# 一键集成测试脚本 - AMF/NRF/DMF/DSMF/DF 自动启动与校验
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
PREFIX="$ROOT_DIR/install"
export LD_LIBRARY_PATH="$PREFIX/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"

echo "=== 开始集成测试 (AMF + NRF + DMF + DSMF + DF) ==="

echo "1) 清理之前的进程..."
pkill -9 open5gs- || true
sleep 2

echo "2) 启动组件..."
"$PREFIX/bin/open5gs-nrfd" & NRF_PID=$!; sleep 1
"$PREFIX/bin/open5gs-amfd" & AMF_PID=$!; sleep 1
"$PREFIX/bin/open5gs-dmfd" & DMF_PID=$!; sleep 1
"$PREFIX/bin/open5gs-dsmfd" & DSMF_PID=$!; sleep 1
"$PREFIX/bin/open5gs-dfd"  & DF_PID=$!;  sleep 2
python3 "$ROOT_DIR/tools/ran_proxy.py" 127.0.0.30 7788 >/dev/null 2>&1 & RAN_PROXY_PID=$!; sleep 1

echo "3) 检查端口监听..."
echo "NRF  127.0.0.10:7777 => $(ss -lnt | grep '127.0.0.10:7777' || echo '未监听')"
echo "AMF  127.0.0.5:7777  => $(ss -lnt | grep '127.0.0.5:7777'  || echo '未监听')"
echo "DMF  127.0.0.21:7777 => $(ss -lnt | grep '127.0.0.21:7777' || echo '未监听')"
echo "DSMF 127.0.0.23:7777 => $(ss -lnt | grep '127.0.0.23:7777' || echo '未监听')"
echo "DF   127.0.0.22:7779 => $(ss -lnt | grep '127.0.0.22:7779' || echo '未监听')"
echo "DF PFCP   127.0.0.22:8805 => $(ss -lun | grep '127.0.0.22:8805' || echo '未监听')"
echo "DSMF PFCP 127.0.0.23:8806 => $(ss -lun | grep '127.0.0.23:8806' || echo '未监听')"

echo "4) 等待 NF 注册到 NRF..."
sleep 3
curl -s --http2-prior-knowledge http://127.0.0.10:7777/nnrf-nfm/v1/nf-instances >/dev/null || true

echo "5) 触发 gNB -> RAN 代理 -> DMF 流程..."
# gNB 先发到 RAN 代理，再由 RAN 代理转发到 DMF
curl -s -H 'content-type: application/json' \
  -d '{"gnb_id":"gNB-1","action":"register"}' \
  http://127.0.0.30:7788 >/dev/null || true
sleep 1
curl -s -H 'content-type: application/json' \
  -d '{"gnb_id":"gNB-1","action":"register","session_id":"sess-001","ran_addr":"127.0.0.50","ran_port":2152,"teid":3735928559}' \
  http://127.0.0.30:7788 >/dev/null || true

echo "6) 等待 PFCP 事务..."
sleep 3

echo "7) 摘要日志:"
echo "--- DMF ---";  tail -n 20 "$PREFIX/var/log/open5gs/dmf.log"  || true
echo "--- DSMF ---"; tail -n 20 "$PREFIX/var/log/open5gs/dsmf.log" || true
echo "--- DF ---";   tail -n 50 "$PREFIX/var/log/open5gs/df.log"   || true

echo "8) 校验 DF 是否打印会话级 RAN 地址..."
if grep -q "Rcv RAN addr" "$PREFIX/var/log/open5gs/df.log"; then
  echo "✓ DF 已打印会话级 RAN 地址:"
  grep "Rcv RAN addr" "$PREFIX/var/log/open5gs/df.log" | tail -3
  RC=0
else
  echo "✗ 未发现 DF 的 RAN 地址打印，DF 日志片段:"
  grep -i "RAN\|TEID\|F-TEID" "$PREFIX/var/log/open5gs/df.log" | tail -10 || true
  RC=1
fi

echo "9) 清理进程..."
kill $RAN_PROXY_PID $DF_PID $DSMF_PID $DMF_PID $AMF_PID $NRF_PID 2>/dev/null || true
sleep 1

echo "=== 集成测试结束 (RC=$RC) ==="
exit $RC
