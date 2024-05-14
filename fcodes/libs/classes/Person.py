from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.classes.FBook import FBook
class Person:
    def __init__(self, 
                 code: str, 
                 partner: str, 
                 father: str, 
                 mother: str,
                 siblings: list, 
                 offspring: list): #registry_path: dict):
        self.self = code
        self.partner = partner
        self.father = father
        self.mother = mother
        self.siblings = siblings
        self.offspring = offspring

    def __repr__(self):
        return self.self