project('cross-build-tools', 'c')
fs = import('fs')

system = host_machine.system()

cc = meson.get_compiler('c')

subproject_default_options = [ 'default_library=static' ]

if system == 'linux-android'
  # lib_log = cc.find_library('log')
  meson.override_dependency('glesv2', cc.find_library('GLESv2'))
  meson.override_dependency('egl', cc.find_library('EGL'))
endif

main_subproject = get_option('main_subproject')
main_subproject_s = subproject(main_subproject, default_options: subproject_default_options)
main_dep = main_subproject_s.get_variable(main_subproject + '_dep')

if system == 'linux-android'
  android_sdk_path = meson.get_external_property('android_sdk_path')
  android_ndk_path = meson.get_external_property('android_ndk_path')
  prog_aapt = find_program('aapt')
  prog_dx = find_program('dx')
  prog_zipalign = find_program('zipalign')
  prog_apksigner = find_program('apksigner')
  prog_javac = find_program('javac')

  lib_android = cc.find_library('android')

  path_android_jar = android_sdk_path / 'platforms/android-30/android.jar'

  lib_native_app_glue = static_library('native_app_glue', android_ndk_path
    / 'sources/android/native_app_glue/android_native_app_glue.c')

  lib_native_activity = shared_library(
    'native-activity',
    dependencies: [ main_dep, lib_android ],
    link_with: lib_native_app_glue,
    # see: https://github.com/android/ndk/issues/381
    link_args: [ '-u', 'ANativeActivity_onCreate' ]
  )

  # the reason these are so messy:
  # https://github.com/mesonbuild/meson/issues/2320
  unsigned_apk = custom_target(
    'unsigned_apk',
    input: [ lib_native_activity, 'assets/android/AndroidManifest.xml' ],
    output: 'NativeActivity.unsigned.apk',
    command: [ 'sh', '-c', '@0@ && @1@ && @2@ && @3@ && @4@'.format(
      'mkdir -p "@PRIVATE_DIR@"/bin "@PRIVATE_DIR@"/lib/arm64-v8a',
      ('"@0@" package -f -m -S "@1@" -M "@2@" -I "@3@"' +
	  ' -F "@PRIVATE_DIR@"/a.apk "@PRIVATE_DIR@"/bin').format(
	prog_aapt.full_path(),
	meson.project_source_root() / 'assets/android/res',
	meson.project_source_root() / 'assets/android/AndroidManifest.xml',
	path_android_jar
      ),
      'ln -sf "@0@" "@PRIVATE_DIR@"/lib/arm64-v8a'
	.format(lib_native_activity.full_path()),
      'cd "@PRIVATE_DIR@" && "@0@" add a.apk lib/arm64-v8a/"@1@" && cd -'
	  .format(
	prog_aapt.full_path(),
	lib_native_activity.full_path().split('/').get(-1)
      ),
      '"@0@" -f 4 "@PRIVATE_DIR@"/a.apk "@OUTPUT@"'
	.format(prog_zipalign.full_path())
    ) ],
    build_by_default: true,
  )

  apk_signing_keystore = get_option('apk_signing_keystore')
  if apk_signing_keystore == ''
    apk_signing_keystore = fs.expanduser('~/.android/debug.keystore')
    if fs.exists(apk_signing_keystore)
      message('Using @0@ for signing the APK.'.format(apk_signing_keystore))
    else
      apk_signing_keystore = ''
      warning('No signing key specified, can\'t sign APK')
    endif
  endif
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
  subdir_done()
endif

executable('executable', dependencies: main_dep)
