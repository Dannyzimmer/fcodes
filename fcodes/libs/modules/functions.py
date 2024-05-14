from fcodes.libs.classes.Fcode import FcodeManager

def get_upward_link(code: str):
    '''Return the code of the previous relative from the given code.'''
    for i in [-1, -2, -3]:
        try:
            if not code[i].isnumeric(): #if last character not a number
                return code[0:i]
        except IndexError:
            return '*'

def build_link_dic(file_path: str):
    '''
    Return a dictionary with codes as keys and names as values:
        Ej.: {'*PMP' : Some Name}
    file_path: path to the text file with the relatives codes.
    '''
    with open(file_path) as file:
        data = file.read().strip().split("\n")
    links_dic = {}
    for d in data:
        code = d.split('\t')[0]
        name = d.split('\t')[1]
        if code not in links_dic.keys():
            links_dic[code] = name
    return links_dic

def build_legend_dic(file_path = './libs/data/legend_ENG.txt'):
    with open(file_path) as file:
        data = file.read().strip().split("\n")
        legend_dic = {}
    for d in data:
        symbol = d.split('\t')[0]
        legend = d.split('\t')[1]
        if symbol not in legend_dic.keys():
            legend_dic[symbol] = legend
    return legend_dic

def clean_fcode(code: str) -> str:
    '''Removes numbers and symbols (except from "*") from an fcode.'''
    to_remove = [str(i) for i in range(10)] + ['-', ':', '.', '']
    result = ''.join([i for i in code if i not in to_remove])
    return result

def get_potential_siblings(code):
    self_code = code
    return [self_code + 'A', self_code + 'O', self_code + 'H']

def get_potential_offspring(code):
    search = [code + 'a', code + 'o', code + 'h']
    fcode = FcodeManager(code)
    # if self is P or M, then offspring = layer-1
    if fcode[-1][0] in ['P', 'M']:
        search.append(fcode[0:-1])
    return search

def get_potential_partners(code):
    ' Return a list of potential partners for the given fcode'
    if code.sex == 'M':
        partner = code[0:-1] + 'M'
    elif code.sex == 'F':
        partner = code[0:-1] + 'P'
    else:
        partner = 'NA'
    return [code.code + 'C', partner]

def booleanize_parents(code: str):
    '''Boleanize only P's and M's. E.g.: *MP1M2O3 --> *MPMO3'''
    fcode = FcodeManager(code)
    layers = fcode.layers
    for index, value in enumerate(layers):
            if value[0] in ['P', 'M']:
                layers[index] = fcode.get_f_bool(value)
    return ''.join(layers)

def print_all_full_names(fbook):
    result = [i.name for i in fbook.DATA.values()]
    result.sort()
    for i in result: print(i)

def get_all_names(fbook):
    result = [i.name for i in fbook.DATA.values()]
    result = [i.split(' ')[0] for i in result]
    result.sort()
    return result

def get_all_unique_names(fbook):
    names = list(set(get_all_names(fbook)))
    names.sort()
    return names

def print_all_fcodes(fbook):
    [print(i.code) for i in fbook.all_fcodes]

def convert_list_to_same_length(list1: list, list2: list) -> tuple:
    '''
    Adds None to the shorter list to equal the size of the longer. Returns a
    tuple with both lists (shorter moddified, longer)
    '''
    if len(list1) == len(list2):
        return (list1, list2)
    lengths = sorted([(len(list1), list1), (len(list2), list2)],
                     key = lambda i: i[0])
    shorter = lengths[0][1]; longer = lengths[1][1]
    difference = lengths[1][0] - lengths[0][0]
    for i in range(difference):
        shorter.append(None)
    return (shorter, longer)