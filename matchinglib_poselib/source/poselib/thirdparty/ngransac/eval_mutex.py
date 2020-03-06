"""
Global lock variable and initialization
"""
from NamedAtomicLock import NamedAtomicLock
import warnings


def init_lock(name='ev_lock'):
    global lock
    lock = NamedAtomicLock(name)


def acquire_lock(timeout=5):
    global acquired
    if lock.acquire(timeout=timeout):
        acquired = True
    else:
        warnings.warn('Reading available GPU memory is blocked too long by other process.', UserWarning)
        acquired = False


def release_lock():
    if acquired:
        if not lock.release():
            warnings.warn('Unable to release Lock.', UserWarning)