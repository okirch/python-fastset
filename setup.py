from setuptools import setup, Extension, find_packages

native_module = Extension(
		name='fastset',
		sources = [
			"src/bitvec.c",
			"src/domain.c",
			"src/extension.c",
			"src/member.c",
			"src/set.c",
			"src/transform.c",
		],
		extra_compile_args = ["-Wall", "-D_GNU_SOURCE", "-mavx2"],
	      )
kwargs = {
      'name' : 'src',
      'version' : '0.9',
      'ext_modules' :  [native_module],
      'license' : "GPL-2.0-or-later",
      'packages': find_packages(where='src'),
      'package_dir':{"": "src"},
      'package_data' : { 'fastset': ['src/*.h', 'src/*.so',
				   ]},
}

setup(**kwargs)
