"""
Global lock variable and initialization
"""
from NamedAtomicLock import NamedAtomicLock
import warnings


class Locking:

    def __init__(self, name):
        self.lock = NamedAtomicLock(name)

    acquired = False
    lock = None

    def acquire_lock(self, timeout=5):
        if self.lock.acquire(timeout=timeout):
            self.acquired = True
        else:
            warnings.warn('Reading available GPU memory is blocked too long by other process.', UserWarning)
            self.acquired = False

    def release_lock(self):
        if self.acquired:
            if not self.lock.release():
                warnings.warn('Unable to release Lock.', UserWarning)