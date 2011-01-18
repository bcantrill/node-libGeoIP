srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.env.append_unique('LINKFLAGS',["-L/opt/local/lib/"]);
  conf.env.append_unique('CXXFLAGS',["-I/opt/local/include/"]);
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.check_cxx(lib='GeoIP', mandatory=True, uselib_store='GeoIP')

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'libGeoIP'
  obj.uselib = 'GeoIP'
  obj.source = 'libGeoIP.cc'
