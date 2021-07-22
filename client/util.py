import math
from datetime import datetime, timedelta
from time import sleep
from typing import Iterable, TypeVar, List, Callable, Generic, Tuple
from itertools import zip_longest

from dataclasses import dataclass, field
from threading import Thread, Lock, Condition, Timer

# Declare type variables
T = TypeVar('T')
U = TypeVar('U')
V = TypeVar('V')


def grouper(n, iterable, fillvalue=None):
    "grouper(3, 'ABCDEFG', 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return zip_longest(fillvalue=fillvalue, *args)


def flatten(iterables: Iterable[Iterable[T]]) -> List[T]:
    return [y for x in iterables for y in x]


def flatmap(fun: Callable[[U], Iterable[T]], iterables: Iterable[U]) -> List[T]:
    return flatten(map(fun, iterables))


class RepeatTimer(Timer):
    def run(self):
        while not self.finished.wait(self.interval):
            self.function(*self.args, **self.kwargs)


class RegularClock:
    """
    Helper class for regularizing delays.
    Call elapse to make sure exactly the timedelta
    elapsed since the last mark.
    """

    def __init__(self, context: str = None):
        self.last_mark = None
        self.context = context

    def mark(self) -> datetime:
        """
        Marks the current time as start for the next
        elapse call.

        Returns: The current datetime.
        """
        self.last_mark = datetime.now()
        return self.last_mark

    def elapse(self, time: timedelta) -> datetime:
        """
        Elapses remaining time since the last mark.
        If there is no mark, elapse the complete time.
        Automatically calls mark after completion.

        Args:
            time: The time to elapse.

        Returns: The current datetime.
        """
        seconds_elapsed = 0

        if self.last_mark:
            last_write_delta = (datetime.now() - self.last_mark)

            if last_write_delta > time and self.context:
                print("Can't keep up! ({0})".format(self.context))

            seconds_elapsed = last_write_delta / timedelta(seconds=1)

        sleep(max(0.0, time / timedelta(seconds=1) - seconds_elapsed))
        return self.mark()
