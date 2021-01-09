project('native-activity', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

cc = meson.get_compiler('c')
android_sdk_path = meson.get_external_property('android_sdk_path')
android_ndk_path = meson.get_external_property('android_ndk_path')
prog_aapt = find_program('aapt')
prog_dx = find_program('dx')
prog_zipalign = find_program('zipalign')
prog_apksigner = find_program('apksigner')
prog_javac = find_program('javac')

lib_android = cc.find_library('android')
lib_log = cc.find_library('log')
lib_egl = cc.find_library('EGL')
lib_glesv2 = cc.find_library('GLESv2')

path_android_jar = android_sdk_path / 'platforms/android-30/android.jar'
path_tmpdir = '@PRIVATE_DIR@'


lib_native_app_glue = static_library(
  'native_app_glue',
  android_ndk_path / 'sources/android/native_app_glue/android_native_app_glue.c'
)

lib_native_activity = shared_library(
  'native-activity',
  'src/main.c',
  dependencies: [ lib_glesv2, lib_egl, lib_android, lib_log ],
  link_with: lib_native_app_glue,
)

# the reason these are so messy: https://github.com/mesonbuild/meson/issues/2320
unsigned_apk = custom_target(
  'unsigned_apk',
  input: [ lib_native_activity, 'AndroidManifest.xml' ],
  output: 'NativeActivity.unsigned.apk',
  command: [ 'sh', '-c', '@0@ && @1@ && @2@ && @3@ && @4@'.format(
    'mkdir -p "@PRIVATE_DIR@"/bin "@PRIVATE_DIR@"/lib/arm64-v8a',
    '"@0@" package -f -m -S "@1@" -M "@2@" -I "@3@" -F "@PRIVATE_DIR@"/a.apk "@4@"/bin'.format(
      prog_aapt.full_path(),
      meson.project_source_root() / 'res',
      meson.project_source_root() / 'AndroidManifest.xml',
      path_android_jar,
      path_tmpdir
    ),
    'ln -sf "@0@" "@1@"/lib/arm64-v8a'.format(lib_native_activity.full_path(), path_tmpdir),
    'cd "@PRIVATE_DIR@" && "@0@" add a.apk lib/arm64-v8a/"@1@" && cd -'.format(prog_aapt.full_path(), lib_native_activity.full_path().split('/').get(-1)),
    '"@0@" -f 4 "@PRIVATE_DIR@"/a.apk "@OUTPUT@"'.format(prog_zipalign.full_path())
  ) ],
  build_by_default: true,
)

apk_signing_keystore = get_option('apk_signing_keystore')
if apk_signing_keystore != ''
  signed_apk = custom_target(
    'signed_apk',
    input: unsigned_apk,
    output: 'NativeActivity.apk',
    command: [
      prog_apksigner, 'sign',
      '--ks', apk_signing_keystore,
      '--out', '@OUTPUT@',
      '--ks-pass', 'pass:android',
      '--key-pass', 'pass:android',
      '@INPUT@' ],
    build_by_default: true,
  )
endif
