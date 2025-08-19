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

# src/dmf/meson.build
libdmf_sources = files('''
    context.c
    event.c
    handler.c
    nnrf-handler.c
    init.c
    dmf-sm.c
    sbi-path.c
'''.split())

libdmf = static_library('dmf',
    sources : libdmf_sources,
    dependencies        : [libmetrics_dep,
                           libsctp_dep,
                           libnas_5gs_dep,
                           libsbi_dep],
    install : false)

libdmf_dep = declare_dependency(
    link_with : libdmf,
    dependencies : [libmetrics_dep,
                    libsctp_dep,
                    libnas_5gs_dep,
                    libsbi_dep])

dmf_sources = files('''
    app.c
    ../main.c
'''.split())

executable('open5gs-dmfd',
           sources : dmf_sources,
           c_args : '-DDEFAULT_CONFIG_FILENAME="@0@/dmf.yaml"'.format(open5gs_sysconfdir),
           include_directories : srcinc,
           dependencies : libdmf_dep,
           install_rpath : libdir,
           install : true)

