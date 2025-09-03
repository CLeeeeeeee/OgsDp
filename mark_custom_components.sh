#!/bin/bash

# 为自定义组件添加 U 标注的脚本
# 这些组件是用户添加的，不是原始 Open5GS 的一部分

echo "开始为自定义组件添加 U 标注..."

# 定义自定义组件目录
CUSTOM_COMPONENTS=("df" "dmf" "dsmf" "dnf")

# 为每个组件目录添加 U 标注
for component in "${CUSTOM_COMPONENTS[@]}"; do
    if [ -d "src/$component" ]; then
        echo "处理组件: $component"
        
        # 在组件目录下创建 U 标注文件
        cat > "src/$component/U_CUSTOM_COMPONENT" << EOF
# U - 自定义组件标注
# 此组件 ($component) 是用户添加的自定义组件，不是原始 Open5GS 的一部分
# 添加时间: $(date)
# 组件功能: 用户自定义的 5G 核心网组件
# 
# 相关文件:
# - src/$component/ - 组件源代码目录
# - configs/open5gs/${component}.yaml.in - 组件配置文件模板
# - install/etc/open5gs/${component}.yaml - 组件安装配置文件
EOF
        
        # 为组件的主要源文件添加 U 标注注释
        find "src/$component" -name "*.c" -o -name "*.h" | while read file; do
            if [ -f "$file" ]; then
                # 检查文件是否已经有 U 标注
                if ! grep -q "U - 自定义组件" "$file"; then
                    # 在文件开头添加 U 标注注释
                    temp_file=$(mktemp)
                    cat > "$temp_file" << EOF
/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 $component 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: $(basename "$file")
 * 组件: $component
 * 添加时间: $(date)
 */

EOF
                    cat "$file" >> "$temp_file"
                    mv "$temp_file" "$file"
                    echo "  - 已为 $file 添加 U 标注"
                fi
            fi
        done
    else
        echo "警告: 组件目录 src/$component 不存在"
    fi
done

# 为配置文件添加 U 标注
for component in "${CUSTOM_COMPONENTS[@]}"; do
    # 配置文件模板
    if [ -f "configs/open5gs/${component}.yaml.in" ]; then
        if ! grep -q "U - 自定义组件配置" "configs/open5gs/${component}.yaml.in"; then
            temp_file=$(mktemp)
            cat > "$temp_file" << EOF
# U - 自定义组件配置文件
# 此配置文件是用户添加的自定义组件 $component 的配置模板
# 不是原始 Open5GS 配置的一部分
# 
# 文件: ${component}.yaml.in
# 组件: $component
# 添加时间: $(date)

EOF
            cat "configs/open5gs/${component}.yaml.in" >> "$temp_file"
            mv "$temp_file" "configs/open5gs/${component}.yaml.in"
            echo "  - 已为 configs/open5gs/${component}.yaml.in 添加 U 标注"
        fi
    fi
    
    # 安装配置文件
    if [ -f "install/etc/open5gs/${component}.yaml" ]; then
        if ! grep -q "U - 自定义组件配置" "install/etc/open5gs/${component}.yaml"; then
            temp_file=$(mktemp)
            cat > "$temp_file" << EOF
# U - 自定义组件配置文件
# 此配置文件是用户添加的自定义组件 $component 的安装配置
# 不是原始 Open5GS 配置的一部分
# 
# 文件: ${component}.yaml
# 组件: $component
# 添加时间: $(date)

EOF
            cat "install/etc/open5gs/${component}.yaml" >> "$temp_file"
            mv "$temp_file" "install/etc/open5gs/${component}.yaml"
            echo "  - 已为 install/etc/open5gs/${component}.yaml 添加 U 标注"
        fi
    fi
done

# 为 meson.build 文件添加 U 标注
for component in "${CUSTOM_COMPONENTS[@]}"; do
    if [ -f "src/$component/meson.build" ]; then
        if ! grep -q "U - 自定义组件构建" "src/$component/meson.build"; then
            temp_file=$(mktemp)
            cat > "$temp_file" << EOF
# U - 自定义组件构建文件
# 此构建文件是用户添加的自定义组件 $component 的构建配置
# 不是原始 Open5GS 构建系统的一部分
# 
# 文件: meson.build
# 组件: $component
# 添加时间: $(date)

