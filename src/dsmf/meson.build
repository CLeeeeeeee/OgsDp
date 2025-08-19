# src/dsmf/meson.build

libdsmf_sources = files('''
    context.h
    event.h
    timer.h
    dsmf-sm.h
    pfcp-path.h
    dn4-build.h
    dn4-handler.h
    sbi-path.h

    context.c
    event.c
    timer.c
    pfcp-path.c
    dn4-build.c
    dn4-handler.c
    pfcp-sm.c
    dsmf-sm.c
    sbi-path.c
    init.c
'''.split())

libdsmf = static_library('dsmf',
    sources : libdsmf_sources,
    dependencies : [libmetrics_dep,
                    libsbi_dep,
                    libpfcp_dep],
    install : false)

libdsmf_dep = declare_dependency(
    link_with : libdsmf,
    dependencies : [libmetrics_dep,
                    libsbi_dep,
                    libpfcp_dep])

dsmf_sources = files('''
    app.c
    ../main.c
'''.split())

executable('open5gs-dsmfd',
    sources : dsmf_sources,
    c_args : '-DDEFAULT_CONFIG_FILENAME="@0@/dsmf.yaml"'.format(open5gs_sysconfdir),
    include_directories : srcinc,
    dependencies : libdsmf_dep,
    install_rpath : libdir,
    install : true)
