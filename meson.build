project(
    'assrender',
    ['c', 'cpp'],
    default_options: [
        'default_library=static',
        'buildtype=release',
        'b_vscrt=static_from_buildtype',
        'c_std=c11',
        'cpp_std=c++17',
        'warning_level=3',
    ],
)

# Mirror the MinGW-specific behavior from CMake
cc = meson.get_compiler('c')
is_mingw = host_machine.system() == 'windows' and cc.get_id() == 'gcc'

if is_mingw
    add_project_arguments(
        '-Wpedantic',
        language: 'c',
    )
    
    add_project_link_arguments(
        '-static-libgcc',
        '-Wl,--add-stdcall-alias',
        language: 'c',
    )
    
    if get_option('buildtype') in ['release', 'minsize']
        add_project_link_arguments(
            '-s',
            language: 'c',
        )
    endif
endif

subdir('src')
