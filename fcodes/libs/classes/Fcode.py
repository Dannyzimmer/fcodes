from fcodes.libs.data.keys import linage_key, structure, switch_layer_sex, forbidden_structure
from fcodes.libs.classes import Exceptions
from fcodes.libs.data.language import consanguinity_key
class FcodeManager:
    '''
    Class used to store a code and iterate trough its different layers of 
    relatives. The code should start with '*'.
    The layers of the code is the number of relatives before reaching the
    origin of coordinates (*), starting from the last relative.

    EXAMPLES:
        code = Fcode('*PO1o1a1')

        Iterate trough the relatives of the code:
            [print(i) for i in code]

        Get the number of layers of the code
            print(code.nlayers)
    '''
    def __init__(self, code: str, force = False) -> None:
        self.error_if_invalid_fcode(code, force)
        self.code = code
        self.f_bool = self.get_f_bool(self.code)
        self.boolcode = self.get_boolcode()
        self.f_linage = self.get_f_linage(self.code)
        self.linagecode = self.get_linagecode(self.code)
        self.depth = self.get_depth()
        self.generation = self.get_generation()
        self.layers = self.split_layers()
        self.sex = self.get_sex()
        self.type = self.get_type()
        self.sexed_type = self.get_sexed_type()
        self.number = self.get_number()
        self.structure = self.get_structure()
    
    def error_if_invalid_fcode(self, code:str, force = False) -> None:
        '''
        Raise an error if the passed fcode is not valid. If force = True
        bypasses the function.
        '''
        if force == True:
            return
        else:
            if not self.is_valid_fcode(code): 
                if type(code) == str: 
                    raise Exceptions.FcodeFormatError(code)
                else: 
                    raise TypeError 

    def is_valid_fcode(self, fcode: str) -> bool: #FIXME: add more tests
        'Check if the given fcode is valid, True if valid, False otherwise.'
        test_1 = True if fcode[0] == '*' else False
        return all([test_1])

    def detect_invalid_structure(self, fcode = None) -> str:
        'Return the forbiden structure of the code, else return "NA"'
        fc = self.code if fcode == None else fcode
        structure = self.get_structure(fc)
        for pattern in forbidden_structure:
            step = len(pattern)
            for i, j in enumerate(structure):
                try:
                    test_patt = structure[i:i+step]
                    if test_patt in forbidden_structure:
                        return pattern
                except:
                    pass
        return "NA"

    def is_valid_structure(self, fcode = None) -> bool:
        'Check if the structure of an fcode is valid'
        invalid_structure = self.detect_invalid_structure(fcode)
        return False if invalid_structure != 'NA' else True
        
    def get_structure(self, fcode = None):
        'Return the structure of an fcode'
        fc = self.code if fcode == None else fcode
        lineage = self.get_f_linage(fc)
        result = []
        for i in lineage:
            result.append(structure[i])
        return ''.join(result)

    def get_depth(self, code = None):
        '''Get the number of layers of the code.'''
        layers = self.code if code == None else code
        layers = [i for i in layers if not i.isnumeric()]
        return len(layers)
    
    def get_generation(self) -> int:
        'Get the number of generations between the OC and the self.code'
        generations = 0
        for l in self.f_linage:
            if l == 'X':
                generations += 1
            elif l == 'h':
                generations -= 1
        return generations

    def get_number(self, code = None) -> int:
        'Return the number of brother/sister of the fcode'
        fcode = self.code if code == None else code
        last_layer = self.get_layer_current(fcode)
        off_number = []
        for i in last_layer:
            if i.isnumeric():
                off_number.append(i)
        if len(off_number) == 0:
            return 0
        return int(''.join(off_number))

    def get_f_bool(self, fcode: str) -> str:
        '''Removes numbers and symbols (except from "*") from an fcode.'''
        to_remove = [str(i) for i in range(10)] + ['-', ':', '.', '']
        f_bool = ''.join([i for i in fcode if i not in to_remove])
        return f_bool

    def get_boolcode(self, fcode = None) -> str:
        '''Removes numbers and symbols (except from "*") from 
        the last layer of an fcode.'''
        fcode = self.code if fcode == None else fcode
        layers = self.split_layers(fcode)
        layers[-1] = self.get_f_bool(layers[-1])
        return ''.join([i for i in layers])

    def get_f_linage(self, fcode = None) -> str:
        '''Input a fcode an output a f_linage'''
        code = self.code if fcode == None else fcode
        code = self.get_f_bool(code)
        f_linage = []
        for l in code:
            f_linage.append(linage_key[l])
        return ''.join(f_linage)
    
    def get_linagecode(self, fcode = None) -> str:
        '''Input a fcode an output a linagecode'''
        code = self.code if fcode == None else fcode
        layers = self.split_layers(code)
        layers[-1] = self.get_f_linage(layers[-1])
        return ''.join([i for i in layers])

    def get_sexed_linage(self, fcode = None) -> str:
        code = self.code if fcode == None else fcode
        fbool = self.get_f_bool(code)
        flinage = self.get_f_linage(code)
        return ''.join(flinage[0:-1] + fbool[-1])
    
    def get_sexed_type(self, fcode = None) -> str:
        code = self.code if fcode == None else fcode
        code = self.get_layer_current()
        return self.get_f_bool(code)

    def get_type(self, fcode = None) -> str:
        code = self.code if fcode == None else fcode
        return self.get_f_linage(code)[-1]

    def split_layers(self, code = None) -> list:
        '''Return a list with relatives layers as items.'''
        last = self.code if code == None else code; layers = []
        depth = self.get_depth(last)
        for i in range(depth):
            layers.insert(0, self.get_layer_current(last))
            curr_layer = self.get_fcode_down(last)
            last = curr_layer
        return [i for i in layers if i != '']

    def get_fcode_down(self, code: str) -> str: #type: ignore
        '''Return the code of the previous relative from the given code.'''
        for i in [-1, -2, -3]:
            try:
                test1 = not code[i].isnumeric()  # last char not a number
                test2 = not code[i] in ['-',':','.'] # last char not - or :
                if test1 and test2: 
                    return code[0:i]
            except IndexError:
                return '*'
    
    def get_layer_current(self, fcode = None) -> str:
        '''Return the code of the current layer (e.g.: *MMO3 --> O3)'''
        code = self.code if fcode == None else fcode
        current_layer = self.get_fcode_down(code)
        current_layer = code[len(current_layer):]
        return current_layer
    
    def get_sex(self):
        '''Return "M" if male, "F" if female or "U" if unknown.'''
        males = ['P', 'O', 'o']
        females = ['M', 'A', 'a']
        unknown = ['H', 'h', 'C', 'X', '*', '']
        curr_layer = self.layers[-1]
        curr_layer = self.get_f_bool(curr_layer)
        if curr_layer in males:
            return 'M'
        elif curr_layer in females:
            return 'F'
        elif curr_layer in unknown:
            return 'U'
        else:
            raise SyntaxError
    
    def switch_sex(self, fcode = None):
        'Return an fcode with the opposite sexed type: *MP --> *MM'
        new_input = False if fcode == None else True 
        code = fcode if new_input else self.code
        layers = self.split_layers(code) if new_input else self.layers
        switched_layer = list(layers[-1])
        for index, value in enumerate(switched_layer):
            if not value.isnumeric():
                switched_layer[index] = switch_layer_sex[value]
        return ''.join(layers[0:-1] + switched_layer)

    def get_parbool(self, fcode = None):
        '''Boleanize only P's and M's. E.g.: *MP1M2O3 --> *MPMO3'''
        new_input = False if fcode == None else True 
        code = fcode if new_input else self.code
        layers = self.split_layers(code) if new_input else self.layers
        for index, value in enumerate(layers):
                if value[0] in ['P', 'M']:
                    layers[index] = self.get_f_bool(value)
        return ''.join(layers)

    def get_unsexed_consanguinity_name(self, fcode = None) -> str:
        code = self.code if fcode == None else fcode
        try:
            linage = self.get_f_linage(code)
            return consanguinity_key[linage]
        except:
            return 'Consanguinity not found'
    
    def get_consanguinity_name(self, fcode = None) -> str:
        code = self.code if fcode == None else fcode
        try:
            sxlinage = self.get_sexed_linage(code)
            return consanguinity_key[sxlinage]
        except:
            return 'Consanguinity not found'

    def has_any_P(self, fcode = None) -> bool:
        '''Return True if the fcode have any P's, else False.'''
        code = self.code if fcode == None else fcode
        for i in code:
            if i == 'P':
                return True
        return False
    
    def has_any_M(self, fcode = None) -> bool:
        '''Return True if the fcode have any M's, else False.'''
        code = self.code if fcode == None else fcode
        for i in code:
            if i == 'M':
                return True
        return False

    def has_any_parent(self, fcode = None) -> bool:
        '''Return True if the fcode have any M's or P's, else False.'''
        code = self.code if fcode == None else fcode
        test = [self.has_any_M(code), self.has_any_P(code)]
        return True if any(test) else False

    def __iter__(self):
       self.links = self.code
       return self

    def __next__(self):
       if self.links not in ['*', 'STOP']:
            x = self.links
            self.links = self.get_fcode_down(self.links)
            return x
       elif self.links == '*':
           x = self.links
           self.links = 'STOP'
           return x
       else:
           raise StopIteration
       
    def __getitem__(self, index):
        return ''.join(self.layers[index])
    
    def __repr__(self):
        return '''code: {}
nlayers: {}
layers: {}
sex: {}
        '''.format(self.code, self.depth, self.layers, self.sex)

    def __str__(self):
        return self.code