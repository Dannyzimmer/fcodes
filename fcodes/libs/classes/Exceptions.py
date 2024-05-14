class FcodeFormatError(Exception):
    '''Raised when the string is not a valid fcode'''
    def __init__(self, fcode: str, message='') -> None:
        self.fcode = fcode
        self.message = str(self.fcode) + ' is not a valid Fcode.'
        super().__init__(message)

    def __str__(self):
        return self.message