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
classes_dex = custom_target(
  'classes_dex',
  input: [ 'AndroidManifest.xml' ],
  output: 'classes.dex',
  command: [ 'sh', '-c', '@0@ && @1@ && @2@ && @3@'.format(
    'mkdir -p "@0@"/src "@0@"/obj'.format(path_tmpdir),
    '"@0@" package -m -S "@1@" -J "@2@"/src -M "@3@" -I "@4@"'.format(
      prog_aapt.full_path(),
      meson.project_source_root() / 'res',
      path_tmpdir,
      meson.project_source_root() / 'AndroidManifest.xml',
      path_android_jar
    ),
    '"@0@" -source 1.7 -target 1.7 -d "@2@"/obj -classpath "@1@" -sourcepath "@2@"/src "@2@"/src/com/example/native_activity/R.java'.format(prog_javac.full_path(), path_android_jar, path_tmpdir),
    '"@0@" --dex --output=classes.dex "@1@"/obj'.format(prog_dx.full_path(), path_tmpdir)
  ) ],
)
unsigned_apk = custom_target(
  'unsigned_apk',
  input: [ classes_dex, lib_native_activity ],
  output: 'NativeActivity.unsigned.apk',
  command: [ 'sh', '-c', '@0@ && @1@ && @2@ && @3@ && @4@'.format(
    'mkdir -p "@0@"/bin "@0@"/lib/arm64-v8a && ln -sf "@1@" "@0@"/bin'.format(path_tmpdir, classes_dex.full_path()),
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
