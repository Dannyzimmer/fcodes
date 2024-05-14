from setuptools import setup, find_packages

setup(
    name='fcode',
    version='0.0.1',
    packages=find_packages(),
    include_package_data=True,
    install_requires=[
        'Click',
        'unidecode',
        'typing_extensions'
    ],
    entry_points={
        'console_scripts': [
            'fcode = fcodes.fcode:cli',
        ],
    },
)
