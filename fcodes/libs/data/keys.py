symbols = ['P', 'M', 'O', 'A', 'o', 'a', 'C']

sexes = {
    'male': 'M',
    'female': 'F',
    'unknown': 'U'
}

legend = {
    'seed': '*',
    'partner': 'C',
    'brother': 'O',
    'sister': 'A',
    'sibling': 'H',
    'father': 'P',
    'mother': 'M',
    'son': 'o',
    'daughter': 'a',
    'parents': 'X'
    }

linage_key = {
    '*':'*',
    'P':'X',
    'M':'X',
    'O':'H',
    'A':'H',
    'o':'h',
    'a':'h',
    'C':'C',
    'H':'H',
    'h':'h'
}

structure = {
    '*':'=',
    'P':'+',
    'M':'+',
    'O':'=',
    'A':'=',
    'o':'-',
    'a':'-',
    'C':'x',
    'X':'+',
    'H':'=',
    'h':'-'
}

forbidden_structure = [
    '=+-',
    '-+',
    'x+-',
    'x-+',
    '=+=',
    '=-=',
    '-=',
    '-x-',
    'x+x',
    'x-=',
    '=+x',
    '=+',
    '===',
    'xx',
    '=x-'
]

forbidden_lineage_patterns = [
    'XhX',
    '*hX',
    'XHX',
]

switch_layer_sex = {
    '*':'*',
    'P':'M',
    'M':'P',
    'O':'A',
    'A':'O',
    'o':'a',
    'a':'o',
    'C':'C',
    'H':'H',
    'h':'h'
}

switch_sex_letter = {
    'U':'U',
    'F':'M',
    'M':'F',
}

sex = {
    '*':'U',
    'P':'M',
    'M':'F',
    'O':'M',
    'A':'F',
    'o':'M',
    'a':'F',
    'C':'U',
    'H':'U',
    'h':'U'
}

# type: [male, female]
types = {
    'X':['P','M'],
    'H':['O','A'],
    'h':['o','a'],
    'C':['C'],
}

directions = {
    'horizontal':['H', 'C', 'h'],
    'up':['X'],
    'down':['h']
}