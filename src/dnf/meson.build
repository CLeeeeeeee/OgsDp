dnf_sources = [
    'app.c',
    'init.c',
    'context.c',
    'ndf-handler.c',
    'storage.c',
]

incdir = include_directories('.', '..')

static_library('dnf', dnf_sources, 
    include_directories : incdir,
) 