class TreeCanvas:
    def __init__(self, width, height, scale = 1) -> None:
        self.width = width
        self.height = height
        self.scale = scale
        self.colspace = 100
        self.rowspace = 100
        self.margin = 100
        self.col_min = self.get_col_min()
        self.row_min = self.get_row_min()
        self.col_max = self.get_col_max()
        self.row_max = self.get_col_min()
        self.cols = self.get_cols()
        self.rows = self.get_rows()
        self.nrows = len(self.rows)
        self.ncols = len(self.cols)

    def get_col_min(self):
        ratio = self.width/self.height
        # return -self.width//2
        return int(-self.width//2 + self.margin // 2)
    
    def get_col_max(self):
        return self.width//2 - self.margin

    def get_row_max(self):
        return self.height//2 - self.margin

    def get_row_min(self):
        ratio = int(self.width/self.height)
        return int(-self.height//2 + self.margin // 2)
        #return int(-self.height//2 - rated_half + self.margin)

    def get_cols(self) -> list:
        min = self.get_col_min()
        max = self.get_col_max()
        rg = range(min, max, self.colspace * self.scale)
        cols = [i for i in rg]
        return cols
    
    def get_rows(self) -> list:
        min = self.get_row_min()
        max = self.get_row_max()
        rg = range(min, max, self.rowspace * self.scale)
        rows = [i for i in rg]; rows.reverse()
        return rows

    def translate(self, coordinates: tuple) -> tuple:
        '''
        Translate numbers in the scale of the canvas.
            INPUT: (x, y)
            OUTPUT: (col, row) 
        '''
        return (self.cols[coordinates[0]], self.rows[coordinates[1]])

# canvas = TreeCanvas(1000, 1000)

pass