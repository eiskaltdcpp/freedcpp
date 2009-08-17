# -*- coding: utf-8 -*-

import os
import commands
import string
import glob

EnsureSConsVersion(0, 96)

APP_NAME = 'freedcpp'
LIB_NAME = 'libdcpp'
BUILD_PATH = '#/build/'
BUILD_LOCALE_PATH = 'build/locale/'

# ----------------------------------------------------------------------
# Function definitions
# ----------------------------------------------------------------------

def CheckPKGConfig(context):
	context.Message('Checking for pkg-config... ')
	ret = context.TryAction('pkg-config --version')[0]
	context.Result(ret)
	return ret

def CheckPKG(context, name):
	context.Message('Checking for %s... ' % name)
	ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
	context.Result(ret)
	return ret

def CheckCXXVersion(context, name, major, minor):
	context.Message('Checking for %s >= %d.%d...' % (name, major, minor))
	ret = commands.getoutput('%s -dumpversion' % name)

	retval = 0
	try:
		if ((string.atoi(ret[0]) == major and string.atoi(ret[2]) >= minor)
		or (string.atoi(ret[0]) > major)):
			retval = 1
	except ValueError:
		print "No C++ compiler found!"

	context.Result(retval)
	return retval


# ----------------------------------------------------------------------
# Command-line options
# ----------------------------------------------------------------------

# Parameters are only sticky from scons -> scons install, otherwise they're cleared.
if 'install' in COMMAND_LINE_TARGETS:
	opts = Options('build/sconf/scache.conf')
else:
	opts = Options()

opts.AddOptions(
	BoolOption('debug', 'Compile the program with debug information', 0),
	BoolOption('release', 'Compile the program with optimizations', 0),
	BoolOption('profile', 'Compile the program with profiling information', 0),
	PathOption('PREFIX', 'Compile the program with PREFIX as the root for installation', '/usr/local'),
	('FAKE_ROOT', 'Make scons install the program under a fake root (for gentoo ebuilds)', '')
)


# ----------------------------------------------------------------------
# Initialization
# ----------------------------------------------------------------------

env = Environment(ENV = os.environ, options = opts)

conf = Configure(env,
	custom_tests =
	{
		'CheckPKGConfig' : CheckPKGConfig,
		'CheckPKG' : CheckPKG,
		'CheckCXXVersion' : CheckCXXVersion
	},
	conf_dir = 'build/sconf',
	log_file = 'build/sconf/config.log')

if os.environ.has_key('CXX'):
	conf.env['CXX'] = os.environ['CXX']

if os.environ.has_key('CC'):
	conf.env['CC'] = os.environ['CC']

if os.environ.has_key('CXXFLAGS'):
	conf.env['CPPFLAGS'] = conf.env['CXXFLAGS'] = os.environ['CXXFLAGS'].split()

if os.environ.has_key('LDFLAGS'):
	conf.env['LINKFLAGS'] = os.environ['LDFLAGS'].split()

if os.environ.has_key('CFLAGS'):
	conf.env['CFLAGS'] = os.environ['CFLAGS'].split()

env.SConsignFile('build/sconf/.sconsign')
opts.Save('build/sconf/scache.conf', env)
Help(opts.GenerateHelpText(env))

# ----------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------

if not 'install' in COMMAND_LINE_TARGETS:

	if not (conf.env.has_key('CXX') and conf.env['CXX']):
		print 'CXX env variable is not set, attempting to use g++'
		conf.env['CXX'] = 'g++'

	if not conf.CheckCXXVersion(env['CXX'], 4, 1):
		print 'Compiler version check failed. g++ 4.1 or later is needed'
		Exit(1)

	if not conf.CheckPKGConfig():
		print '\tpkg-config not found.'
		Exit(1)

	if not conf.CheckPKG('gtk+-2.0 >= 2.10'):
		print '\tgtk+ >= 2.10 not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckPKG('gthread-2.0 >= 2.4'):
		print '\tgthread >= 2.4 not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckPKG('libglade-2.0 >= 2.4'):
		print '\tlibglade-2.0 >= 2.4 not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)
	
	if not conf.CheckCXXHeader('boost/version.hpp', '<>'):
		print '\tboost not found.'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckHeader('time.h'):
		Exit(1)

	if not conf.CheckHeader('signal.h'):
		Exit(1)

	if not conf.CheckHeader('unistd.h'):
		Exit(1)

	if not conf.CheckLibWithHeader('pthread', 'pthread.h', 'c'):
		print '\tpthread library not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('z', 'zlib.h', 'c'):
		print '\tz library (gzip/z compression) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckLibWithHeader('bz2', 'bzlib.h', 'c'):
		print '\tbz2 library (bz2 compression) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	# This needs to be before ssl check on *BSD systems
	if not conf.CheckLib('crypto'):
		print '\tcrypto library not found'
		print '\tNote: This library may be a part of libssl on your system'
		Exit(1)

	if not conf.CheckLibWithHeader('ssl', 'openssl/ssl.h', 'c'):
		print '\tOpenSSL library (libssl) not found'
		print '\tNote: You might have the lib but not the headers'
		Exit(1)

	if not conf.CheckHeader('iconv.h'):
		Exit(1)
	elif conf.CheckLibWithHeader('iconv', 'iconv.h', 'c', 'iconv(0, (const char **)0, 0, (char**)0, 0);'):
		conf.env['ICONV_CONST'] = 'const'

	if conf.CheckHeader(['sys/types.h', 'sys/socket.h', 'ifaddrs.h', 'net/if.h']):
		conf.env['HAVE_IFADDRS_H'] = True

	env = conf.Finish()


