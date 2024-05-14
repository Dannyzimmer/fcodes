from fcodes.libs.classes.FBook import FBook
from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.modules import functions as f
from fcodes import parameters as par

fbook = FBook(par.DATA_PATH)

def get_common_ancestor(fcode1: str, fcode2: str) -> str:
    '''Return the common ancestor between two fcodes'''
    fcode_1 = [i for i in FcodeManager(fcode1)][::-1]
    fcode_2 = [i for i in FcodeManager(fcode2)][::-1]
    fcode_1, fcode_2 = f.convert_list_to_same_length(fcode_1, fcode_2)
    last = ''
    for a, b in zip(fcode_1, fcode_2):
        if a == b:
            last = a
        else:
            return last
    return 'NA'

# def get_common_ancestor_multiple(fcodes: list[str]) -> str:
#     '''Return the common ancestor from a list of fcodes'''
#     depths = [FcodeManager(i).depth for i in fcodes]
#     # sort fcodes by depth
#     depths, fcodes = zip(*sorted(zip(depths, fcodes))); fcodes = list(fcodes)
#     half = len(fcodes)//2
#     result = [] 
#     for f1, f2 in zip(fcodes[0:half], fcodes[half:]):
#         result.append(get_common_ancestor(f1, f2))
#     if len(result) > 1:
#         result = get_common_ancestor_multiple(result)
#         return result
#     elif len(result) == 1:
#         return result[0]
#     elif len(result) == 0:
#         return ''
#     else:
#         return ''

def get_common_ancestor_name(fcode1:str, fcode2:str, fbook = fbook) -> str:     #FIXME: does not return true values
    '''Look for the common ancestor, then return its name'''
    ancestor = get_common_ancestor(fcode1, fcode2)
    if ancestor != '':
        return fbook.search_fcode(ancestor)
    else:
        return 'NA' 

def get_divergent_sections(fcode1:str, fcode2:str) -> list:
    '''Return a list of length 2 with the divergent sections of two fcodes'''
    fcode_1 = fbook.get_fcode(fcode1)
    fcode_2 = fbook.get_fcode(fcode2)
    common_len = fbook.get_fcode(get_common_ancestor(fcode1, fcode2)).depth
    divergent_sections = [fcode_1[common_len:], fcode_2[common_len:]]
    return divergent_sections

def get_longer_divergent_section(fcode1:str, fcode2:str) -> str:
    '''Return the longer divergent section of two fcodes'''
    div_sections = get_divergent_sections(fcode1, fcode2)
    if len(div_sections[0]) > len(div_sections[1]):
        return div_sections[0]
    if len(div_sections[1]) > len(div_sections[0]):
        return div_sections[1]
    else:
        return ''
    
def get_shorter_divergent_section(fcode1:str, fcode2:str) -> str:
    '''Return the longer divergent section of two fcodes'''
    div_sections = get_divergent_sections(fcode1, fcode2)
    if len(div_sections[0]) > len(div_sections[1]):
        return div_sections[1]
    if len(div_sections[1]) > len(div_sections[0]):
        return div_sections[0]
    else:
        return ''

def is_blood_kinship(fcode1:str, fcode2:str) -> bool:
    '''True if fcode1 and fcode2 are blood relatives; False otherwise'''
    divsec = get_divergent_sections(fcode1, fcode2)
    search = [i for i in divsec[0]] + [i for i in divsec[1]]
    return False if 'C' in search else True

def get_coefficient_of_consanguinity(fcode1:str, fcode2:str) -> float:
    '''Return the coefficient of consanguinity of two fcodes'''
    blood_kinship = is_blood_kinship(fcode1, fcode2)
    if not blood_kinship:
        return 0
    LD_text = get_longer_divergent_section(fcode1, fcode2)
    SD_text = get_shorter_divergent_section(fcode1, fcode2)
    if LD_text != '':
        LD = fbook.get_fcode(LD_text, force=True).depth
    else:
        LD = 0
    if SD_text != '':
        SD = fbook.get_fcode(SD_text, force=True).depth
    else:
        SD = 0

    return 0.5**(LD + SD)