from fcodes.libs.classes.Fcode import FcodeManager
from fcodes.libs.external import graphviz
from fcodes.libs.classes.FBook import FBook
from fcodes import parameters as par
from typing import List, Dict, Tuple
from operator import attrgetter
from fcodes.libs.modules import tree_filters as tf
from fcodes.libs.data.keys import switch_sex_letter

class FamilyTree():
    def __init__(self, data_path = par.DATA_PATH):
        self.fbook = FBook(DATA_path = data_path)
        self.M = '#B4DBDE'
        self.F = '#E6C8E1'

    def get_ID_marriage(self, marriage: Tuple[str, str]) -> str:
        'Return the ID of a given marriage (a tuple of two fcodes)'
        fbook = self.fbook
        mar = [fbook.get_fcode(marriage[0]),
            fbook.get_fcode(marriage[1])]
        mar.sort(key = lambda x: x.sex)
        mar_ID = ['X(', mar[0].code, '/', mar[1].code, ')']
        return ''.join(mar_ID)

    def build_parental_node(self, fcode: str) -> str: #FIXME: adapt to new IDs
        '''
        Build the parental node from an fcode or return NA
        (for parents)
        '''
        fd = self.fbook.get_fcode(fcode)
        partner = self.fbook.get_partner_code(fcode)
        if partner == 'NA': 
            return 'NA'
        partner = self.fbook.get_fcode(partner)
        num_A = str(partner.number) #[-1] if partner[-1].isnumeric() else '0'
        num_B = str(fd.number) #fcode[-1] if fcode[-1].isnumeric() else '0'
        result = fd.boolcode + 'X' + num_A + '|' + num_B
        result = self.get_ID_marriage((fcode, partner.code))
        return result

    def search_ID_parental_node(self, fcode: str) -> str:
        'Search the parental node ID of an fcode, return NA if not found'
        father = self.fbook.get_father_code(fcode)
        return self.build_parental_node(father)

    def get_main_OFFnode_from_PARnode(self, parental_node:str) -> str:
        return parental_node + '|H'

    def build_ID_main_offspring_node(self, fcode: str) -> str:
        '''
        Return the offspring node from an fcode of type X, return NA if not found
        (from offspring).
        '''
        fd = self.fbook.get_fcode(fcode)
        if fd.type in ['H', 'h']:
            father = self.fbook.get_father_code(fcode)
            if father != 'NA':
                parental_node = self.search_ID_parental_node(father)
            else:
                return 'NA'
            result = self.get_main_OFFnode_from_PARnode(parental_node)
            return result
        else:
            raise Exception("Fcode must be of type H or h")

    def search_ID_main_offspring_node(self, fcode: str) -> str:
        '''
        Search the main offspring node of an fcode, return NA if not found
        (from parents)
        '''
        offspring = self.fbook.get_offspring_code(fcode)
        if offspring != []:
            father = self.fbook.get_father_code(offspring[0])
            main_off_node = self.build_parental_node(father)
            return self.get_main_OFFnode_from_PARnode(main_off_node)
        else:
            return 'NA'

    def build_minor_offspring_node(self, fcode: str) -> str:
        fd = self.fbook.get_fcode(fcode)
        parental_node = self.search_ID_parental_node(fcode)
        main_off_node = self.get_main_OFFnode_from_PARnode(parental_node)
        return main_off_node + '(' + fd.code + ')'

    def build_person_node(self, graph: graphviz.Digraph, fcode: str) -> None:
        fd = self.fbook.get_fcode(fcode)
        name = self.fbook.search_fcode(fcode)
        sex = fd.sex
        if sex == 'U':
            sex = self.fbook.predict_sex_by_partner(fcode)
        shape = 'box' if sex in ['M', 'U'] else 'box'
        color = {
            'M':'#B4DBDE',
            'F':'#E6C8E1',
            'U': '#DADADA'}
        graph.node(fcode, name, shape = shape, fillcolor = color[sex], style='filled')

    def build_link_node(self, graph: graphviz.Digraph, ID: str) -> None:
        graph.node(ID, shape = 'point') 

    def get_marriage(self, fcode) -> tuple:
        '''
        Return a tuple with both parts of the marriage from a single fcode. If
        no partner found return NA for that partner, e.g.: (*PM, 'NA')
        '''
        partner = self.fbook.get_partner_code(fcode)
        return (fcode, partner)

    def build_parents_nodes(self, graph: graphviz.Digraph, marriage: tuple) -> str:
        '''
        Build the parents from a list of two fcodes, return the parental node,
        '''
        parental_node = self.build_parental_node(marriage[0])
        self.build_person_node(graph = graph, fcode = marriage[0]) # parent 1
        self.build_person_node(graph = graph, fcode = marriage[1]) # parent 2
        self.build_link_node(graph = graph, ID = parental_node)
        return parental_node

    def build_edge(self, graph: graphviz.Digraph, node_ids: tuple) -> None:
        'Build an edge between two nodes by their node_ids (node_id_1, node_id_2)'
        graph.edge(node_ids[0], node_ids[1], dir = 'none')

    def build_offspring_nodes(self, 
            graph_link_nodes: graphviz.Digraph, 
            graph_offspring: graphviz.Digraph,
            marriage: tuple) -> tuple:
        '''
        Build the offspring nodes from a marriage tuple (father, mother), return 
        the nodes_id, main and minor offspring nodes as a tuple as follows:
            (main, [(minor_x, node_id_x), (minor_y, node_id_y) ...])
        or if no offspring:
            ('NA', 'NA')
        '''
        fd = self.fbook.get_fcode(marriage[0])
        offspring = self.fbook.get_offspring_code(marriage[0])
        if offspring != []:
            result = []
            # main offspring node
            main_ON = self.search_ID_main_offspring_node(fd.code)
            self.build_link_node(graph = graph_link_nodes, ID = main_ON)
            # minor offspring nodes
            minor_ON = []
            for o in offspring:
                mi_on = self.build_minor_offspring_node(o)
                minor_ON.append(mi_on)
                result.append((mi_on, o))
            # if more than one offspring
            if len(offspring) > 1:
                for on in minor_ON:
                    self.build_link_node(graph = graph_link_nodes, ID = on)
            for p in offspring:
                self.build_person_node(graph = graph_offspring, fcode = p)
            return (main_ON, result)

        return ('NA', 'NA')

    def arrange_edges(self, edge_list: list, main_node: str) -> list:
        '''
        Return edges from an edge_list puting a main_node in the middle:
            INPUT: 
                edge_list =  [A, B, C, D]    main_node = X 
            OUTPUT:
                [(A,B), (B,X), (X,B), (B,C)]
        '''
        half_list = len(edge_list)//2
        edge_list.insert(half_list, main_node)
        return [i for i in zip(edge_list, edge_list[1:])]

    def build_edges_from_list(self, graph: graphviz.Digraph, edge_list: list) -> None:
        '''
        Build edges from a list of tuples of len 2 [(node_1, node_2), ...]
        '''
        for edges in edge_list:
            self.build_edge(graph = graph, node_ids = edges)
        
    def build_parents(self, main_graph: graphviz.Digraph,
                    parents_graph: graphviz.Digraph,
                    marriage: tuple):
        parental_node = self.build_parents_nodes(parents_graph, marriage)
        self.build_edge(parents_graph, (marriage[0], parental_node))
        self.build_edge(parents_graph, (parental_node, marriage[1]))
        main_graph.subgraph(parents_graph)

    def build_offspring(self, 
        main_graph: graphviz.Digraph, 
        graph_link_offspring: graphviz.Digraph, 
        graph_offspring: graphviz.Digraph,
        marriage: tuple) -> None:
            'Build the offspring nodes and edges and put them in the graphs'
            off_nodes = self.build_offspring_nodes(graph_link_offspring, 
                                            graph_offspring,
                                            marriage)
            parental_node = self.build_parental_node(marriage[0])
            main_ON = off_nodes[0]
            edges_arranged = self.arrange_edges(
                edge_list = [i[0] for i in off_nodes[1]],
                main_node = main_ON)
            self.build_edge(main_graph, (parental_node, main_ON))

            # Put the minor_ON if more than one person on the offspring #FIXME
            if len(off_nodes[1]) > 1:
                self.build_edges_from_list(graph_link_offspring, edges_arranged)
                for n in off_nodes[1]:
                    minor_ON = n[0]
                    person_node = n[1]
                    self.build_edge(main_graph, (minor_ON, person_node))
            else:
                person_node = off_nodes[1]
                self.build_edge(main_graph, (main_ON, person_node[0][1]))


            main_graph.subgraph(graph_link_offspring)
            main_graph.subgraph(graph_offspring)

    def build_graph_id_from_number(self, number: int, link = False) ->str:
        'INPUT: 1 --> OUTPUT: G001, if link = True then G001_link'
        id = 'G000'
        result = id[:-len(str(number))] + str(number)
        if link == True:
            return result + '_link'
        else:
            return result

    def get_graph_id_from_fcode(self, fcode: str):
        '''
        Return the id of the graphviz graph were the fcode should be placed
        '''
        generation = self.fbook.get_fcode(fcode).generation
        return self.build_graph_id_from_number(generation)

    def sum_number_to_graph_id(self, graph_id: str, number: int) -> str:
        'Adds a number to de graph_id value: G001 --> G002 (if number = 1)'
        is_link = True if graph_id[-1] == 'k' else False
        if is_link:
            code_number = graph_id[1:-4] # _link
        else:
            code_number = graph_id[1:]
        operation = int(code_number) + number
        if is_link:
            return self.build_graph_id_from_number(operation, link=True)
        else:
            return self.build_graph_id_from_number(operation)

    def get_number_from_graph_id(self, graph_id: str) -> int:
        'Return the number of a graph id'
        'G0-1_link'
        number = []; result = []
        # get the number, including left zeros and minus symbol
        for i in graph_id:
            if i.isnumeric() or i == '-':
                number.append(i)
        number = ''.join(number)
        if number != '000':
            number = number.lstrip('0')
        return int(number)

    def substract_number_to_graph_id(self, graph_id: str, number: int) -> str:
        'Adds a number to de graph_id value: G002 --> G001 (if number = 1)'
        is_link = True if graph_id[-1] == 'k' else False
        code_number = self.get_number_from_graph_id(graph_id)
        operation = code_number - number
        if is_link:
            return self.build_graph_id_from_number(operation, link=True)
        else:
            return self.build_graph_id_from_number(operation)

    def estimate_number_of_graphs(self):
        'Return the number of graphs needed to build the family tree'
        number_of_graphs = (self.fbook.max_distance * 2) - 1
        return number_of_graphs

    def get_list_of_graph_IDs(self): # FIXME: crashes
        'Return a list of IDs for the graphviz.Digraph instances needed for the tree'
        # number_of_graphs = estimate_number_of_graphs()
        id_list = []
        for i in self.fbook.all_fcodes:
            gen = i.generation
            id_list.append(self.build_graph_id_from_number(gen))
        generations = list(set(id_list))
        result = []
        for i in generations:
            result.append(i + '_link')
        result = result + generations
        result.sort()
        return result

    GraphDict = Dict[str, graphviz.Digraph]
    def build_estimated_graphs(self) -> GraphDict: # FIXME
        '''
        Build the graphs and return a dict with graph_ids as keys and
        graphviz.Digraph instances as values. The maingraph has the key 'MAIN' e.g.:
            {
                'ROOT': <libs.external.graphviz.graphs.Digraph object at ...>,
                'G01' : <libs.external.graphviz.graphs.Digraph object at ...>,
                'G02' : <libs.external.graphviz.graphs.Digraph object at ...>,
                ...
            }
        '''
        graphs_ids = self.get_list_of_graph_IDs()
        graphs = {'ROOT': graphviz.Digraph(name='ROOT',
                                        strict=True,
                                        graph_attr={'ranksep':'1 equally'})}
        for i in graphs_ids:
            graphs[i] = graphviz.Digraph(name = 'ID_' + i,
                                        node_attr = {'style':'filled'})
            graphs[i].attr(rank = 'same')

        return graphs
        
    def build_family(self, graph_dict: GraphDict, fcode: str) -> GraphDict:
        # prepare the IDs
        parents_graph_id        = self.get_graph_id_from_fcode(fcode)
        graph_offspring_id      = self.substract_number_to_graph_id(parents_graph_id, 1)
        graph_link_offspring_id = graph_offspring_id + '_link'

        # retrieve the graphs
        main_graph           = graph_dict['ROOT']
        parents_graph        = graph_dict[parents_graph_id]
        graph_offspring      = graph_dict[graph_offspring_id]
        graph_link_offspring = graph_dict[graph_link_offspring_id]

        # build nodes and edges
        marriage = self.get_marriage(fcode)

        self.build_parents(main_graph = main_graph, 
                    parents_graph = parents_graph,
                    marriage = marriage)
        
        self.build_offspring(main_graph = main_graph,
                        graph_link_offspring = graph_link_offspring,
                        graph_offspring = graph_offspring,
                        marriage = marriage)
        
        return graph_dict

    def is_parent(self, fcode: str) -> bool:
        'Return True if the fcode has offspring'
        fc = self.fbook.get_fcode(fcode)
        offspring = self.fbook.get_offspring_code(fc.code)
        return True if len(offspring) > 0 else False

    def get_family_members(self, marriage: tuple) -> list:
        'Return a list with the marriage members and their offspring'
        marriage_cleaned = [i for i in marriage if i != 'NA']
        if len(marriage_cleaned) == 0:
            return []
        offspring = self.fbook.get_offspring_code(marriage_cleaned[0])
        return marriage_cleaned + offspring

    def filter_fcodes_of_fbook(self, 
                            all_fcodes: List[FcodeManager],
                            fun_list: list) -> List[FcodeManager]:
        '''
        Filter a list of FcodeManager objects by appliying each function on fun_list
        to each FcodeManager object. Return a list with the FcodeManager objects
        from which all functions returned True.
        '''
        fcodes_filtered = []
        for fun in fun_list:
            filtered = list(filter(fun, all_fcodes))
            for i in filtered:
                fcodes_filtered.append(i)
        result = filter(lambda i: fcodes_filtered.count(i) == len(fun_list),
                        fcodes_filtered)
        return list(result)

    def sort_all_fcodes_by_attribute(self, 
                                    all_fcodes: List[FcodeManager],
                                    attr: List[str]) -> List[FcodeManager]: # FIXME: fcodes sorting is important to the tree structure
        '''
        Sort all the fcodes on fbook by the given attributes (e.g.: sex, type).
        Return a list with all the fcodes (just the codes) sorted.
        '''
        for attribute in attr:
            all_fcodes.sort(key = attrgetter(attribute))
        return all_fcodes

    def build_family_tree(self, all_fcodes: List[FcodeManager]): # FIXME
        '''
        Build the whole family tree and return it as a graphviz.Digraph instance
        '''
        fcodes = [i.code for i in all_fcodes]
        done = []
        graph_dict = self.build_estimated_graphs()
        for f in fcodes:
            if f not in done:
                if self.is_parent(f):
                    marriage = self.get_marriage(f)
                    if 'NA' not in marriage:
                        graph_dict = self.build_family(graph_dict, f)
                        done.append(marriage[0])
                        done.append(marriage[1])
        return graph_dict['ROOT']

    def get_tree(self, sort_by = ['f_linage', 'type'], **kwargs: tf.Unpack[tf.Filters]) -> graphviz.Digraph:
        'Build the family tree'
        filter = tf.build_filters(**kwargs)
        all_fcodes = self.fbook.all_fcodes
        if filter != []:
            filtered_fcodes = self.filter_fcodes_of_fbook(all_fcodes, filter)
        else:
            filtered_fcodes = all_fcodes
        codes = self.sort_all_fcodes_by_attribute(filtered_fcodes, sort_by)
        TREE = self.build_family_tree(codes)
        return TREE

    def render_tree(self,
                    filepath,
                    sort_by = ['f_linage', 'type'], 
                    format = 'pdf',
                    **kwargs: tf.Unpack[tf.Filters])-> None:
        TREE = self.get_tree(sort_by, **kwargs)
        TREE.render(filepath, format = format)