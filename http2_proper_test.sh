#!/bin/bash

# HTTP/2 SBI 消息处理测试脚本

echo "=== HTTP/2 SBI 消息处理测试 ==="

# 设置环境变量
export LD_LIBRARY_PATH=/home/lyj/open5gs/install/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
export PATH=/home/lyj/open5gs/install/bin:$PATH

# 创建日志目录
mkdir -p /tmp/open5gs_test/logs

# 清理之前的进程
echo "清理之前的进程..."
pkill -f open5gs-dfd
pkill -f open5gs-dmfd
pkill -f open5gs-dsmfd
sleep 3

# 启动 DF 组件
echo "启动 DF 组件..."
./build/src/df/open5gs-dfd -c /home/lyj/open5gs/install/etc/open5gs/df.yaml > /tmp/open5gs_test/logs/df.log 2>&1 &
DF_PID=$!
echo "DF PID: $DF_PID"
sleep 3

# 启动 DMF 组件
echo "启动 DMF 组件..."
./build/src/dmf/open5gs-dmfd -c /home/lyj/open5gs/install/etc/open5gs/dmf.yaml > /tmp/open5gs_test/logs/dmf.log 2>&1 &
DMF_PID=$!
echo "DMF PID: $DMF_PID"
sleep 3

# 启动 DSMF 组件
echo "启动 DSMF 组件..."
./build/src/dsmf/open5gs-dsmfd -c /home/lyj/open5gs/install/etc/open5gs/dsmf.yaml > /tmp/open5gs_test/logs/dsmf.log 2>&1 &
DSMF_PID=$!
echo "DSMF PID: $DSMF_PID"
sleep 5

# 检查进程状态
echo "检查进程状态..."
ps aux | grep -E "(open5gs-dfd|open5gs-dmfd|open5gs-dsmfd)" | grep -v grep

# 检查端口监听状态
echo "检查端口监听状态..."
ss -tlnp | grep -E "(7777|8805|2152)"

# 测试 1: 使用 curl 的 HTTP/2 升级
echo ""
echo "=== 测试 1: 使用 curl 的 HTTP/2 升级 ==="
curl -X POST \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -H "Connection: Upgrade, HTTP2-Settings" \
  -H "Upgrade: h2c" \
  -H "HTTP2-Settings: AAMAAABkAAQCAAAAAAIAAAAA" \
  -d '{"gnb_id":"test_gnb_001","action":"register","session_id":"session_001","ran_addr":"192.168.1.100","ran_port":2152,"teid":12345}' \
  "http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync" \
  -v \
  --max-time 10

echo ""
echo "等待 3 秒..."
sleep 3

# 显示 DMF 日志
echo "=== DMF 日志 ==="
tail -10 /tmp/open5gs_test/logs/dmf.log

# 测试 2: 使用 curl 的 --http2 标志
echo ""
echo "=== 测试 2: 使用 curl 的 --http2 标志 ==="
curl -X POST \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{"gnb_id":"test_gnb_002","action":"register","session_id":"session_002","ran_addr":"192.168.1.101","ran_port":2152,"teid":12346}' \
  "http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync" \
  --http2 \
  -v \
  --max-time 10

echo ""
echo "等待 5 秒观察日志..."
sleep 5

# 显示各组件日志
echo "=== DMF 日志 ==="
tail -15 /tmp/open5gs_test/logs/dmf.log

echo ""
echo "=== DSMF 日志 ==="
tail -15 /tmp/open5gs_test/logs/dsmf.log

# 测试 3: 使用 curl 的 --http2-prior-knowledge 标志
echo ""
echo "=== 测试 3: 使用 curl 的 --http2-prior-knowledge 标志 ==="
curl -X POST \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{"gnb_id":"test_gnb_003","action":"register","session_id":"session_003","ran_addr":"192.168.1.102","ran_port":2152,"teid":12347}' \
  "http://127.0.0.21:7777/ndmf-gnb-sync/v1/gnb-sync" \
  --http2-prior-knowledge \
  -v \
  --max-time 10

echo ""
echo "等待 3 秒..."
sleep 3

# 测试 4: 直接测试 DSMF
echo ""
echo "=== 测试 4: 直接测试 DSMF ==="
curl -X POST \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{"gnb_id":"test_gnb_004","session_id":"session_004","ran_addr":"192.168.1.103","ran_port":2152,"teid":12348}' \
  "http://127.0.0.23:7777/nsmf-pdusession/v1/gnb-sync" \
  --http2-prior-knowledge \
  -v \
  --max-time 10

echo ""
echo "等待 5 秒观察日志..."
sleep 5

# 显示最终日志
echo "=== 最终日志 ==="
echo "DMF:"
tail -10 /tmp/open5gs_test/logs/dmf.log
echo ""
echo "DSMF:"
tail -10 /tmp/open5gs_test/logs/dsmf.log

# 测试 5: 使用 Python 脚本测试 HTTP/2
echo ""
echo "=== 测试 5: 使用 Python 脚本测试 HTTP/2 ==="
cat > /tmp/test_http2.py << 'EOF'
#!/usr/bin/env python3
import socket
import struct

def send_http2_request(host, port, path, data):
    # HTTP/2 连接前言
    preface = b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n'
    
    # 创建 socket 连接
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    # 发送 HTTP/2 连接前言
    sock.send(preface)
    
    # 发送 SETTINGS 帧
    settings_frame = struct.pack('!BBBB', 0x00, 0x00, 0x00, 0x04)  # 4 bytes
    settings_frame += struct.pack('!HH', 0x0000, 0x0001)  # SETTINGS_MAX_CONCURRENT_STREAMS = 1
    sock.send(settings_frame)
    
    print(f"发送 HTTP/2 请求到 {host}:{port}{path}")
    print(f"数据: {data}")
    
    # 接收响应
    response = sock.recv(1024)
    print(f"响应: {response}")
    
    sock.close()

# 测试 DMF
send_http2_request('127.0.0.21', 7777, '/gnb-sync', 
                   '{"gnb_id":"test_gnb_py","action":"register","session_id":"session_py","ran_addr":"192.168.1.104","ran_port":2152,"teid":12349}')
EOF

python3 /tmp/test_http2.py

echo ""
echo "等待 3 秒..."
sleep 3

# 显示最终日志
echo "=== 最终日志 ==="
echo "DMF:"
tail -5 /tmp/open5gs_test/logs/dmf.log
echo ""
echo "DSMF:"
tail -5 /tmp/open5gs_test/logs/dsmf.log

# 清理
echo ""
echo "清理测试环境..."
kill $DF_PID $DMF_PID $DSMF_PID 2>/dev/null
sleep 2
rm -f /tmp/test_http2.py

echo "=== HTTP/2 测试完成 ===" 