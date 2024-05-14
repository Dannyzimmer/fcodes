from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.data import keys
from typing import Tuple

class Frecord:
    def __init__(self) -> None:
        # To build current fcode
        self.seed = '*'
        self.current_type = ''
        self.male = True
        self.fnumber = 0
        self.movement = ''  # up, down, left, right, back (str)

        # The map where the user moves {generation: [(fcode, name),...]}
        self.map = {0: [('*', '?')]}
        self.generation = 0

        self.data = []

        self.up_lineages = [
            keys.linage_key[keys.legend['father']], 
            keys.linage_key[keys.legend['partner']]
            ]
        
        # Movements
        self.vertical = ['up', 'down']
        self.horizontal = ['right', 'left']
        self.vertical_history = []

    def add_current_generation_to_map(self)-> None:
        """Adds a new key with the number in self.generation"""
        self.map[self.generation] = []
    
    def test_or_add_position(self)-> None:
        """Check if current position is in self.map, if not creates it by
        adding empty positions ('?','?') until reaching the target. E.g.:
            For generation 3, fnumber 3:
                [('?','?'), ('?','?'), ('?','?'), ('?','?')]
        """
        if self.generation not in self.map.keys():
            self.add_current_generation_to_map()
        gen_len = len(self.map[self.generation])
        if gen_len < self.fnumber:
            add = self.fnumber - gen_len
            for i in range(add):
                empty_position = ('?', '?')
                self.map[self.generation].append(empty_position) #type: ignore

    def get_map_in_current_position(self)-> Tuple[str, str]:
        """Return the element in self.map by self.position"""
        return self.map[self.generation][self.fnumber]

    def refresh_seed(self)-> None:
        """Set the self.seed using the current values of the attributes."""
        self.add_last_layer_to_history()
        self.seed = self.build_fcode()
    
    def record_data(self, fcode: str, name: str)-> None:
        """Add data to the given position on self.map."""
        self.test_or_add_position()
        self.data.append((fcode, name))

    def switch_sex(self)-> None:
        """Switch between values of self.male (True/False)"""
        if self.male:
            self.male = False
        elif self.male == False:
            self.male = True
        fc = FcodeManager(self.seed)
        self.seed = fc.switch_sex()

    def go_right(self)-> None:
        """When right key is pressed"""
        # If the last movement was vertical, resets the fnumber
        if self.movement in self.vertical:
            self.fnumber = 1
        else:
            self.fnumber += 1
        self.movement = 'right'
        self.current_type = keys.directions['horizontal'][0]
        self.refresh_seed()
    
    def go_left(self)-> None:
        """When left key is pressed"""
        # If the last movement was vertical, resets the fnumber
        if self.movement in self.vertical:
            self.fnumber = 0
        elif self.fnumber > 1:
            self.fnumber -= 1
        else:
            self.fnumber = 1
        self.movement = 'left'
        self.current_type = keys.directions['horizontal'][0]
        self.refresh_seed()

    def add_last_layer_to_history(self):
        """Adds the last layer of self.seed to the vertical_history"""
        last_layer = FcodeManager(self.seed)[-1]
        self.vertical_history.append(last_layer)
    
    def go_up(self)-> None:
        """When up key is pressed"""
        # If the last movement was horizontal, resets the fnumber
        if self.movement in self.horizontal or self.movement == '':
            self.fnumber = 1
        self.movement = 'up'
        self.male = True
        self.current_type = keys.directions['up'][0]
        self.generation += 1
        self.refresh_seed()
    
    def go_down(self)-> None:
        """When down key is pressed go to the sons of that fcode"""
        # If the last movement was horizontal, resets the fnumber
        if self.movement in self.horizontal or self.movement == '':
            self.fnumber = 1
        self.movement = 'down'
        self.current_type = keys.directions['down'][0]
        self.generation -= 1
        self.refresh_seed() # FIXME: not implemented
    
    def go_previous_layer(self)-> None:
        """When CTL + down key is pressed go to the previous layer"""
        fr = FcodeManager(self.seed)
        if len(fr.layers) > 2:
            self.generation -= 1
            fc = FcodeManager(self.seed)
            new_seed = ''.join(fc[:-2]) + self.vertical_history.pop()
            self.seed = new_seed
        else:
            self.seed = keys.legend['seed']

    def add_kinship(
            self, seed = None, sexed_type = None, fnumber = None)-> str:
        """Return the seed with a new upper layer using the class attributes."""
        use_seed = self.seed if seed == None else seed
        fc = FcodeManager(use_seed)
        # Removes the fnumber of the last layer before adding a new layer if 
        # that layer is of type X
        if fc.type in self.up_lineages:
            seed = fc.boolcode
        else:
            seed = use_seed

        sexed_type = self.get_sextype_from_type()
        result = seed + sexed_type

        if self.fnumber == 0 and self.seed != keys.legend['seed']:
            return result
        else:
            return result + str(self.fnumber)
    
    def replace_kinship(self)-> str:
        """Return the seed with a its upper layer replaced with the given 
        sexed type and fnumber.
        """
        fc = FcodeManager(self.seed)
        fc = fc[:-1] if len(self.seed) > 1 else '*'
        sexed_type = self.get_sextype_from_type()
        result = self.add_kinship(fc, sexed_type, self.fnumber)
        return result
    
    def back_on_kinship(self)-> None:
        """Go back to the previous layer of self.seed."""
        if self.seed != keys.legend['seed']:
            fc = FcodeManager(self.seed)
            self.seed = ''.join(fc[:-1])

    def get_sextype_from_type(self, type = None, male = None)-> str:
        """Transform the type in sexed type using the argument bool:
                H -> A (if male == false)
                H -> O (default)
        """
        type = self.current_type if type == None else type
        male = self.male if male == None else male
        if type in keys.types.keys():
            types = keys.types[type]
        else:
            return ''
        if male:
            return types[0]
        else:
            return types[1]
        
    def build_fcode(self)-> str:
        """Return the seed with a new upper layer of number fnumber and the
        sexed type deduced from the argument type and male.
            *M -> *MA1 (seed= *M, type = H, fnumber = 1 and male = False)
        """
        # If movement was horizontal
        if self.current_type in keys.directions['horizontal']:
            layer_type = FcodeManager(self.seed).type
            if layer_type in keys.legend['parents']:
                return self.add_kinship()
            else:
                return self.replace_kinship()
        # If movement was vertical
        else:
            return self.add_kinship()
