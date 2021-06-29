import ctypes

from zenutils import rel2abs, os_name

if os_name == 'win32':
    try:
        import win32api
    except ImportError as e:
        raise ImportError('Please run: python -m pip install pywin32') from e
    win32api.SetDllDirectory(rel2abs(__file__, 'lib'))
    ctypes.cdll.LoadLibrary('zeno.dll')
else:
    ctypes.cdll.LoadLibrary(rel2abs(__file__, 'lib', 'libzeno.so'))

from . import pyzeno as core

__all__ = ['core']
