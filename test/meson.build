upnptestfiles = files(
    'gmock-gtest-all.cpp',
    'main.cpp',

    'devicediscover.h',
    'testenvironment.h',

    'xmlutilstest.cpp',
    'upnputilstest.cpp',
    #'mediarenderertest.cpp',
    #'mediaservertest.cpp',
    #'controlpointtest.cpp',
    'httpreadertest.cpp',
    'webservertest.cpp',
    'upnpavtransporttest.cpp',
    'upnpcontentdirectorytest.cpp',
    'upnprenderingcontroltest.cpp',
    'upnpservicebasetest.cpp'
)

testinc = include_directories(meson.current_build_dir() + '/..')

upnptest = executable('upnptest',
                      upnptestfiles,
                      include_directories : [utilsinc, upnpinc, testinc],
                      link_with : [upnplib, utilslib],
                      dependencies : libupnp_dep)

test('upnp test', upnptest)