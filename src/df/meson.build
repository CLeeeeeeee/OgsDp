# U - 自定义组件构建文件
# 此构建文件是用户添加的自定义组件 df 的构建配置
# 不是原始 Open5GS 构建系统的一部分
# 
# 文件: meson.build
# 组件: df
# 添加时间: 2025年 08月 20日 星期三 11:16:14 CST

# Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>

# This file is part of Open5GS.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

df_conf = configuration_data()

df_headers = ('''
    ifaddrs.h
    net/ethernet.h
    net/if.h
    net/if_dl.h
    netinet/in.h
    netinet/ip.h
    netinet/ip6.h
    netinet/ip_icmp.h
    netinet/icmp6.h
    sys/ioctl.h
    sys/socket.h
'''.split())

foreach h : df_headers
    if cc.has_header(h)
        define = 'HAVE_' + h.underscorify().to_upper()
        df_conf.set(define, 1)
    endif
endforeach

if have_func_kqueue
    df_conf.set('HAVE_KQUEUE', 1)
endif

configure_file(output : 'df-config.h', configuration : df_conf)

libdf_sources = files('''
    event.h
    context.h
    df-sm.h
    pfcp-path.h
    sbi-path.h
    ndf-build.h
    ndf-handler.h
    dn3-path.h
    timer.h

    init.c
    event.c
    context.c
    df-sm.c
    pfcp-sm.c
    pfcp-path.c
    sbi-path.c
    ndf-build.c
    ndf-handler.c
    dn3-path.c
    timer.c
'''.split())

libdf = static_library('df',
    sources : libdf_sources,
    dependencies : [
        libmetrics_dep,
        libpfcp_dep,
        libgtp_dep,
        libsbi_dep,
    ],
    install : false)

libdf_dep = declare_dependency(
    link_with : libdf,
    dependencies : [
        libmetrics_dep,
        libpfcp_dep,
        libgtp_dep,
        libsbi_dep,
    ])

df_sources = files('''
    app.c
    ../main.c
'''.split())

executable('open5gs-dfd',
    sources : df_sources,
    c_args : '-DDEFAULT_CONFIG_FILENAME="@0@/df.yaml"'.format(open5gs_sysconfdir),
    include_directories : srcinc,
    dependencies : libdf_dep,
    install_rpath : libdir,
    install : true) 