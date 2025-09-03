#!/bin/bash
set -euo pipefail

echo "=== 开始集成测试 (NRF + AMF + DMF + DSMF + DF + DNF) ==="

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
PREFIX="$ROOT_DIR/install"
export LD_LIBRARY_PATH="$PREFIX/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"

echo "1) 清理之前的进程..."
pkill -9 open5gs- || true
sleep 1

echo "2) 启动组件..."
"$PREFIX/bin/open5gs-nrfd"  & NRF_PID=$!;  sleep 1
"$PREFIX/bin/open5gs-amfd"  & AMF_PID=$!;  sleep 1
"$PREFIX/bin/open5gs-dmfd"  & DMF_PID=$!;  sleep 1
"$PREFIX/bin/open5gs-dsmfd" & DSMF_PID=$!; sleep 2
"$PREFIX/bin/open5gs-dnfd"  & DNF_PID=$!;  sleep 2
"$PREFIX/bin/open5gs-dfd"   & DF_PID=$!;   sleep 2

echo "3) 检查端口监听..."
echo "NRF  127.0.0.10:7777 => $(ss -lnt | grep '127.0.0.10:7777' || echo '未监听')"
echo "AMF  127.0.0.5:7777  => $(ss -lnt | grep '127.0.0.5:7777'  || echo '未监听')"
echo "DMF  127.0.0.21:7777 => $(ss -lnt | grep '127.0.0.21:7777' || echo '未监听')"
echo "DSMF 127.0.0.23:7777 => $(ss -lnt | grep '127.0.0.23:7777' || echo '未监听')"
echo "DF PFCP   127.0.0.22:8805 => $(ss -lun | grep '127.0.0.22:8805' || echo '未监听')"
echo "DSMF PFCP 127.0.0.23:8806 => $(ss -lun | grep '127.0.0.23:8806' || echo '未监听')"
echo "DNF GTP-U :2154        => $(ss -lun | grep ':2154 ' || echo '未监听')"

echo "4) 等待 NF 注册到 NRF..."
sleep 3
curl -s --http2-prior-knowledge http://127.0.0.10:7777/nnrf-nfm/v1/nf-instances >/dev/null || true

echo "5) 触发 DMF 的 gNB 注册与 RAN 地址同步..."
# gNB 注册（简格式）
curl -s --http2-prior-knowledge \
  -H 'content-type: application/json' \
  -d '{"gnb_id":"gNB-1","action":"register"}' \
  http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync >/dev/null || true

sleep 1
# 会话级 RAN 地址同步（含 TEID）
curl -s --http2-prior-knowledge \
  -H 'content-type: application/json' \
  -d '{"gnb_id":"gNB-1","action":"register","session_id":"sess-001","ran_addr":"127.0.0.50","ran_port":2152,"teid":3735928559}' \
  http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync >/dev/null || true

echo "6) 等待 PFCP 事务..."
sleep 3

echo "7) 摘要日志:"
echo "--- DF ---";   tail -n 80 "$PREFIX/var/log/open5gs/df.log"   || true
echo "--- DSMF ---"; tail -n 40 "$PREFIX/var/log/open5gs/dsmf.log" || true
echo "--- DMF ---";  tail -n 40 "$PREFIX/var/log/open5gs/dmf.log"  || true
echo "--- DNF ---";  tail -n 40 "$PREFIX/var/log/open5gs/dnf.log"  || true

echo "8) 校验 DF 是否打印会话级 RAN 地址..."
RC=0
if grep -q "Rcv RAN addr" "$PREFIX/var/log/open5gs/df.log"; then
  grep "Rcv RAN addr" "$PREFIX/var/log/open5gs/df.log" | tail -3
  echo "✓ DF 已打印会话级 RAN 地址"
else
  RC=1
  echo "✗ 未发现 DF 的 RAN 地址打印，DF 日志片段:"
  grep -i "RAN\|TEID\|F-TEID" "$PREFIX/var/log/open5gs/df.log" | tail -10 || true
fi

echo "9) 校验 DF 是否打印会话级 RAN 地址..."
RC=0
if grep -q "Rcv RAN addr" "$PREFIX/var/log/open5gs/df.log"; then
  grep "Rcv RAN addr" "$PREFIX/var/log/open5gs/df.log" | tail -3
  echo "✓ DF 已打印会话级 RAN 地址"
else
  RC=1
  echo "✗ 未发现 DF 的 RAN 地址打印，DF 日志片段:"
  grep -i "RAN\|TEID\|F-TEID" "$PREFIX/var/log/open5gs/df.log" | tail -10 || true
fi

# 检查是否保持运行
if [ "${KEEP_RUNNING:-}" = "1" ]; then
    echo "10) 保持网元运行中..."
    echo "    PID: NRF=$NRF_PID, AMF=$AMF_PID, DMF=$DMF_PID, DSMF=$DSMF_PID, DF=$DF_PID, DNF=$DNF_PID"
    echo "    按 Ctrl+C 停止所有网元"
    echo "    或运行: kill $NRF_PID $AMF_PID $DMF_PID $DSMF_PID $DF_PID $DNF_PID"
    
    # 等待用户中断
    trap 'echo "收到中断信号，清理进程..."; kill $DF_PID $DNF_PID $DSMF_PID $DMF_PID $AMF_PID $NRF_PID 2>/dev/null || true; exit 0' INT TERM
    wait
else
    echo "10) 清理进程..."
    kill $DF_PID $DNF_PID $DSMF_PID $DMF_PID $AMF_PID $NRF_PID 2>/dev/null || true
    sleep 1
fi

echo "=== 集成测试结束 (RC=$RC) ==="
exit $RC


