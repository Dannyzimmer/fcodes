from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.classes.FEntry import FEntry, FEntry_parents_booled
from fcodes.libs.modules import functions as f
from fcodes.libs.data.keys import switch_sex_letter
from typing import List
import unidecode

class FBook:
    '''
    Manages the DATA of fcodes and names. The DATA is a tsv file with
    two columns, the first one has the fcodes, the second one the names of the
    relatives.
        DATA_path: path to the DATA file.
    '''
    def __init__(self, DATA_path) -> None:
        self.DATA_path = DATA_path
        self.DATA = self.build_DATA()
        self.DATA_fbool = self.build_DATA(booleanize=True)
        self.DATA_booleanized_parents = self.build_DATA_parents_booleanized()
        self.max_depth = self.get_max_depth()
        self.max_distance = self.get_max_generation()
        self.all_fcodes = [FcodeManager(i) for i in self.DATA.keys()]

    def build_DATA(self, booleanize:bool=False) -> dict:
        '''
        Return a dictionary with codes as keys and names as values:
            Ej.: {'*PMP' : FEntry(name, fcode_obj)
        file_path: path to the text file with the relatives codes.
        '''
        with open(self.DATA_path) as file:
            data = file.read().strip().split("\n")
        registry = {}
        for d in data:
            if len(d) > 0: 
                if d[0] != '#':
                    if booleanize == False:
                        code = d.split('\t')[0]
                    elif booleanize == True:
                        code = f.clean_fcode(d.split('\t')[0])
                    name = d.split('\t')[1]
                    if code not in registry.keys():
                        registry[code] = FEntry(name.strip(), FcodeManager(code))
        return registry
    
    def build_DATA_parents_booleanized(self) -> dict:
        with open(self.DATA_path) as file:
            data = file.read().strip().split("\n")
        registry = {}
        for d in data:
            if len(d) > 0: 
                if d[0] != '#':
                    original_code = d.split('\t')[0]
                    code = f.booleanize_parents(d.split('\t')[0])
                    name = d.split('\t')[1]
                    if code not in registry.keys():
                        registry[code] = FEntry_parents_booled(name=name.strip(), 
                                                            fcode=FcodeManager(code),
                                                            original_code=original_code)
        return registry

    def is_valid_fcode(self, fcode: str) -> bool: #FIXME: add more tests
        'Check if the given fcode is valid, True if valid, False otherwise.'
        test_1 = True if fcode[0] == '*' else False
        return all([test_1])

    def has_partner(self, fcode: str)-> bool:
        partner = self.get_partner_code(fcode)
        if partner != 'NA':
            return True
        else:
            return False

    def search_fcode(self, code: str) -> str:
        '''Search a fcode and return its name.'''
        if self.is_valid_fcode(code) == False:
            return 'NA'
        if code in self.DATA.keys():
            return self.DATA[code].name
        if code in self.DATA_booleanized_parents.keys():
            return self.DATA_booleanized_parents[code].name
        fcode = self.get_fcode(code)
        if fcode.type in ['P', 'M']:
            result = self.search_boolcode(code)
            if result != 'NA': return result 
        try:
            parbool = fcode.get_parbool()
            if parbool in self.DATA.keys():
                result = self.DATA[parbool].name
                if result != 'NA': return result
        except:
            return 'NA'
        return 'NA'
    
    def get_boolcode(self, fcode: str) -> str:
        '''
        Transform an fcode in a boolcode.
        E.g.: *MO2a2 --> *MO2a
        '''
        return self.get_fcode(fcode).get_boolcode()
    
    def normalize_string(self, string: str)-> str:
        """Transform the string to lowercase, remove accents and
        replaces spaces to a middle dash. E.g.:
            INPUT:  "Helló World"
            OUTPUT: "hello-world"
        """
        lower = string.lower().replace(' ', '-')
        return unidecode.unidecode(lower)

    def search_name(self, name: str) -> str:
        """Searches a normalized name and returns the fcode of the first match.
        If no matches returns "NA".
        """
        norm_name = self.normalize_string(name)
        for key, entry in self.DATA.items():
            norm_value = self.normalize_string(entry.name)
            if norm_value.find(norm_name) != -1:
                return key
        return 'NA'
    
    def search_names(self, name: str) -> list:
        """Searches a normalized name and returns all the matching fcodes.
        If no matches returns an empty list.
        """
        norm_name = self.normalize_string(name)
        result = []
        for key, entry in self.DATA.items():
            norm_value = self.normalize_string(entry.name)
            if norm_value.find(norm_name) != -1:
                result.append(key)
        if result != []:
            return result
        else:
            return []
    
    def get_parbool(self, fcode: str) -> str:
        '''
        Transform an fcode in a parbool.
        E.g.: *MO2a2 --> *MO2a
        '''
        return self.get_fcode(fcode).get_parbool()

    def return_first_no_NA(self, list: List[str]) -> str:
        'Return the first value of the list different from NA, else return NA'
        for i in list:
            if i != 'NA':
                return i
        return 'NA'
    
    def remove_NA_from_list(self, list: list) -> list:
        if len(list) > 0:
            return [i for i in list if i != 'NA']
        else:
            return []

    def search_fcode_from_boolcode(self, fcode: str) -> str:
        '''
        Transform the fcode in a boolcode and returns the fcode matched or NA.
        Returns the first match.
        '''
        boolcode = self.get_boolcode(fcode)
        library = [[self.get_boolcode(i[0]), i[1]] for i in self.DATA.items()]
        for codes in library:
            if boolcode == codes[0]:
                return codes[1].fcode.code
        return 'NA'
    
    def search_boolcode(self, fcode: str) -> str:
        'Return the name of the fcode by performing a boolcode search.'
        result = self.search_fcode_from_boolcode(fcode)
        return self.search_fcode(result)

    def fcode_search(self, fcode: str) -> str:
        'Search an fcode in DATA, if match return the fcode, else return NA.'
        for i in [i for i in self.DATA.keys()]:
            if i == fcode:
                return fcode
        return 'NA'
    
    def parbool_search(self, code: str) -> str:
        '''
        First booleanize the parents in an fcode, then perform a search in
        the DATA with the keys parbooled and finally return the matched fcode.
        or NA. E.g.: 
            INPUT:  *P2M1a1
            OUTPUT: *PM1a1  (in the example, the P has not been numerated).
        '''
        parbool = self.get_parbool(code)
        library = [[self.get_parbool(i[0]), i[1]] for i in self.DATA.items()]
        for codes in library:
            if parbool == codes[0]:
                return codes[1].fcode.code
        return 'NA'

    def fbool_search(self, code: str) -> str:
        '''First booleanize a code, then search it in the cleaned DATA, 
        finally returns its name.'''
        code = f.clean_fcode(code)
        if code in self.DATA_fbool.keys():
            return self.DATA_fbool[code].name
        else:
            return 'NA'

    def search_fcodes(self, fcodes: List[str]) -> list:
        'Search several fcodes and return their names.'
        result = []
        for code in fcodes:
            result.append(self.search_fcode(code))
        return result
    
    def fcode_multiple_search(self, fcodes: list) -> list:
        'Return input fcodes present in the registy.'
        result = []
        for code in fcodes:
            if code in [i for i in self.DATA.keys()]:
                result.append(code)
        return result

    def deep_search(self, fcode: str) -> str:
        'Perform a fcode_search and parbool_search, returns the first match.'
        query = [self.parbool_search(fcode), self.fcode_search(fcode)]
        return self.return_first_no_NA(query)
    
    def deep_multiple_search(self, fcode: List[str]) -> List[str]:
        '''
        Perform a fcode_search and parbool_search for each item in fcodes,
        then returns the first match.
        '''
        result = []
        for f in fcode:
            result.append(self.deep_search(f))
        return self.remove_NA_from_list(result)

    def match_boolcode_return_fcode(self, fcode: str) -> str:
        '''
        Takes a fcode, turns it into a boolcode and then returns the
        first match in DATA of that boolcode. Return 'NA' if no match. Eg.:
            INPUT : *PA
            OUTPUT: *PA1
        Useful for finding numerated parents (e.g.: P1, M3).
        '''
        code = FcodeManager(fcode).get_boolcode()
        #code_cleaned = fun.clean_fcode(code)
        for f in self.all_fcodes:
            if code == f.boolcode:
                return f.code
        return 'NA'

    def match_boolcodes_return_fcodes(self, fcodes: List[str]) -> list:
        '''
        Takes a list of fcodes, turns them into boolcodes and then returns the
        fcodes of those present in DATA. Eg.:
            INPUT : [*PA, *PO]
            OUTPUT: [*PA1, *PO2, *PO3, *PA4]
        '''
        result = []
        for code in fcodes:
            code = FcodeManager(code).get_boolcode()
            #code_cleaned = fun.clean_fcode(code)
            for fcode in self.all_fcodes:
                if code == fcode.boolcode:
                    result.append(fcode.code)
        return result
    
    def get_max_depth(self) -> int:
        max_depth = 0
        for e in self.DATA.values():
            curr_depth = e.fcode.depth
            if curr_depth > max_depth:
                max_depth = curr_depth
        return max_depth
    
    def get_max_generation(self) -> int:
        'Return the maximun distance between generations'
        max_generation = 0
        for e in self.DATA.values():
            curr_generation = e.fcode.generation
            if curr_generation > max_generation:
                max_generation = curr_generation
        return max_generation

    def get_entry(self, fcode: str) -> FEntry:
        'Return the Entry object of the given fcode.'
        return self.DATA[fcode]

    def get_fcode(self, code: str, force = False) -> FcodeManager:
        'Search fcode in data and return fcode, if not, generates a new one.'
        try: fcode = self.get_entry(code).fcode
        except KeyError: fcode = FcodeManager(code, force = force)
        return fcode

    def get_opposite_sex(self, fcode: str)-> str:
        'Return the oposite sex of an fcode'
        sex = self.get_fcode(fcode).sex
        return switch_sex_letter[sex]

    def predict_sex_by_partner(self, fcode: str) -> str:
        '''
        Return the sex of a fcode as the opposite of its partner. Return "U" 
        if the partner was not found.
        '''
        if self.has_partner(fcode):
            partner = self.get_partner_code(fcode)
            return self.get_opposite_sex(partner)
        else:
            return 'U'

    def predict_father_code(self, code: str) -> list:
        fcode = self.get_fcode(code)
        predictions = []
        predictions.append(code + 'P')
        if fcode.type == 'H':
            predictions.append(fcode[:-1] + 'P')
        elif fcode.type == 'h':
            #if down layer = 'O'
            if self.get_fcode(fcode[:-1]).sex == 'M': # Male
                predictions.append(fcode[:-1])
            elif self.get_fcode(fcode[:-1]).code == '*': # OC
                predictions.append('*')
            else:
                predictions.append(fcode[:-1] + 'C')
        boolcodes = []
        for i in predictions:
            boolcodes.append(self.match_boolcode_return_fcode(i))
        predictions = predictions + boolcodes
        return self.remove_NA_from_list(predictions)
    
    def get_father_code(self, code: str) -> str:
        query = self.predict_father_code(code)
        search = self.deep_multiple_search(query)
        return self.return_first_no_NA(search)
    
    def get_father_name(self, code: str) -> str:
        search = self.get_father_code(code)
        return self.search_fcode(search)

    def predict_mother_code(self, code: str) -> list:
        fcode = self.get_fcode(code)
        predictions = []
        predictions.append(code + 'M')
        if fcode.type == 'H':
            predictions.append(fcode[:-1] + 'M')
        elif fcode.type == 'h':
            #if down layer = 'O'
            if self.get_fcode(fcode[:-1]).sex == 'F':
                predictions.append(fcode[:-1])
            # elif self.get_fcode(fcode[:-1]).code == '*': # FIXME: this method do not differenciate between father and mother
            #     predictions.append('*')
            else:
                predictions.append(fcode[:-1] + 'C')
        boolcodes = []
        for i in predictions:
            boolcodes.append(self.match_boolcode_return_fcode(i))
        predictions = predictions + boolcodes
        return self.remove_NA_from_list(predictions)
    
    def get_mother_code(self, code: str) -> str:
        query = self.predict_mother_code(code)
        search = self.deep_multiple_search(query)
        return self.return_first_no_NA(search)

    def get_mother_name(self, code: str) -> str:
        search = self.get_mother_code(code)
        return self.search_fcode(search)

    def get_parents_code(self, code: str) -> list:
        'Return al list with the parents of a fcode'
        return [self.get_father_code(code), self.get_mother_code(code)]

    def get_potential_partners(self, code: str) -> list:
        'Return a list of potential partners for the given fcode'
        fcode = self.get_fcode(code)
        prediction = [fcode.code + 'C']
        if fcode.sexed_type in ['P', 'M']:
            sex_switched = fcode.switch_sex()
            prediction.append(sex_switched)
            prediction.append(fcode.get_boolcode(sex_switched))
        if fcode.type == 'C':
            return [fcode[0:-1]]
        if fcode.has_any_parent() == True:
            prediction = self.add_parbools_to_list(prediction)
        return [i for i in prediction]
    
    def get_partner_code(self, code: str) -> str:
        'Input a fcode, returns the fcode of its partner or NA.'
        search = self.get_potential_partners(code)
        result = self.fcode_multiple_search(search) \
                + [self.parbool_search(i) for i in search]
        return self.return_first_no_NA(result)

    def get_partner_name(self, code: str) -> str:
        'Input a fcode, returns the name of its partner or NA.'
        search = self.get_partner_code(code)
        return self.search_fcode(search)
        # search = self.get_potential_partners(code)
        # result = self.fcode_multiple_search(search) \
        #         + [self.parbool_search(i) for i in search]
        # if len(result) == 1:
        #     return result[0]
        # elif len(result) > 1:
        #     for i in result:
        #         if i != 'NA':
        #             return self.search_fcode(i)
        # else:
        #     return 'NA'
    
    def get_parbool_from_fcodes(self, fcodes: List[str]) -> List[str]:
        '''
        Return the parbool codes from a list of fcodes. Return only those
        different from the fcodes provided. E.g.:
            INPUT: [ '*MP', '*PP2', '*PM', '*O2a1']
            OUTPUT: ['*PP']
        '''
        parbools = [self.get_fcode(i).get_parbool() for i in fcodes]
        result = []
        for i in parbools:
            if i not in fcodes:
                result.append(i)
        return result

    def add_parbools_to_list(self, fcodes: List[str]) -> List[str]:
        '''Return the same list with missing parbools appended.'''
        return self.get_parbool_from_fcodes(fcodes) + fcodes

    def get_potential_siblings(self, code: str) -> list:
        fcode = self.get_fcode(code)
        result = [code + 'A', code + 'O', code + 'H']
        if fcode.type == 'H':
            result = result + [fcode[0:-1],
                               fcode[0:-1] + 'A',
                               fcode[0:-1] + 'O',
                               fcode[0:-1] + 'H']
        if fcode.type == 'h': #FIXME: use function get_offspring_of_down_layer
            result = result +  [fcode[0:-1] + 'a',
                               fcode[0:-1] + 'o',
                               fcode[0:-1] + 'h']
        if fcode.has_any_parent() == True:
            result = self.add_parbools_to_list(result)
        return result

    def get_siblings_code(self, code: str) -> list:
        search = self.get_potential_siblings(code)
        result = self.match_boolcodes_return_fcodes(search)
        return [i for i in result if i != code]
    
    def get_siblings_name(self, code:str) -> list:
        search = self.get_siblings_code(code)
        return self.search_fcodes(search)
    
    def build_offspring_of_down_layer(self, code: str) -> list:
        '''
        Build three fcodes by replacing the last layer of the input code with
        a/o/h
        '''
        fcode = self.get_fcode(code)
        result = [fcode[0:-1] + 'a', fcode[0:-1] + 'o', fcode[0:-1] + 'h']
        return [''.join(i) for i in result]
    
    def build_offspring_of_last_layer(self, code: str) -> list:
        '''
        Build three fcodes by adding an upper layer with a/o/h to the input code
        '''
        fcode = self.get_fcode(code)
        result = [fcode[0:] + 'a', fcode[0:] + 'o', fcode[0:] + 'h']
        return [''.join(i) for i in result]

    def get_potential_offspring(self, code: str) -> list:
        'Return the potential fcodes of a given "code"'
        fcode = self.get_fcode(code)
        search = []
        # if self is P or M, then offspring = layer-1 + siblings of layer-1
        if fcode.type == 'X':
            last_layer = fcode[0:-1]
            last_layer_siblings = self.get_siblings_code(last_layer)
            for s in last_layer_siblings:
                search.append(s)
            search.append(last_layer)
        elif fcode.type in ['H', 'h']:
            search = self.build_offspring_of_last_layer(code)
        elif fcode.type == 'C':
            predic_off_1 = self.build_offspring_of_down_layer(code)
            for i in predic_off_1:
                search.append(i)
        elif fcode.type == '*':
            offspring = ['*a', '*o', '*h']
            for i in offspring:
                search.append(i)
        if fcode.has_any_parent() == True:
            search = self.add_parbools_to_list(search)
        return search
    
    def get_offspring_code(self, code: str) -> list:
        search = self.get_potential_offspring(code)
        return list(set(self.match_boolcodes_return_fcodes(search)))
    
    def get_offspring_names(self, code: str) -> list:
        search = self.get_offspring_code(code)
        return self.search_fcodes(search)
    
    def get_family_members(self, code: str) -> dict:
        '''
        Return a dict with the fathers, siblings and offspring of an fcode with
        the folloging format:
            dict = {
                'X': parents,
                'C': partner,
                'H': siblings,
                'h': offspring 
            }
        '''
        parents = self.get_parents_code(code)
        partner = self.get_partner_code(code)
        siblings = self.get_siblings_code(code)
        offspring = self.get_offspring_code(code)
        result = {
            'X': parents,
            'C': partner,
            'H': siblings,
            'h': offspring
        }
        return result
    
    def __iter__(self):
        'Iterates through registry items (name, fcode).'
        for entry in self.DATA.values():
            yield entry
