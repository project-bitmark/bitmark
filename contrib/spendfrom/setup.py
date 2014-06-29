from distutils.core import setup
setup(name='btmspendfrom',
      version='1.0',
      description='Command-line utility for bitmark "coin control"',
      author='Gavin Andresen',
      author_email='gavin@bitmarkfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
