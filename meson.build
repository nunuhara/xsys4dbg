project('xsys4-dbg', 'cpp')

qt5 = import('qt5')
qt5_dep = dependency('qt5', modules : ['Core', 'Gui', 'Widgets'])

libsys4_proj = subproject('libsys4')
libsys4_dep = libsys4_proj.get_variable('libsys4_dep')

if meson.get_compiler('c').has_function('iconv')
    deps = [qt5_dep, libsys4_dep]
else
    iconv = dependency('iconv')
    deps = [iconv, qt5_dep, libsys4_dep]
endif

incdir = include_directories('include')

subdir('src')
