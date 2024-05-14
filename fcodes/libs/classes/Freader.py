from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.data.legend_ENG import legend
from fcodes.libs.classes.Fgenerator import Fgenerator

class Freader(FcodeManager):
    def __init__(self, code = '*', force=False) -> None:
        super().__init__(code, force)
    
    def get_layer_number(self, layer: str)-> str:
        'Return the number of a layer. E.g.: A1 -> 1'
        number = []
        for i in layer:
            if i.isnumeric():
                number.append(i)
        return ''.join(number)
    
    def get_layer_symbol(self, layer: str)-> str:
        'Return the symbol of a layer. E.g.: A1 -> A'
        return layer[0]
    
    def read_layer_symbol(self, layer: str)-> str:
        'Return the text of the symbol of a layer. E.g.: A1 -> sister'
        symbol = self.get_layer_symbol(layer)
        return legend[symbol]
    
    def read_layer_number(self, layer: str)-> str:
        'Return the text of the symbol of a layer. E.g.: A1 -> sister'
        number = self.get_layer_number(layer)
        if number != '':
            return legend[number]
        else:
            return 'NA'
    
    def read_layer(self, layer: str)-> str:
        'Return the text of the layer. E.g.: A1 -> first sister'
        number = self.read_layer_number(layer)
        symbol = self.read_layer_symbol(layer)
        sex = FcodeManager(layer, force=True).sex
        sex_dic = {'M': 'his', 'F': 'her', 'U': 'its'}
        pronoum = sex_dic[sex]
        if self.get_layer_symbol(layer) in ['P','M','C'] and number != 'NA':
            return '{} (the {} among {} siblings)'.format(symbol, number, pronoum)
        elif number != 'NA':
            return '{} {}'.format(number, symbol)
        else:
            return '{}'.format(symbol)
        
    def read_fcode(self, fcode:str)-> str:
        'Return the text of the fcode.'
        fc = FcodeManager(fcode)
        reading = []
        for i, layer in enumerate(reversed(fc.layers)):
            if fc.depth == 2:
                reading.append("My")
                reading.append(self.read_layer(layer))
                return ' '.join(reading)
            elif i == 0:
                reading.append("The")
                reading.append(self.read_layer(layer))
            elif i == fc.depth - 2:
                reading.append("of my")
                reading.append(self.read_layer(layer))
                return ' '.join(reading)
            else:
                reading.append("of the")
                reading.append(self.read_layer(layer))
        return ' '.join(reading)
    
    def read_fcodes(self, fcodes: list)-> list:
        """Return a list of tuples with the following format:
            [(fcode_1, reading), (fcode_2, reading),...]
        """
        result = []
        for i in fcodes:
            reading = self.read_fcode(i)
            result.append((i, reading))
        return result
    
    def build_training_data(self, size = 50)-> list:
        'Generate fcode data for the training of a NLP'
        gen = Fgenerator()
        fcodes = []
        training_data = []
        # Build random fcodes
        for i in range(size):
            random_fcode = gen.get_random_fcode()
            counter = 0
            # Tries to generate a unique code 100 times before returning
            while random_fcode in fcodes:
                if counter >= 100:
                    return training_data
                random_fcode = gen.get_random_fcode()
                counter = 0
            fcodes.append(gen.get_random_fcode())
        # Read random fcodes
        reading_list = self.read_fcodes(fcodes)
        # Generates the training data
        for line in reading_list:
            training_data.append('{}\t{}'.format(line[0], line[1]))
        return training_data
    
    def generate_training_data_file(self, filepath:str, size = 50)-> None:
        'Generate fcode data for the training of a NLP'
        training_data = self.build_training_data(size = size)
        gen = Fgenerator()
        gen.save_fcodes_as_data(training_data, filepath)
            
