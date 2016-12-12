from conans import ConanFile, CMake

import os

class CMark(ConanFile):
    name = 'cmark'
    url = 'https://github.com/sztomi/cmark'
    settings = 'os', 'compiler', 'build_type', 'arch'
    license = 'BSD2'
    version = '0.27.1'
    exports = '*'
    generators = 'cmake'

    def build(self):
        cmake = CMake(self.settings)
        os.mkdir('build')
        os.chdir('build')
        self.run('cmake {} {} -DCMAKE_INSTALL_PREFIX={}'
                    .format(self.conanfile_directory,
                            cmake.command_line,
                            self.package_folder))
        self.run('cmake --build .')
        self.run('make install')

    def package(self):
        # build() also installs
        pass

    def package_info(self):
        self.cpp_info.libs = ['cmark']
