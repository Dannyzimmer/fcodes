import sys
import os
from fcodes import parameters as par
from fcodes.libs.classes.FBook import FBook

help_message = '''
Fmanager                User Commands

NAME
       fmanager - manage fcodes

SYNOPSIS
       python3 fmanager.py -g KINSHIP  FCODE  [FILE]
       python3 fmanager.py -r [FILTER] [FILE]
       python3 fmanager.py -s FCODE    [FILE]
       python3 fmanager.py -c FCODE    FCODE
       python3 fmanager.py -h

DESCRIPTION
       fmanager manages the fcodes of a tsv FILE (data.txt in the current
       directory by default). The FILE must have two columns, the first one with
       fcodes, the second one with the corresponding relatives names.

KINSHIP
       P: father
       M: mother
       X: parents
       C: partner
       H: sibling
       h: offspring

OPERATIONS
    -g KINDSHIP, --get-code KINSHIP FCODE
            Gets the codes given a KINSHIP.

    -G KINDSHIP, --get-name KINSHIP FCODE
            Gets the name given a KINSHIP.

    -s FCODE, --search FCODE
            Return the name of the given FCODE.

    -r, --report [FILTER]
            Return an HTML report of all the relationships present on the FILE.
            Relationships can be filtered by the beginning of their fcode.
    
    -c FCODE FCODE, --consanguinity FCODE FCODE
            Return the coefficient of consanguinity between two fcodes.
    
    -h, --help display this help and exit

EXAMPLES
    Get the codes of the parents of *MM
            python3 ./run.py -g X *PP

    Get the names of the siblings of *MM
            python3 ./run.py -G H *PP

    Return a report filtering by *PP
            python3 ./run.py -r *PP

    Return the name of the fcode *CM using a file in the desktop 
            ./run.py -s *CM /home/user/Desktop/family_data.txt


'''

# Detect if a path to data.txt is passed
default_path = not os.path.isfile(sys.argv[-1])
DATA_PATH = par.DATA_PATH if default_path else sys.argv[-1]

arg_1 = sys.argv[1]
# Get codes and names
if arg_1 in ['-g', '--get-code', '-G', '--get-name']:
    kinship = sys.argv[2]
    fcode = sys.argv[3]
    fbook = FBook(DATA_PATH)
    
    if kinship == 'P':
        if arg_1 in ['-g', '--get-code']:
            print(fbook.get_father_code(fcode))
        else:
            print(fbook.get_father_name(fcode))

    elif kinship == 'M':
        if arg_1 in ['-g', '--get-code']:
            print(fbook.get_mother_code(fcode))
        else:
            print(fbook.get_mother_name(fcode))

    elif kinship == 'X':
        if arg_1 in ['-g', '--get-code']:
            parents = [fbook.get_father_code(fcode), fbook.get_mother_code(fcode)]
        else:
            parents = [fbook.get_father_name(fcode), fbook.get_mother_name(fcode)]
        for i in parents: print(i)

    elif kinship == 'C':
        if arg_1 in ['-g', '--get-code']:
            print(fbook.get_partner_code(fcode))
        else:
            print(fbook.get_partner_name(fcode))

    elif kinship == 'H':
        if arg_1 in ['-g', '--get-code']:
            for i in fbook.get_siblings_code(fcode): print(i)
        else:
            for i in fbook.get_siblings_name(fcode): print(i)

    elif kinship == 'h':
        if arg_1 in ['-g', '--get-code']:
            for i in fbook.get_offspring_code(fcode): print(i)
        else:
            for i in fbook.get_offspring_names(fcode): print(i)

# Search
elif arg_1 in ['-s', '--search']:
    fcode = sys.argv[2]
    fbook = FBook(DATA_PATH)
    result = fbook.search_fcode(fcode)
    if result == None:
        pass
    else:
        print(result)

# Report
elif arg_1 in ['-r', '--report']:
    # --r [FILTER] [FILE]
    from libs.modules import html_report as rep
    if len(sys.argv) == 2:
        rep.print_full_report_html()
    elif len(sys.argv) == 3:
        # if not FILE
        if default_path:
            rep.print_full_report_html(sys.argv[-1])
        # if FILE
        else:
            rep.print_full_report_html()
    elif len(sys.argv) == 4:
        fbook = FBook(DATA_PATH)
        rep.print_full_report_html(pattern = sys.argv[2],
                                   fbook = fbook)

# Consanguinity
elif arg_1 in ['-c', '--consanguinity']:
    from libs.modules import consanguinity as cs
    fcode1 = sys.argv[2]
    fcode2 = sys.argv[3]
    print(cs.get_coefficient_of_consanguinity(fcode1, fcode2))

# Help
elif arg_1 in ['-h', '--help']:
    print(help_message)

# clt.print_report('*O2')
# clt.print_report('*PP')
# clt.print_partner('*PP')
# y = rep.fbook.get_potential_partners('*PP2')
# [rep.fbook.parbool_search(i) for i in y]

#FIXME: 
# [x] parents and siblings of *O2
# [x] partner of *O2 (currently his mother)
# [x] father and mother of *M (*MP)
# [x] Sibling *PA3 of *PO1
# [x] Offspring of *CMA1o1o1
# [X] Offspring of *CMA1o1C

pass