# U - 自定义组件构建文件
# 此构建文件是用户添加的自定义组件 dnf 的构建配置
# 不是原始 Open5GS 构建系统的一部分
# 
# 文件: meson.build
# 组件: dnf
# 添加时间: 2025年 08月 20日 星期三 11:16:14 CST

libdnf_sources = files('''
    context.c
    event.c
    init.c
    sbi-path.c
    gtpu-path.c
    dnf-sm.c
    timer.c
'''.split())

libdnf = static_library('dnf',
    sources : libdnf_sources,
    dependencies : [libmetrics_dep,
                    libsbi_dep,
                    libgtp_dep],
    install : false)

libdnf_dep = declare_dependency(
    link_with : libdnf,
    dependencies : [libmetrics_dep,
                    libsbi_dep,
                    libgtp_dep])

dnf_sources = files('''
    app.c
    ../main.c
'''.split())

executable('open5gs-dnfd',
    sources : dnf_sources,
    c_args : '-DDEFAULT_CONFIG_FILENAME="@0@/dnf.yaml"'.format(open5gs_sysconfdir),
    include_directories : srcinc,
    dependencies : libdnf_dep,
    install_rpath : libdir,
    install : true)