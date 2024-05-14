from typing import List, Tuple, TypedDict, Callable
from typing_extensions import Unpack, NotRequired

# FILTERS
def is_type(type: list) -> Callable:
    'Return lambda function that return True if fcode.type == type'
    return lambda i: i.type in type

def is_generation(generation: int) -> Callable:
    'Return lambda function that return True if fcode.generation == generation'
    return lambda i: i.generation == generation

def generation_between(gen_range: Tuple[int, int]) -> Callable:
    '''
    Return lambda function that return True if fcode.generation in between
    gen_range
    '''
    start = gen_range[0]; stop = gen_range[1]
    return lambda i: i.generation in [i for i in range(start, stop)]

def start_with(start_with: str) -> Callable:
    '''
    Return lambda function that return True if the fcode starts with
    "starts_with", else False
    '''
    return lambda i: i[0:len(start_with)] == start_with

def key(fun: Callable) -> Callable:
    'Return the user input'
    return fun

filters = {
    'type_is' : is_type,
    'start_with' : start_with,
    'is_generation': is_generation,
    'generation_between': generation_between,
    'key': key
}

class Filters(TypedDict):
    type_is: NotRequired[List[str]]
    start_with: NotRequired[str]
    is_generation: NotRequired[int]
    generation_between: NotRequired[Tuple[int, int]]
    key: NotRequired[Callable]

def build_filters(filters = filters, **kwargs: Unpack[Filters]) -> list:
    chosen_filters = []
    for key, value in kwargs.items():
        chosen_lambda = filters[key]
        chosen_filters.append(chosen_lambda(value))
    return chosen_filters