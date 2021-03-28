import matplotlib.pyplot as plt
import networkx as nx
from matplotlib.backends.backend_pdf import PdfPages


def calculate_alfabet_labels():
    # 65 e 'A' iar 90 = 'Z' in ASCII
    alfabet_labels = [chr(i) for i in range(65, 91)]
    for i in range(65, 91):
        for j in range(65, 91):
            alfabet_labels.append(str(chr(i) + chr(j)))

    return alfabet_labels


def get_new_state(state, modified_states):
    for i in modified_states:
        if state == i[0]:
            return i[1]

    return state


def draw_automata(all_states, transitions, initial_states, final_states, alfabet, plot_title):
    # setează titlul plotului
    plt.gca().set_title(plot_title)

    # creează graful
    G = nx.MultiDiGraph()

    # adaugă nodurile
    G.add_nodes_from(all_states)

    # adaugă muchiile
    edges = [(node_a, node_b, weight) for (node_a, weight, node_b) in transitions]
    for edge in edges:
        G.add_edge(edge[0], edge[1], weight=edge[2])

    # impart nodurile pe categorii pentru a le da diferite culori
    start_nodes = initial_states
    final_nodes = final_states
    start_nodes_and_final_nodes = []
    for node in start_nodes:
        if node in final_nodes:
            start_nodes_and_final_nodes.append(node)

    rest_of_nodes = []
    for node in all_states:
        if node not in start_nodes and node not in final_nodes:
            rest_of_nodes.append(node)

    # Se creează layout-ul plotului si se fac apeluri separate pentru a desena nodurile si muchiile
    pos = nx.spring_layout(G)
    nx.draw(G, pos, with_labels=True, connectionstyle='arc3, rad = 0.1')

    # edge_label va fi elementul din alfabet cu care se face tranziția si va fi valoare din alfabet[d["weight"]]
    edge_labels = {}
    for u, v, d in G.edges(data=True):
        # pentru nodurile cu self loop nu mai afișez nimic
        if u == v:
            edge_labels.update({(u, v,): ""})
            continue

        # pentru nodurile care au tranziții de tipul A -a-> B  si B -c-> D afișez valorile pentru ambele tranziții
        if pos[u][0] > pos[v][0] and G.get_edge_data(v, u) is not None:
            edge_labels.update({(u, v,): f'{alfabet[d["weight"]]}\n\n{alfabet[G.get_edge_data(v, u)[0]["weight"]]}'})
            continue

        # pentru orice alta tranziție afișez elementul cu care aceasta are loc
        edge_labels.update({(u, v,): f'{alfabet[d["weight"]]}'})

    # se desenează muchiile
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, font_color='red')

    # se desenează nodurile
    node_size = 400
    nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), nodelist=start_nodes, node_color='green',
                           node_size=node_size)
    nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), nodelist=rest_of_nodes, node_color='yellow',
                           node_size=node_size)
    nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), nodelist=final_nodes, node_color='red',
                           node_size=node_size)
    nx.draw_networkx_nodes(G, pos, cmap=plt.get_cmap('jet'), nodelist=start_nodes_and_final_nodes, node_color='orange',
                           node_size=node_size)


def show_dfa_after_conversion(show_dfa, alfabet, initial_states, final_states, all_states, transitions):

    if show_dfa:
        print("\n-----------------------------------------------\n"
              "Afisare conversie python pentru automatul - DFA\n"
              "-----------------------------------------------")
    else:
        print("\n------------------------------------------------------\n"
              "Afisare conversie python pentru automatul - Lambda NFA\n"
              "------------------------------------------------------")

    print(f"Alfabet: {alfabet}")
    print(f"Stari initiale: {initial_states}")
    print(f"Stari finale: {final_states}")
    print(f"Toate starile: {all_states}")
    print(f"Numar Tranzitii: {len(transitions)}")
    print(f"Tranzitii:")
    for transition in transitions:
        print(transition[0], alfabet[transition[1]], transition[2])
    print("\n")


def read_and_convert_info(input_file_name, output_file_name, plot_title, alfabet_labels, draw_dfa):
    try:
        input_file = open(input_file_name, "r")
    except IOError:
        print(f"Could not open file {input_file_name}")
        return

    # se citesc datele
    alfabet = input_file.readline().rstrip().split(" ")
    # pentru fiecare caracter din alfabet se stabilește un index
    idx = 0
    alfabet_indexes = {}
    for x in alfabet:
        alfabet_indexes.update({x: idx})
        idx += 1

    initial_states = [alfabet_labels[int(i)] for i in input_file.readline().rstrip().split(" ")]
    final_states = [alfabet_labels[int(i)] for i in input_file.readline().rstrip().split(" ")]
    all_states = [alfabet_labels[int(i)] for i in input_file.readline().rstrip().split(" ")]
    transitions_number = int(input_file.readline().rstrip())
    transitions = []
    for i in range(transitions_number):
        transition = input_file.readline().rstrip().split(" ")
        transitions.append((alfabet_labels[int(transition[0])], alfabet_indexes[transition[1]],
                            alfabet_labels[int(transition[2])]))

    show_dfa_after_conversion(draw_dfa == "true", alfabet, initial_states, final_states, all_states, transitions)

    # modifica nodurile care au un self loop
    # ex: Daca B merge in B cu 'a' si 'c' atunci B se va redenumi in B_a_c
    modified_states = []
    for i in range(len(all_states)):
        loop_label = ""
        for j in range(len(transitions)):
            if transitions[j][0] == all_states[i] and transitions[j][2] == all_states[i]:
                loop_label += "_" + alfabet[transitions[j][1]]
        modified_states.append((all_states[i], all_states[i] + loop_label))
        all_states[i] += loop_label

    # se updateaza numele starilor si in tranzitii
    for i in range(len(transitions)):
        transitions[i] = (get_new_state(transitions[i][0], modified_states), transitions[i][1],
                          get_new_state(transitions[i][2], modified_states))

    # se updateaza numele starilor initiale si finale
    initial_states = [get_new_state(state, modified_states) for state in initial_states]
    final_states = [get_new_state(state, modified_states) for state in final_states]

    # desenează de 10 ori pentru ca nu desenul este facut random si nu iese de fiecare data ok
    pp = PdfPages(output_file_name)
    if draw_dfa == "true":
        figures_nr = 10
    else:
        figures_nr = 2
    for i in range(figures_nr):
        print(f"Se genereaza figura {plot_title}_{i} / {figures_nr}")
        if draw_dfa == "true":
            fig = plt.figure(figsize=(20, 15))
        else:
            fig = plt.figure(figsize=(20, 15))
        draw_automata(all_states, transitions, initial_states, final_states, alfabet, plot_title)
        pp.savefig(fig)

    pp.close()


def main():
    alfabet_labels = calculate_alfabet_labels()
    data = [("output/dfa_out.txt", "output/dfa_draw.pdf", "DFA", "true"),
            ("output/lambda_nfa_out.txt", "output/lambda_nfa_draw.pdf", "Lambda NFA", "false")]
    for (input_file_name, output_file_name, plot_title, draw_dfa) in data:
        read_and_convert_info(input_file_name, output_file_name, plot_title, alfabet_labels, draw_dfa)


if __name__ == '__main__':
    main()
