from fcodes.libs.classes.FBook import FBook
from fcodes.libs.external import unidecode as uni
from fcodes import parameters as par

def print_offspring(code, fbook):
    result = fbook.get_offspring_names(code)
    codes = fbook.get_offspring_code(code)
    for c, name in zip(codes, result):
        print('		<a href="#' + c + '" class="relative">' + name + '</a><br>')

def print_parents(code, fbook):
    codes = [fbook.get_father_code(code), fbook.get_mother_code(code)]
    result = fbook.search_fcodes(codes)
    for code, parent in zip(codes, result):
        print('		<a href="#' + code + '" class="relative">' + parent + '</a><br>')

def print_siblings(code, fbook):
    result = fbook.get_siblings_name(code)
    codes = fbook.get_siblings_code(code)
    for c, name in zip(codes, result):
        print('		<a href="#' + c + '" class="relative">' + name + '</a><br>')

def print_partner(code, fbook):
    result = fbook.get_partner_name(code)
    result = result if result != 'NA' else 'NA'
    print('		<a href="#' + code + '" class="relative">' + result + '</a><br>')

def insert_tag(tag: str, text: str, attribute = None) -> str:
    if attribute != None:
        open_tag = tag[0:-1] + ' ' + attribute + tag[-1]
    else:
        open_tag = tag
    close_tag = tag[0] + '/' + tag[1:]
    return open_tag + text + close_tag

def new_line(text = '') -> str:
    return '<br>' + text

def add_CSS(columns = 3):
    result = '''
<!DOCTYPE html>
<html>
<head>
<style>
body {
  width: 230mm;
  height: 100%;
  margin: auto;
  padding: 0;
  font-size: 10pt;
  font-family: "Monaco";
  background: #FEFEFE;
  column-count: '''+str(columns)+''';
  column-gap: 5px;
  column-rule: 4px double #C7C7C7;
  -webkit-print-color-adjust:exact !important;
  print-color-adjust:exact !important;
}

div.section {
    color: #818181;
    margin-left: 25px;
    margin-bottom:0px;
    margin-top: 0px;
    white-space: pre;
}

.relative{
    color: #181818;
    margin-left: 40px;
    margin-bottom:0px;
    margin-top: 0px;
}

div.header_box{
    padding: 5px;
    margin: 5px;
    background-color: #EFEFEF;
}

div.person{
    margin-left: 15px;
    margin-bottom:15px;
    margin-top:0px;
    white-space: normal;
}

h4 {
  color: maroon;
  margin-left: 0px;
  white-space: pre;
}

a:link {
  text-decoration: none;
  color: #224746;
  margin-left: 40px;
  margin-bottom:0px;
  margin-top: 0px;
  white-space: pre;
}

a:visited {
  color: #376665;
}

</style>
</head>
'''
    return result

def put_person_header(name, consanguinity_name, fcode: str) -> str:
    result = '''<div class="header_box">
        <h4 id="'''+fcode+'''" style="display:inline">'''+name+'''</h4>
        <em>('''+consanguinity_name+''')</em>&ensp;
    </div>'''
    return result

def print_report_html(code, fbook, pattern = ''):
    name = fbook.search_fcode(code)
    if pattern != '':
        cons_code = '*' + str(code[len(pattern):])
        consanguinity_name = fbook.get_fcode(cons_code).get_consanguinity_name()
    else:
        consanguinity_name = fbook.get_fcode(code).get_consanguinity_name()
    print('<div class="person">')
    print('	' + put_person_header(name, consanguinity_name, code))
    print('	' + insert_tag('<div>', 'PARENTS', 'class="section"'))
    print_parents(code, fbook = fbook)
    print('	' + insert_tag('<div>', 'SIBLINGS', 'class="section"'))
    print_siblings(code, fbook = fbook)
    print('	' + insert_tag('<div>', 'PARTNER', 'class="section"'))
    print_partner(code, fbook = fbook)
    print('	' + insert_tag('<div>', 'OFFSPRING', 'class="section"'))
    print_offspring(code, fbook = fbook)
    print('</div>')

def print_full_report_html(fbook, pattern = '', columns = 3):
    '''
    Return the HTML code of a report. Persons can be filtered using "pattern", e.g.:
        pattern = '*C' --> *CM, *CP, *CO2a1...
    '''
    try:
        # sort fcodes alphabetically
        if pattern != '':
            codes = [f.code for f in fbook.all_fcodes if ''.join(f.code[0:len(pattern)]) == pattern]
        else:
            codes = [f.code for f in fbook.all_fcodes]
        names = [uni.unidecode(fbook.search_fcode(i)) for i in codes]
        names, codes = zip(*sorted(zip(names, codes)))
        # print the HTML report
        print(add_CSS(columns = columns))
        print('<body>')
        for c in codes:
            print_report_html(code = c, pattern = pattern, fbook = fbook)
        print('</body>')
        print('</html>')
    except ValueError:
        pass

def report(fcode_file: str, pattern = '', columns = 3)-> None:
    fb = FBook(fcode_file)
    print_full_report_html(
            pattern = pattern,
            fbook = fb,
            columns = columns)