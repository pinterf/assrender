cc = meson.get_compiler('c')

cmake_mod = import('cmake')
win = import('windows')

plugin_name = 'assrender'

sources = files(
    'assrender.c',
    'render.c',
    'sub.c',
    'timecodes.c',
)

if host_machine.system() == 'windows'
    sources += win.compile_resources('ASSRender.rc')
endif

inc_public = include_directories('include')

libass_dep = dependency(
    'libass',
    required: true,
    version: '>=0.17.4',
)

avisynth_dep = dependency(
    'avisynth',
    required: false,
    version: '>=3.7.3',
)

if not avisynth_dep.found()
    opt = cmake_mod.subproject_options()
    opt.add_cmake_defines({'HEADERS_ONLY': true})

    avisynth_sp = cmake_mod.subproject('avisynthplus', options: opt)
    avisynth_dep = avisynth_sp.dependency('AviSynth-Headers')
endif

deps = [avisynth_dep, libass_dep]
is_mingw = (host_machine.system() == 'windows' and cc.get_id() == 'gcc')
need_defs = host_machine.system() == 'windows' and (not is_mingw) and host_machine.cpu_family() == 'x86'

if need_defs
    assrender = shared_library(
        plugin_name,
        sources,
        include_directories: [inc_public],
        dependencies: deps,
        vs_module_defs: files('assrender.def'),
        install: true,
        install_dir: join_paths(get_option('libdir'), 'avisynth'),
    )
else
    assrender = shared_library(
        plugin_name,
        sources,
        include_directories: [inc_public],
        dependencies: deps,
        install: true,
        install_dir: join_paths(get_option('libdir'), 'avisynth'),
    )
endif
