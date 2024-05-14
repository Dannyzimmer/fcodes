from fcodes.libs.classes.Fcode import FcodeManager
class FEntry:
    '''
    Each of the entries of the FBook.
    '''
    def __init__(self, name: str, fcode: FcodeManager) -> None:
        self.name:str = name
        self.fcode:FcodeManager = fcode
    
class FEntry_parents_booled(FEntry):
    def __init__(self, name: str, fcode: FcodeManager, original_code: str) -> None:
        super().__init__(name, fcode)
        self.original_code = original_code