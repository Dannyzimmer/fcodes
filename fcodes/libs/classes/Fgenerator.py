from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.data.keys import symbols, types, legend, forbidden_structure
from fcodes.libs.data.random import names as nms
import math
from typing import List
import random
import re

class Fgenerator:
    def __init__(self, fnumber_mu = 1.2, fnumber_sigma = 1.5) -> None:
        self.offspring_ave = fnumber_mu
        self.offspring_var = fnumber_sigma
        self.down_expansion_ratio = 0.62  # Chances of creating a family of an offpring
        self.expansion_ratio_wide = 0.50
        self.time_fading = 0.1 # Decrease of vertical expansion ratio each new generation
        pass

    def get_random_bool(self)-> bool:
        'Return True or False randomly'
        return bool(random.getrandbits(1))
    
    def get_ponderated_bool(self, true_ratio: float)-> bool:
        """Return True or False. Uses true_ratio to adjust the chances of
        returning True (true_ratio between 0-1).
        """
        options = [True, False]
        false_ratio = 1-true_ratio
        result = random.choices(
            population = options, weights = [true_ratio, false_ratio], k = 1)
        return result[0]
    
    def get_random_sex(self)-> str:
        """Return a random sex (M/F)"""
        coin = self.get_random_bool()
        return 'M' if coin else 'F'
    
    def get_random_letter(self)-> str:
        'Return a random Fcode letter from libs.data.keys.symbols'
        return random.choice(symbols)
    
    def get_random_fnumber(self)-> str:
        'Return a valid fnumber using a normal distribution'
        result = 0
        while int(result) <= 0:
            result = random.gauss(
                mu = self.offspring_ave,
                sigma = self.offspring_var
            )
        return str(int(result))

    def get_random_numerated_layer(self)-> str:
        'Return a random Fcode layer numerated, e.g.: O2'
        letter = self.get_random_letter()
        number = self.get_random_fnumber()
        layer = '{}{}'.format(letter, number)
        return layer
    
    def join_layer_parts(self, letter: str, number: int)-> str:
        'Return the letter and the number of a layer joined'
        return letter + str(number)
    
    def get_random_sexed_type_from_type(self, type: str)-> str:
        '''
        Return a random sexed type fron the given type.
        INPUT:
            get_random_sexed_type_from_type('H')
        OUTPUT:
            A
        '''
        result = random.choice(types[type])
        return result

    def make_layers_in_range(self, sexed_type, stop, start = 1)-> list:
        '''Return layers of a given sexed type with the numbers in a range. If
        sexed_type is a type, then return layers with random sexes.
            INPUT:
                make_layers_in_range(4, 'H', start = 0)
            OUTPUT:
                [A1, O2, O3, A4]
        '''
        stop += 1
        layers_range = range(start, stop)
        result = []
        if sexed_type in types.keys() and sexed_type != '*':
            for n in layers_range:
                layer = self.get_random_sexed_type_from_type(sexed_type)+str(n)
                result.append(layer)
        else:
            for i in layers_range:
                result.append(sexed_type + str(i))
        return result
    
    def get_random_fcode(self, depth = None) -> str: # type: ignore
        '''Return a random fcode of the desired depth (default random
        between 1-6)
        '''
        loop = True
        while loop:
            fcode = ['*']
            fcode_depth = random.randint(1, 6) if depth == None else depth
            for i in range(fcode_depth):
                layer = self.get_random_numerated_layer()
                if i < fcode_depth-1 and layer[0] in ['P','M','X','C']:
                    fcode.append(layer[0])
                else:
                    fcode.append(layer)
            result = ''.join(fcode)
            structure = FcodeManager(result)
            if structure not in forbidden_structure:
                return result
            
    
    def get_fcode_with_pattern(self, pattern, iterations = 500)-> str: #FIXME
        '''Generates an fcode with the given pattern, if reach iterations without
        a code with the pattern return 'NA'
        '''
        for i in range(iterations):
            fcode = self.get_random_fcode()
            structure = FcodeManager(fcode).structure
            is_pattern = re.search('.*{}.*'.format(pattern), structure)
            if is_pattern != None:
                return fcode
        return 'NA'

    def make_random_father(self, fcode = '*')-> str:
        '''Generate random number for the father of the given fcode (* default).
        Return the generated fcode.
        '''
        layer_number = self.get_random_fnumber()
        layer = legend['father'] #+ layer_number
        return fcode + layer
    
    def make_random_mother(self, fcode = '*')-> str:
        '''Generate random number for the mother of the given fcode (* default).
        Return the generated fcode.
        '''
        layer_number = self.get_random_fnumber()
        layer = legend['mother'] #+ layer_number
        return fcode + layer
    
    def make_random_partner(self, fcode = '*')-> str:
        '''Generate random number for the partner of the given fcode (* default).
        Return the generated fcode.
        '''
        layer_number = self.get_random_fnumber()
        layer = legend['partner'] #+ layer_number
        return fcode + layer

    def make_random_siblings(self, fcode = '*')-> list:
        '''Generate random sexed types and numbers for the siblings of the given
        fcode (* default). Return a list with the fcodes.
        '''
        sib_number = int(self.get_random_fnumber())
        layers = self.make_layers_in_range('H', sib_number)
        result = [fcode + i for i in layers]
        return result
    
    def make_random_offspring(self, fcode = '*')-> list:
        '''Generate random sexed types and numbers for the offspring of the given
        fcode (* default). Return a list with the fcodes.
        '''
        off_number = int(self.get_random_fnumber())
        layers = self.make_layers_in_range('h', off_number)
        result = [fcode + i for i in layers]
        return result
    
    def make_random_family_down(self, seed = '*')-> dict:
        '''Generate a random family from the given seed (* default). The family
        is made downwards, by generating a partner and offspring of that seed.
        Return a dictionary with the following structure:
            output = {
                'parents'   : [fcode_father, fcode_mother],
                'offspring' : [fcode_offspring_1, fcode_offspring_2 ...]
            }
        '''
        partner = self.make_random_partner(seed)
        offspring = self.make_random_offspring(seed)
        result = {
            'parents'   : [seed, partner],
            'offspring' : offspring
        }
        return result

    def make_random_family_up(self, seed = '*')-> dict: # FIXME: make this function
        '''Generate a random family from the given seed (* default). The family
        is made upwards, by generating brothers and parents of that seed. The 
        chances of generating a partner are self.offspring_ave, if the partner
        is not generated then, ommits its fcode. Return a dictionary with the
        following structure:
            output = {
                'parents'   : [fcode_father, fcode_mother],
                'siblings' : [fcode_brother_1, fcode_brother_2 ...]
                'seed' : [seed]
            }
        '''
        father = self.make_random_father(seed)
        mother = self.make_random_mother(seed)
        siblings = self.make_random_siblings(seed)
        result = {
            'parents'   : [father, mother],
            'siblings' : siblings,
            'seed' : [seed]}
        
        if self.get_ponderated_bool(self.expansion_ratio_wide):
            partner = self.make_random_partner(seed)
            result['seed'] = result['seed'] + [partner]
        return result

    def unpack_family_dict(self, family_dict: dict)-> List[str]:
        'Return the values of family dict as a list of strings'
        result = []
        for v in family_dict.values():
            if type(v) == list:
                for p in v:
                    if type(p) == str:
                        result.append(p)
                    else:
                        raise TypeError
            elif type(v) == str:
                result.append(v)
            else:
                raise TypeError
        return result

    def calculate_expansion(self, fcodes: list)-> list:
        '''Return randomly the fcodes to expand from a list. The number of the
        sample is randomly chosen using a normal distribution with mu = number
        of kids per woman (self.get_random_fnumber()).
        '''
        f_len = len(fcodes)
        sample_size = int(self.get_random_fnumber())

        while sample_size > f_len:
            sample_size = int(self.get_random_fnumber())

        expand =  random.sample(fcodes, sample_size)
        return expand
    
    def calculate_upward_fading(self, fcodes: list, generation: int)-> list:
        """Return randomly sampled fcodes by applying less probability
        of return a sample as the generation number increases.
        """
        if generation == 0:
            return fcodes
        p_extend = 1/(generation**self.time_fading)
        weights = [1-p_extend, p_extend/2, p_extend/2]
        options = [0, 1, 2]
        sample_size = random.choices(options, weights=weights)[0]
        if sample_size == 0:
            return []
        elif sample_size == 2:
            return fcodes
        return [random.choice(fcodes)]

    def make_random_tree_up(self, 
            seed = '*',  max_size = 15, f_tree = [], generation = 0,
            person_pile = [])-> list:
        '''Generate a random family tree from the given seed (* default). The
        family tree is made upwards, by generating brothers, parents and 
        partners of the seed, and adding layers up recursively by using the 
        parents. No horizontal expansion.

        The argument "max_size" is used to limit number of members of the tree
        (default 15).
        '''
        if len(f_tree) >= max_size:
            return list(set(f_tree))

        # Build a seed
        seed_family = self.make_random_family_up(seed)
        
        # Unpack the seed to the tree
        unpacked_seed = self.unpack_family_dict(seed_family)
        for p in unpacked_seed:
            if len(f_tree) <= max_size:
                f_tree.append(p)
            else:
                return list(set(f_tree))

        # Choose if none, any or both parents are expanded
        expansion = self.calculate_upward_fading(
                seed_family['parents'], generation = generation)
        for i in expansion:
            person_pile.insert(0, i)

        # Apply recursion to the person pile
        for p in person_pile:
            family = self.make_random_tree_up(
                    seed = p, max_size = max_size, f_tree = f_tree,
                    generation = generation + 1, person_pile = person_pile)
            # Fill the tree while possible or exit
            for person in family:
                if len(f_tree) >= max_size:
                    return f_tree
                if person not in f_tree:
                    f_tree.append(person)
            person_pile.pop(0)

        if person_pile == []:
            if len(f_tree) >= max_size:
                return list(set(f_tree))
        
        return list(set(f_tree))

    def make_random_tree_down(self, 
            seed = '*',  max_size = 15, f_tree = [], person_pile = [])-> list:
        '''Generate a random family tree from the given seed (* default). The
        family tree is made downwards, by generating a partner and offspring of
        that fcode, and adding layers recursively.

        For a given generation, a random number of infividuals are chosen to
        expand their families (self.calculate_expansion()).

        The argument "max_size" is used to limit number of members of the tree
        (default 15).
        '''
        seed_family = self.make_random_family_down(seed)
        expand =  self.calculate_expansion(seed_family['offspring'])

        if len(f_tree) >= max_size:
            return f_tree

        for lst in seed_family.values():
            for person in lst:
                if person not in f_tree:
                    f_tree.append(person)
            
        for person in expand:
            person_pile.insert(0, person)

        # Expand the kindships on person_pile
        for seed_person in person_pile:
            continue_down = self.get_ponderated_bool(self.down_expansion_ratio)
            if continue_down:
                family = self.make_random_tree_down(
                    seed = seed_person, max_size = max_size, f_tree = f_tree,
                    person_pile = person_pile)
                for person in family:
                    if len(f_tree) >= max_size:
                        return f_tree
                    if person not in f_tree:
                        f_tree.append(person)
            person_pile.pop(0)
        
        if person_pile == []:
            if len(f_tree) >= max_size:
                return f_tree
        
        return f_tree
    
    def remove_forbiden_structures(self, fcodes: list)-> list:
        result = []
        for i in fcodes:
            structure = FcodeManager(i).structure
            if structure not in forbidden_structure:
                result.append(i)
        return result

    def make_random_tree_vertical(
            self, seed = '*',  max_size = 16, f_tree = [])-> list:
        '''Generate a random family tree from the given seed (* default). The
        family tree is made, first upwards, then downwards.

        The argument "max_size" is used to limit number of members of the tree;
        default 16.
        '''
        up_size = random.randint(1, max_size - 1)
        down_size = max_size - up_size
        seed = '*'
        f_tree_mix = []
        f_tree_up = self.make_random_tree_up(seed = seed, max_size = up_size)
        random_seed = random.choice(f_tree_up)
        f_tree_down = self.make_random_tree_down(seed = random_seed, max_size = down_size)
        for i in f_tree_up + f_tree_down:
            if i not in f_tree_mix:
                f_tree_mix.append(i)
        return self.remove_forbiden_structures(f_tree_mix)

    def get_random_first_name(self, sex = None)-> str:
        """Return a random name, the sex of the name can be specified (M/F)"""
        if sex == None:
            sex =  self.get_random_sex()
        name = random.choice(nms.first_name_dic[sex])
        return name
    
    def get_random_surname(self)-> str:
        """Return a random surname"""
        surname = random.choice(nms.surnames)
        return surname
    
    def get_random_tree_data(self, max_size:int, method = 'u')-> list:
        """Return a random tree in TSV format compatible with the FamilyTree
        module.
            max_size: the maximum size of the tree.
            method (u/d/m): the method for constructing the tree. Acepted values
                are "u" (up), "d" (down) or "v" (vertical).
        """
        # Choose the tree building method
        if method == 'u':
            tree = self.make_random_tree_up(max_size = max_size)
        elif method == 'd':
            tree = self.make_random_tree_down(max_size = max_size)
        elif method == 'v':
            tree = self.make_random_tree_vertical(max_size = max_size)
        else:
            if type(method) == str:
                raise ValueError
            raise TypeError
        
        data = []
        for person in tree: # type: ignore
            sex = FcodeManager(person).sex
            p_name = self.get_random_first_name(sex)
            p_surname = self.get_random_surname()
            data.append('{}\t{} {}'.format(person, p_name, p_surname))
        return data
    
    def save_fcodes_as_data(self, 
            tree_data, filepath: str)-> None:
        """Save the given fcodes as a data file."""
        with open(filepath, 'w') as fp:
            for line in tree_data:
                fp.write("%s\n" % line)

    def save_random_tree_data(
            self, filepath: str, max_size: int, method = 'u')-> None:
        """Generate and save to a file random data compatible with FBook and
        FamilyTree classes. The maximum size of the data can be adjusted with
        the argument "max_size" (15 default). Method is the algorith for 
        constructing the tree. Acepted values are "u" (up), "d" (down) or 
        "m" (mix).
        """
        data = self.get_random_tree_data(max_size = max_size, method = method)
        self.save_fcodes_as_data(data, filepath)