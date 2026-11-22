project('dashboard-launcher', 'cpp',
  version: '0.1',
  license: 'MIT',
  default_options: ['cpp_std=c++17', 'warning_level=2'])

wpe = dependency('wpe-webkit-2.0')

executable('dashboard-launcher', 'main.cpp',
  dependencies: wpe,
  install: true)
