from fcodes.libs.external import click
from fcodes.libs.classes.Frecord import Frecord
from fcodes.libs.classes.FamilyTree import FamilyTree
from fcodes.libs.modules import html_report as hrep
from fcodes.libs.classes.Freader import Freader
from fcodes.libs.classes.Fgenerator import Fgenerator
from fcodes.libs.classes.FBook import FBook
from fcodes.libs.data.language import consanguinity_key
from fcodes.libs.classes.Fcode import FcodeManager

fr = Frecord()

@click.group()
def cli():
    """This is the Fcode CLI. You can find more help of each command by
    using:

        fcode.py COMMAND --help.
    """
    pass

@cli.command()
def record():
    """Launches the fcode contructor (NOT YET WORKING)."""
    click.clear()
    record = "start"
    fr = Frecord()
    char_combo = ['','']
    while True:
        char = click.getchar()
        click.echo(char)
        if char_combo:
            char_combo.pop()
        char_combo.append(char)
        # Commands executed before the fcode construction
        if record == "start":
            record = "main"
        if record == "main":
            # Uses WASD to contruct the fcode
            if char == "a":
                click.clear()
                fr.go_left()
                click.echo(fr.seed)
            elif char == "d":
                click.clear()
                fr.go_right()
                click.echo(fr.seed)
            elif char == "w":
                click.clear()
                fr.go_up()
                click.echo(fr.seed)
            elif char == "s":
                click.clear()
                fr.go_down()
                click.echo(fr.seed)
            elif char == '\x1b[D':
                click.clear()
                fr.go_previous_layer()
                click.echo(fr.seed)
            elif char == "x":
                click.clear()
                fr.switch_sex()
                click.echo(fr.seed)
            elif char == "q":
                click.clear()
                record = "quit"
            pass

        # Exit
        elif record == "quit":
            return

@cli.command()
@click.option('--output', default='my_tree', help='Output filepath')
@click.argument("fcode_file")
def tree(fcode_file: str, output: str)-> None:
    """ Generates a family tree figure in PDF from a TSV 
    file with fcodes and names.
    """
    TREE = FamilyTree(fcode_file)
    TREE.render_tree(output)

@cli.command()
@click.option('--pattern', default='', help='Return a report of fcodes starting with this pattern')
@click.option('--columns', default=3, help='Number of columns of the report')
@click.argument("fcode_file")
def report(fcode_file: str, pattern: str, columns: int):
    """Prints an HTML report of a TSV file with fcodes and names.
    The output is displayed on the command line, so it is advisable
    to redirect it to an HTML file using a pipe:

        fcode.py report FCODE_FILE > myTree.html

    """
    hrep.report(fcode_file, pattern, columns)

@cli.command()
@click.argument("fcode")
def read(fcode: str)-> None:
    """Return the reading of the given fcode."""
    reader = Freader()
    reading = reader.read_fcode(fcode)
    click.echo(reading)

@cli.command()
@click.option(
    '--fcode',
    'generate',
    default = 6, 
    help = 'Generates a random fcode (max depth 6).',
    flag_value = 'fcode')
@click.option(
    '--data',
    'generate',
    default = 3,
    help = 'Generates a random tree (max size 30).',
    flag_value = 'data')
@click.option(
    '--name',
    'generate',
    help = 'Generates a random name.',
    flag_value = 'name')
def random(generate: str)-> None:
    """Return random fcodes, names and data."""
    generator = Fgenerator()
    if generate == 'fcode':
        click.echo(generator.get_random_fcode())
    elif generate == 'data':
        random_tree = generator.get_random_tree_data(max_size = 30)
        for i in random_tree:
            click.echo(i)
    elif generate == 'name':
        firstname = generator.get_random_first_name()
        surname = generator.get_random_surname()
        click.echo(firstname + ' ' + surname)
    else:
        click.echo('Please provide a valid option.')

@cli.command()
@click.option(
    '--fcode',
    'attribute',
    help = 'Search an fcode, return its name.',
    flag_value = 'search_fcode')
@click.option(
    '--name',
    'attribute',
    help = 'Search a name, return its fcode.',
    flag_value = 'search_names')
@click.argument("fcode_file")
@click.argument("query")
def search(fcode_file: str, attribute, query):
    """Searches names or fcodes in a TSV file with fcodes and names. When
    searching for names, return all the matching fcodes with that name
    associated with the following format:

        Name Surname    kinship fcode
    """
    fbook = FBook(fcode_file)
    reader = Freader()
    fun = getattr(fbook, attribute)
    result = fun(query)
    if attribute == 'search_fcode':
        click.echo(result)
    elif attribute == 'search_names':
        if result != []:
            lines = []
            sep = '\t'
            for i in result:
                name = fbook.search_fcode(i)
                reading = reader.read_fcode(i)        
                try:
                    kinship = consanguinity_key[FcodeManager(i).get_sexed_linage()]
                except KeyError:
                    kinship = 'NA'
                lines.append((name, i, kinship, reading))

            names_ljust = max([len(i[0]) for i in lines])
            fcodes_ljust = max([len(i[1]) for i in lines])
            kinship_ljust = max([len(i[2]) for i in lines])
            for line in lines:
                click.secho(
                    line[0].ljust(names_ljust) + sep, fg='cyan', nl = False)
                click.secho(
                    line[1].ljust(fcodes_ljust) + sep, fg='green', nl = False)
                click.secho(
                    line[2].ljust(kinship_ljust) + sep, fg='reset', nl = False)
                click.echo(line[3])
