gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
  else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled'
        )
  endif
endif

dump = declare_dependency(
         sources: [
        '../dump_serialize.cpp'
    ])

tests = [
    'debug_inif_test',
]

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     include_directories: ['.', '../'],
                     implicit_include_directories: false,
                     dependencies:[ gtest_dep,
                                    gmock_dep,
                                    dump,
                                    phosphor_logging_dep,
                                    fmt_dep,
                                    cppfs]),
       workdir: meson.current_source_dir())
endforeach
