project('ffmpeg-wrap', 'c', 'cpp')
add_global_arguments('-fno-rtti', language : 'cpp')
#add_global_arguments('-Wall', language : 'cpp')
add_global_arguments('-pthread', language : 'cpp')
#add_global_arguments('-Wno-variadic-macros', language : 'c')

inc = include_directories(
  '/home/savage/git/macro-logger',
  '/home/savage/git/spdlog/include'
)

src = [
'ffmpeg_format.c',
'ffmpeg_log.c',
'ffmpeg_transcoding.c',
'gang_decoder_impl.c',

'gang_audio_device.cc',
'gang_decoder.cc',
'gang_init_deps.cc',
'gang_spdlog_console.cc',
'gangvideocapturer.cc'
]

jsoncpp = dependency('jsoncpp')
avcodec = dependency('libavcodec')
avformat = dependency('libavformat')
avfilter = dependency('libavfilter')
crypto = dependency('libcrypto')
openssl = dependency('openssl')
webrtc_lib = find_library('webrtc_full', dirs: '/home/savage/soft/webrtc/webrtc-linux64/lib/Debug')
webrtc_inc = include_directories('/home/savage/git/webrtcbuilds')
webrtc = declare_dependency(include_directories: webrtc_inc,
                            dependencies: webrtc_lib)

ffwraplib = static_library('ffmpeg-wrap',
                        sources: src,
                        include_directories : inc,
                        dependencies: [avcodec, avformat, avfilter, webrtc, jsoncpp, crypto, openssl],
                        install: true,
                        cpp_args: ['-DLOG_LEVEL=1',
                                   '-DGANG_AV_LOG=0',
                                   '-DENABLE_THREAD_CHECKER=1',
                                   '-DENABLE_DEBUG=0',
                                   '-DSPDLOG_TRACE_OFF',
                                   '-DWEBRTC_POSIX',
                                   '-DSPDLOG_NO_DATETIME',
                                   '-D_GLIBCXX_USE_CXX11_ABI=0'])

h = install_headers([
'ffmpeg_format.h',
'ffmpeg_log.h',
'ffmpeg_transcoding.h',
'gang_audio_device.h',
'gang_dec.h',
'gang_decoder.h',
'gang_decoder_impl.h',
'gang_init_deps.h',
'gang_spdlog_console.h',
'gangvideocapturer.h'
])

pkgconfig_gen(libraries : ffwraplib,
              subdirs: ['.'],
              version : '1.0',
              name : 'libffmpeg-wrap',
              filebase : 'ffmpeg-wrap',
              description : 'ffmpeg wrap tool')

ffwrap_inc = include_directories(
  '/home/savage/git/webrtcbuilds',
  '/home/savage/git/spdlog/include'
)

ffwrap_links = [
'-L/home/savage/soft/webrtc/webrtc-linux64/lib/Debug', '-lwebrtc_full',
'-pthread', '-lrt', '-ldl'
]

executable('gang_rec_only_test_main',
           'test/gang_rec_only_test_main.cc',
           link_with: ffwraplib,
           link_args: ffwrap_links,
           include_directories: ffwrap_inc,
           cpp_args: ['-DLOG_LEVEL=1',
                      '-DGANG_AV_LOG=0',
                      '-DENABLE_THREAD_CHECKER=1',
                      '-DENABLE_DEBUG=0',
                      '-DSPDLOG_TRACE_OFF',
                      '-DWEBRTC_POSIX',
                      '-DSPDLOG_NO_DATETIME',
                      '-D_GLIBCXX_USE_CXX11_ABI=0'])
