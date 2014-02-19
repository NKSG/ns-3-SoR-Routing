# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('sor-routing', ['internet', 'wifi', 'mesh', 'applications', 'network'])
    module.source = [
        'model/sor-ntable.cc',
        'model/sor-packet.cc',
        'model/sor-packet-queue.cc',
        'model/sor-routing.cc',
        'model/sor-rtable.cc',
        'helper/sor-routing-helper.cc',
        ]

#    module_test = bld.create_ns3_module_test_library('sor-routing')
#    module_test.source = [
#        'test/sor-routing-test-suite.cc',
#        ]

    headers = bld(features='ns3header')
    headers.module = 'sor-routing'
    headers.source = [
        'model/sor-ntable.h',
        'model/sor-packet.h',
        'model/sor-packet-queue.h',
        'model/sor-routing.h',
        'model/sor-rtable.h',
        'helper/sor-routing-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

