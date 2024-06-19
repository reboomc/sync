# coding:utf-8

from setuptools import setup, Extension
from Cython.Build import cythonize


src_files = ['intc_cyw.pyx', 'src/intc_cli.c', 'src/intc_ll.c',
             'src/libev/anet.c', 'src/log.c', 'src/vector.c', 'src/dict.c',
             'src/intc_dict.c', 'src/siphash.c']

ext = [
    Extension(
        'intc',
        src_files,
        include_dirs=['./inc'],
        define_macros=[],
    ),
]

setup(
    name='intc_cy',
    author='micyng',
    ext_modules=cythonize(ext),
)
