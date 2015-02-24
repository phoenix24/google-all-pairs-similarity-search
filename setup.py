from distutils.core import setup, Extension

extension_mod = Extension("allpairs", 
                          ["allpairs.cc", "data-source-iterator.cc", "allpairs_wrap.cxx"]
)

setup(name = "allpairs",
      version = "0.1.0",
      url = "https://github.com/phoenix24/google-all-pairs-similarity-search",
      ext_modules=[extension_mod],

      author="Roberto Bayardo",
      author_email="roberto.bayardo@gmail.com",

      maintainer = "Chaitanya Sharma",
      maintainer_email = "chaitanya@3bandar.org",

      classifiers = [
          "Classifier: Development Status :: 5 - Production/Stable",
          "Classifier: Environment :: Console",
          "Classifier: Intended Audience :: Developers",
          "Classifier: Programming Language :: Python",
          "Classifier: Programming Language :: Python :: 2",
          "Classifier: Topic :: Software Development :: Libraries"
      ],

      description = """
      This package provides a bare-bones implementation of the
      'All-Pairs-Binary' algorithm described in the following paper:

      R. J. Bayardo, Yiming Ma, Ramakrishnan Srikant. Scaling Up All-Pairs
      Similarity Search. In Proc. of the 16th Int'l Conf. on World Wide Web,
      131-140, 2007. (download from: http://www.bayardo.org/ps/www2007.pdf).
      """
)





