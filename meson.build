project('upnp', 'cpp', default_options : ['cpp_std=c++14'])

if not meson.is_subproject()
    utilssub = subproject('utils')
endif

utilsinc = utilssub.get_variable('utilsinc')
utilslib = utilssub.get_variable('utilslib')

upnpinc = include_directories('inc')

upnpfiles = files(
    'inc/upnp/upnpaction.h',                       'src/upnpaction.cpp',
    'inc/upnp/upnpactionresponse.h',               'src/upnpactionresponse.cpp',
    'inc/upnp/upnpavtransportclient.h',            'src/upnpavtransportclient.cpp',
    'inc/upnp/upnpavtransportservice.h',           'src/upnpavtransportservice.cpp',
    'inc/upnp/upnpavtransporttypes.h',
    'inc/upnp/upnpclientinterface.h',
    'inc/upnp/upnpconnectionmanagerclient.h',      'src/upnpconnectionmanagerclient.cpp',
    'inc/upnp/upnpconnectionmanagerservice.h',     'src/upnpconnectionmanagerservice.cpp',
    'inc/upnp/upnpconnectionmanagertypes.h',
    'inc/upnp/upnpcontentdirectoryclient.h',       'src/upnpcontentdirectoryclient.cpp',
    'inc/upnp/upnpcontentdirectoryservice.h',      'src/upnpcontentdirectoryservice.cpp',
    'inc/upnp/upnpcontentdirectorytypes.h',
    'inc/upnp/upnpcontrolpoint.h',                 'src/upnpcontrolpoint.cpp',
    'inc/upnp/upnpdevice.h',
    'inc/upnp/upnpdeviceservice.h',
    'inc/upnp/upnpdeviceserviceexceptions.h',
    'inc/upnp/upnpdevicescanner.h',                'src/upnpdevicescanner.cpp',
    'inc/upnp/upnpdlnainfo.h',                     'src/upnpdlnainfo.cpp',
    'inc/upnp/upnpfactory.h',                      'src/upnpfactory.cpp',
    'inc/upnp/upnpfwd.h',
    'inc/upnp/upnphttpclient.h',                   'src/upnphttpclient.cpp',
    'inc/upnp/upnphttpreader.h',                   'src/upnphttpreader.cpp',
    'inc/upnp/upnpitem.h',                         'src/upnpitem.cpp',
    'inc/upnp/upnplastchangevariable.h',           'src/upnplastchangevariable.cpp',
    'inc/upnp/upnpmediarenderer.h',                'src/upnpmediarenderer.cpp',
    'inc/upnp/upnpmediaserver.h',                  'src/upnpmediaserver.cpp',
    'inc/upnp/upnpprotocolinfo.h',                 'src/upnpprotocolinfo.cpp',
    'inc/upnp/upnprenderingcontrolclient.h',       'src/upnprenderingcontrolclient.cpp',
    'inc/upnp/upnprenderingcontrolservice.h',      'src/upnprenderingcontrolservice.cpp',
    'inc/upnp/upnprenderingcontroltypes.h',
    'inc/upnp/upnprootdevice.h',                   'src/upnprootdevice.cpp',
    'inc/upnp/upnprootdeviceinterface.h',
    'inc/upnp/upnpserviceclientbase.h',
    'inc/upnp/upnpservicevariable.h',
    'inc/upnp/upnpstatevariable.h',
    'inc/upnp/upnptypes.h',
    'inc/upnp/upnputils.h',
    'inc/upnp/upnpwebserver.h',                    'src/upnpwebserver.cpp',
    'inc/upnp/upnpxml.h',                          'src/upnpxml.cpp',
    'inc/upnp/upnpxmlutils.h',                     'src/upnpxmlutils.cpp',
    'src/upnpclient.h',                            'src/upnpclient.cpp'
)

libupnp_dep = dependency('libupnp')

config = configuration_data()

upnplib = static_library('upnp',
                          upnpfiles,
                          include_directories : [utilsinc, upnpinc],
                          dependencies : libupnp_dep)

subdir('test')
