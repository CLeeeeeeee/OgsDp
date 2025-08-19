#!/usr/bin/env bash

set -euo pipefail

# 优先使用本仓库 build 目录下的共享库
export LD_LIBRARY_PATH="/home/lyj/open5gs/build/lib:/home/lyj/open5gs/build/lib/sbi:/home/lyj/open5gs/build/lib/core:/home/lyj/open5gs/build/lib/gtp:/home/lyj/open5gs/build/lib/diameter:${LD_LIBRARY_PATH:-}"

log_dir=/tmp
nrf_log=$log_dir/nrf.log
amf_log=$log_dir/amf.log
dmf_log=$log_dir/dmf.log
dsmf_log=$log_dir/dsmf.log
df_log=$log_dir/df.log

stop_all() {
  pkill -f open5gs-nrfd || true; pkill -9 -f open5gs-nrfd || true
  pkill -f open5gs-amfd || true; pkill -9 -f open5gs-amfd || true
  pkill -f open5gs-dmfd || true; pkill -9 -f open5gs-dmfd || true
  pkill -f open5gs-dsmfd || true; pkill -9 -f open5gs-dsmfd || true
  pkill -f open5gs-dfd || true; pkill -9 -f open5gs-dfd || true
}

start_nf() {
  local bin=$1 cfg=$2 out=$3
  "$bin" -c "$cfg" >"$out" 2>&1 &
  echo $!
}

check_alive() {
  local pid=$1 name=$2 out=$3
  sleep 1
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "[ERR] $name 启动失败"; sed -n '1,200p' "$out" || true; exit 1
  fi
}

wait_port_free_udp() {
  local addr=$1 port=$2
  for i in {1..20}; do
    if ss -lunp | grep -q "$addr:$port"; then
      sleep 0.3
    else
      return 0
    fi
  done
  echo "[WARN] UDP $addr:$port 仍被占用，继续尝试启动" >&2
}

wait_port_listen_tcp() {
  local addr=$1 port=$2 timeout=${3:-10}
  for ((i=0;i<timeout*10;i++)); do
    if ss -ltn | grep -q "${addr}:${port}"; then
      return 0
    fi
    sleep 0.1
  done
  echo "[WARN] TCP ${addr}:${port} 未监听就绪" >&2
  return 1
}

echo "清理旧进程..."; stop_all; sleep 1
wait_port_free_udp 127.0.0.22 8805 || true
wait_port_free_udp 127.0.0.23 8806 || true

echo "启动 NRF..."
NRF_PID=$(start_nf /home/lyj/open5gs/build/src/nrf/open5gs-nrfd /home/lyj/open5gs/install/etc/open5gs/nrf.yaml "$nrf_log")
check_alive "$NRF_PID" NRF "$nrf_log"

echo "启动 AMF..."
AMF_PID=$(start_nf /home/lyj/open5gs/build/src/amf/open5gs-amfd /home/lyj/open5gs/install/etc/open5gs/amf.yaml "$amf_log")
check_alive "$AMF_PID" AMF "$amf_log"

echo "启动 DMF..."
DMF_PID=$(start_nf /home/lyj/open5gs/build/src/dmf/open5gs-dmfd /home/lyj/open5gs/install/etc/open5gs/dmf.yaml "$dmf_log")
check_alive "$DMF_PID" DMF "$dmf_log"

echo "启动 DSMF..."
DSMF_PID=$(start_nf /home/lyj/open5gs/build/src/dsmf/open5gs-dsmfd /home/lyj/open5gs/install/etc/open5gs/dsmf.yaml "$dsmf_log")
check_alive "$DSMF_PID" DSMF "$dsmf_log"

echo "启动 DF..."
DF_PID=$(start_nf /home/lyj/open5gs/build/src/df/open5gs-dfd /home/lyj/open5gs/install/etc/open5gs/df.yaml "$df_log")
check_alive "$DF_PID" DF "$df_log"

echo "等待 5 秒以便 NF 向 NRF 注册..."; sleep 5

summ() {
  local name=$1 out=$2
  echo "==== $name 摘要 ===="
  grep -E "registered|Register|NF registered|Nnrf|association" -i "$out" || true
}

summ NRF "$nrf_log"
summ AMF "$amf_log"
summ DMF "$dmf_log"
summ DSMF "$dsmf_log"
summ DF "$df_log"

echo "\n验证 DMF HTTP/2 接口..."
# 先模拟 AMF 注册 gNB（仅 gnb_id 和 action），失败则重试最多2次
attempts=0
while (( attempts < 3 )); do
  if curl -sS -v --http2-prior-knowledge \
    -H "Content-Type: application/json" \
    -H "Accept: application/json" \
    --data '{"gnb_id":"gnb-1","action":"register"}' \
    http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync; then
    break
  fi
  attempts=$((attempts+1))
  echo "[WARN] gNB 注册请求失败，重试第 $attempts 次" >&2
  sleep 1
done

# 等待 DMF HTTP 端口监听稳定
wait_port_listen_tcp 127.0.0.21 7777 10 || true
sleep 2

# 轮询DMF日志，等待gNB入库完成
echo "等待 DMF 确认 Added gNB: gnb-1 ..."
for i in {1..20}; do
  if grep -q "Added gNB: gnb-1" "$dmf_log"; then
    echo "DMF 已确认 gNB 入库"
    break
  fi
  sleep 0.5
done
if ! grep -q "Added gNB: gnb-1" "$dmf_log"; then
  echo "[ERR] 超时未见 DMF 入库 gNB 记录"; tail -100 "$dmf_log" || true; exit 1
fi

# 再发送包含会话级 RAN 地址的同步请求
DMF_OK=1
set +e
curl -v --http2-prior-knowledge \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  --data '{"gnb_id":"gnb-1","action":"register","session_id":"sess-1","ran_addr":"127.0.0.100","ran_port":2152,"teid":1001}' \
  http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync
DMF_RC=$?
set -e
if [ $DMF_RC -ne 0 ]; then
  echo "[WARN] 发送会话级地址到 DMF 失败（rc=$DMF_RC），改为直接发送给 DSMF"
  # DSMF 回退：先注册，再发地址
  curl -sS -v --http2-prior-knowledge \
    -H "Content-Type: application/json" \
    -H "Accept: application/json" \
    --data '{"gnb_id":"gnb-1","action":"register"}' \
    http://127.0.0.23:7777/nsmf-pdusession/v1/gnb-sync || true
  sleep 1
  curl -v --http2-prior-knowledge \
    -H "Content-Type: application/json" \
    -H "Accept: application/json" \
    --data '{"gnb_id":"gnb-1","action":"register","session_id":"sess-1","ran_addr":"127.0.0.100","ran_port":2152,"teid":1001}' \
    http://127.0.0.23:7777/nsmf-pdusession/v1/gnb-sync || true
fi

echo "\n最近 50 行 DMF 日志："
tail -50 "$dmf_log" || true

echo "\n最近 50 行 DF 日志（查找会话级 RAN 地址）："
sed -n '1,200p' "$df_log" | grep -E "\[DF\] Received session-level RAN addr" -n || true

echo "\n如需结束：kill $NRF_PID $AMF_PID $DMF_PID $DSMF_PID $DF_PID" 