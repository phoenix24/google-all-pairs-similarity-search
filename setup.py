#!/usr/bin/env python

#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#	http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

import sys
try:
    from setuptools import setup, Extension
except:
    from distutils.core import setup, Extension, Command

from distutils.command.build_ext import build_ext
from distutils.errors import CCompilerError, DistutilsExecError, DistutilsPlatformError


if sys.platform == 'win32':
    include_dirs.append('compat/win32')
    ext_errors = (CCompilerError, DistutilsExecError, DistutilsPlatformError, IOError)
else:
    ext_errors = (CCompilerError, DistutilsExecError, DistutilsPlatformError)

class BuildFailed(Exception):
    pass

class ve_build_ext(build_ext):
    def run(self):
            build_ext.run(self)

    def build_extension(self, ext):
            build_ext.build_extension(self, ext)

def run_setup(with_binary):
    extensions = dict (
        ext_modules = [
            Extension('_allpairs', 
                      sources = ["src/allpairs.i", 
                                 "src/allpairs.cc", 
                                 "src/data-source-iterator.cc"], 
                      swig_opts = ["-modern", "-c++"])
        ],
        py_modules = ["allpairs"],
        cmdclass=dict(build_ext=ve_build_ext)
    )

    setup(name = "allpairs",
          version = "0.1.0",
          description = "Python bindings for the Google All Pairs Similarity Search",
          url = "https://github.com/phoenix24/google-all-pairs-similarity-search",
          license = 'Apache License 2.0',
          author="Roberto Bayardo",
          author_email="roberto.bayardo@gmail.com",
          maintainer = "Chaitanya Sharma",
          maintainer_email = "chaitanya@3bandar.org",

          packages = [
              "allpairs"
          ],
          package_dir = {
              "allpairs" : "src"
          },
          classifiers = [
              "Classifier: Development Status :: 5 - Production/Stable",
              "Classifier: Environment :: Console",
              "Classifier: Intended Audience :: Developers",
              "Classifier: Programming Language :: Python",
              "Classifier: Programming Language :: Python :: 2",
              "Classifier: Topic :: Software Development :: Libraries"
          ],
          **extensions
      )

run_setup(True)