# ----------------------------------------------------------------------
# Compile and link flags
# ----------------------------------------------------------------------

	# todo: remove -fpermissive and fix the errors
	env.Append(CXXFLAGS = ['-I.', '-D_GNU_SOURCE', '-D_LARGEFILE_SOURCE', '-D_FILE_OFFSET_BITS=64', '-D_REENTRANT', '-fpermissive'])

	if os.sys.platform == 'linux2':
		env.Append(LINKFLAGS = ['-Wl,--as-needed'])

	if os.name == 'mac' or os.sys.platform == 'darwin':
		env['ICONV_CONST'] = ''

	if env.has_key('ICONV_CONST') and env['ICONV_CONST']:
		env.Append(CXXFLAGS = '-DICONV_CONST=' + env['ICONV_CONST'])

	if env.has_key('HAVE_IFADDRS_H'):
		env.Append(CXXFLAGS = '-DHAVE_IFADDRS_H')

	if env.has_key('debug') and env['debug']:
		env.Append(CXXFLAGS = ['-g', '-ggdb', '-D_DEBUG', '-Wall'])
		env.Append(LINKFLAGS = ['-g', '-ggdb' '-Wall'])
		BUILD_PATH = BUILD_PATH + 'debug/'

	elif env.has_key('release') and env['release']:
		env.Append(CXXFLAGS = ['-O3', '-fomit-frame-pointer', '-DNDEBUG'])
		BUILD_PATH = BUILD_PATH + 'release/'

	if env.has_key('profile') and env['profile']:
		env.Append(CXXFLAGS = '-pg')
		env.Append(LINKFLAGS= '-pg')

	if env.has_key('PREFIX') and env['PREFIX']:
		env.Append(CXXFLAGS = '-D_DATADIR=\'\"' + env['PREFIX'] + '/share' + '\"\'')

	env.ParseConfig('pkg-config --libs libglade-2.0')
	env.ParseConfig('pkg-config --libs gthread-2.0')

	env.Append(LIBPATH = BUILD_PATH + LIB_NAME)
	env.Prepend(LIBS = LIB_NAME)

	mo_args = ['msgfmt', '-c', '-o', '$TARGET', '$SOURCE']
	mo_bld = Builder(action = Action([mo_args], 'Create $TARGET from $SOURCE'))
	env.Append(BUILDERS = {'MoBuild' : mo_bld})


# ----------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------

	Export('env')

	# @todo: Change build_dir to variant_dir when required SCons version is >=1.0.0
	# 	(variant_dir doesn't seem to work with 0.97 and everything is built in ./dcpp/ and ./linux/
	#	 -> libdcpp.a is not found when linking, as it is in ./dcpp/libdcpp.a instead of build/dcpp..)
	# Build the dcpp library
	libdcpp = SConscript(dirs = 'dcpp', build_dir = BUILD_PATH + LIB_NAME, duplicate = 0)

	# Build the GUI
	obj_files = SConscript(dirs = 'linux', build_dir = BUILD_PATH + APP_NAME, duplicate = 0)

	# Create the executable
	env.Program(target = APP_NAME, source = [libdcpp, obj_files])

# ----------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------

else:

	glade_files = glob.glob('glade/*.glade')
	pixmap_files = glob.glob('pixmaps/*.png')
	text_files = glob.glob('*.txt')

	mo_files = []
	locale = []

	# freedcpp gui
	path = BUILD_LOCALE_PATH + APP_NAME + '/'
	languages = os.listdir(path)

	for lang in languages:
		locale.append(env['FAKE_ROOT'] + env['PREFIX'] + '/share/locale/' + lang + '/LC_MESSAGES/' + APP_NAME + '.mo')
		mo_files.append(path + lang)

	# dcpp library
	path = BUILD_LOCALE_PATH + LIB_NAME + '/'
	languages = os.listdir(path)
	
	for lang in languages:
		locale.append(env['FAKE_ROOT'] + env['PREFIX'] + '/share/locale/' + lang + '/LC_MESSAGES/' + LIB_NAME + '.mo')
		mo_files.append(path + lang)

	# install glade files
	env.Alias('install', env.Install(dir = env['FAKE_ROOT'] + env['PREFIX'] + '/share/' + APP_NAME + '/glade', source = glade_files))

	# install pixmap files
	env.Alias('install', env.Install(dir = env['FAKE_ROOT'] + env['PREFIX'] + '/share/' + APP_NAME + '/pixmaps', source = pixmap_files))

	# install text files
	env.Alias('install', env.Install(dir = env['FAKE_ROOT'] + env['PREFIX'] + '/share/doc/' + APP_NAME, source = text_files))
	
	# install app file
	env.Alias('install', env.Install(dir = env['FAKE_ROOT'] + env['PREFIX'] + '/bin', source = APP_NAME))

	# install locale
	env.Alias('install', env.InstallAs(target = locale, source = mo_files))