EOF
            cat "src/$component/meson.build" >> "$temp_file"
            mv "$temp_file" "src/$component/meson.build"
            echo "  - 已为 src/$component/meson.build 添加 U 标注"
        fi
    fi
done

# 创建总体 U 标注文档
cat > "U_CUSTOM_COMPONENTS_README.md" << EOF
# U - 自定义组件文档

## 概述
本文档记录了用户添加的自定义 5G 核心网组件，这些组件不是原始 Open5GS 代码库的一部分。

## 自定义组件列表

### 1. DF (Data Forwarding) - 数据转发组件
- **目录**: `src/df/`
- **功能**: 负责用户面数据转发，通过 GTP-U 协议处理数据包
- **配置文件**: 
  - `configs/open5gs/df.yaml.in`
  - `install/etc/open5gs/df.yaml`
- **主要文件**:
  - `src/df/init.c` - 组件初始化
  - `src/df/context.c` - 上下文管理
  - `src/df/dn3-path.c` - DN3 接口处理
  - `src/df/ndf-handler.c` - NDF 消息处理

### 2. DMF (Data Management Function) - 数据管理功能
- **目录**: `src/dmf/`
- **功能**: 管理 gNB 注册和 RAN 地址信息，转发给 DSMF
- **配置文件**: 
  - `configs/open5gs/dmf.yaml.in`
  - `install/etc/open5gs/dmf.yaml`
- **主要文件**:
  - `src/dmf/init.c` - 组件初始化
  - `src/dmf/context.c` - 上下文管理
  - `src/dmf/dmf-sm.c` - 状态机
  - `src/dmf/sbi-path.c` - SBI 接口
  - `src/dmf/nnrf-handler.c` - NRF 处理

### 3. DSMF (Data Session Management Function) - 数据会话管理功能
- **目录**: `src/dsmf/`
- **功能**: 管理数据会话，处理 PFCP 协议，与 DF 建立会话
- **配置文件**: 
  - `configs/open5gs/dsmf.yaml.in`
  - `install/etc/open5gs/dsmf.yaml`
- **主要文件**:
  - `src/dsmf/init.c` - 组件初始化
  - `src/dsmf/context.c` - 上下文管理
  - `src/dsmf/dsmf-sm.c` - 状态机
  - `src/dsmf/sbi-path.c` - SBI 接口
  - `src/dsmf/dn4-build.c` - PFCP 消息构建
  - `src/dsmf/dn4-handler.c` - PFCP 消息处理

### 4. DNF (Data Network Function) - 数据网络功能
- **目录**: `src/dnf/`
- **功能**: 数据网络功能组件（待实现）
- **配置文件**: 待添加
- **主要文件**: 待实现

## 组件间关系
```
AMF → DMF → DSMF → DF
  ↓      ↓      ↓
NRF ←→ DMF ←→ DSMF
```

## 协议支持
- **SBI (Service-Based Interface)**: HTTP/2, 用于组件间通信
- **PFCP (Packet Forwarding Control Protocol)**: 用于 DSMF 和 DF 之间的会话管理
- **GTP-U**: 用于 DF 的用户面数据转发

## 配置端口
- **DF PFCP 客户端**: 8805
- **DSMF PFCP 服务器**: 8806
- **DF DN3 (GTP-U)**: 2153
- **DMF SBI 服务器**: 7777
- **DSMF SBI 服务器**: 7778

## 注意事项
1. 这些组件是实验性的，可能包含未完成的实现
2. 组件间的通信协议和接口可能随开发进展而变化
3. 建议在测试环境中使用，生产环境需要进一步验证

## 开发状态
- [x] DF: 基础实现完成，支持 GTP-U 数据转发
- [x] DMF: 基础实现完成，支持 gNB 注册和 RAN 信息管理
- [x] DSMF: 基础实现完成，支持 PFCP 会话管理
- [ ] DNF: 待实现

## 最后更新
$(date)
EOF

echo "U 标注完成！"
echo "已创建 U_CUSTOM_COMPONENTS_README.md 文档"
echo ""
echo "自定义组件已标注为 U："
for component in "${CUSTOM_COMPONENTS[@]}"; do
    if [ -d "src/$component" ]; then
        echo "  ✓ $component"
    else
        echo "  ✗ $component (目录不存在)"
    fi
done
