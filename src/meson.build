gui_sources = ['codeviewer.cpp',
               'dapclient.cpp',
               'debugger.cpp',
               'outputlog.cpp',
               'main.cpp',
               'mainwindow.cpp',
               'sceneviewer.cpp',
               'settingsdialog.cpp',
               'syntaxhighlighter.cpp',
               'variablesmodel.cpp',
               'xsystem4.cpp',
]

gui_moc = ['codeviewer.hpp',
           'dapclient.hpp',
           'debugger.hpp',
           'outputlog.hpp',
           'mainwindow.hpp',
           'sceneviewer.hpp',
           'settingsdialog.hpp',
           'syntaxhighlighter.hpp',
           'variablesmodel.hpp',
]

generated_files = qt5.preprocess(moc_headers : gui_moc,
                                 qresources : ['../icons.qrc'],
                                 dependencies : [qt5_dep])

executable('xsys4dbg', gui_sources, generated_files,
           dependencies : deps,
           include_directories : incdir,
           gui_app : true,
           install : true)
