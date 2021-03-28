#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <queue>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <Windows.h>

using namespace std;

#define OPERATOR_CONCATENARE '.'
#define OPERATOR_STAR '*'
#define OPERATOR_SAU '|'
#define DESCHIDERE_PARANTEZA '('
#define INCHIDERE_PARANTEZA ')'
#define SIMBOL_LAMBDA '#'
#define EROARE "[EROARE] "
#define STARE_NESETATA 0
#define EXPRESIE_PRELUCRATA '_'


class Tranzitie {
    public:
        unsigned int stareSursa, stareDestinatie;
        char elementAlfabet;

        Tranzitie(unsigned int stareSursa, unsigned int stareDestinatie, char elementAlfabet){
            this->stareSursa = stareSursa;
            this->stareDestinatie = stareDestinatie;
            this->elementAlfabet = elementAlfabet;
    }
};

class StructuraAutomat {
    public:
        set<char> alfabet;
        unsigned int stareInitiala{};
        set<unsigned int> stari;
        vector <Tranzitie*> tranzitii;
};

class AutomatLambdaNFA : public StructuraAutomat {

    public:
        unsigned int stareFinala{}; // pentru primul pas al conversiei va exista doar o singura stare finala
        char ultimulElement{}; // folosit pentru retinerea elementului care a modificat un automat anterior

        void transfomaStare(unsigned int stareVeche, unsigned int stareNoua){
            if (stareInitiala == stareVeche){
                stareInitiala = stareNoua;
            }
            if(stareFinala == stareVeche){
                stareFinala = stareNoua;
            }
            stari.erase(stareVeche);
            stari.insert(stareNoua);

            for(auto tranzitie:tranzitii){
                if(tranzitie->stareSursa == stareVeche){
                    tranzitie->stareSursa = stareNoua;
                }
                if(tranzitie->stareDestinatie == stareVeche){
                    tranzitie->stareDestinatie = stareNoua;
                }
            }
        }
};

class AutomatDFA : public StructuraAutomat {
    public:
        set <unsigned int> stariFinale;

        // Datorita conversiei o stare poate fi {1,2,3} (inchiderea lambda a unei stari din automatul lambda NFA),
        // astfel ca ii voi asocia un indice, spre ex 1. Deci starea 1 din DFA va fi multimea {1,2,3} din NFA
        vector <set<unsigned int>> stari;
};

// alfabetul dupa ce validarea alfabetului introdus s-a efectuat
set<char> alfabetFinal;

// numarul total de stari al automatului Lmabda NFA construit pe baza expresiei regex
unsigned int totalStari;

// fiecare pereche de paranteze creeaza un nou nivel de prioritate (0 reprezinta gradul cel mai mic, iar prioritatea
// maxima reprezinta parantezele la nivelul cel mai identat)
vector <stack <AutomatLambdaNFA *>> stivaAutomate;


/** Functii pentru validarea alfabetului de intrare */
bool esteSimbolPredefinit(char x){
    // pentru simbolul lambda nu se face verificare deoarece va fi acceptata introducerea lui
    if (x == DESCHIDERE_PARANTEZA || x == INCHIDERE_PARANTEZA || x == OPERATOR_STAR || x == OPERATOR_SAU ||
        x == OPERATOR_CONCATENARE || x == EXPRESIE_PRELUCRATA) {
        return true;
    }
    return false;
}

bool esteOperator(char x) {
    if(x == OPERATOR_STAR || x == OPERATOR_SAU || x == OPERATOR_CONCATENARE){
        return true;
    }
    return false;
}

bool esteInAlfabet(char x){
    set<char>::iterator itr;
    for (itr = alfabetFinal.begin(); itr != alfabetFinal.end(); itr++){
        if(*itr == x){
            return true;
        }
    }
    return false;
}

bool alfabetValid(const string& alfabet){
    // la alfabet se va adauga si simbolul LAMBDA
    string alfabetExtins = alfabet;
    alfabetExtins.append(1,SIMBOL_LAMBDA);

    for(char x: alfabetExtins){
        if(esteSimbolPredefinit(x)){
            cout << "Alfabetul contine simbolul [" << x << "]! Acest simbol nu este permis!\n";
            return false;
        }
        alfabetFinal.insert(x);
    }
    return true;
}


/** Functii pentru validarea formatului expresiei regex **/

bool corectParantezata(const string& expresie){
    // se verifica daca expresia este corect parantezata
    stack <char> tmp;
    for(auto x:expresie){
        if(x == DESCHIDERE_PARANTEZA){
            tmp.push(DESCHIDERE_PARANTEZA);
            continue;
        }
        if(x == INCHIDERE_PARANTEZA && tmp.top() == DESCHIDERE_PARANTEZA){
            tmp.pop();
            continue;
        }
    }

    return tmp.empty();
}

bool corectPrevalidat(const string& expresie){
    // se verifica daca expresia este corect parantezata si daca contine doar simboluri din alfabetul declarat si
    // operatori
    for(auto x:expresie){
        if(!esteInAlfabet(x) && x != DESCHIDERE_PARANTEZA && x != INCHIDERE_PARANTEZA && x != OPERATOR_STAR
           && x != OPERATOR_CONCATENARE && x != OPERATOR_SAU){
            cout << "\n" << EROARE << "[corectPrevalidat] Simbolul '" << x << "' nu este nici in alfabet si nu este nici operator!\n";
            return false;
        }
    }

    if(!corectParantezata(expresie)){
        cout << "\n" << EROARE << "[expresieValida] Parantezele nu sunt puse corect in expresie!\n";
        return false;
    }

    return true;
}

bool contineSimbolConcatenare(const string& expresie){
    return std::any_of(expresie.begin(),expresie.end(),[](char x){ return x == OPERATOR_CONCATENARE; });
}

bool sablonCorect(const string& expresie, bool contineSimbolConcatenare){
    // este creat un automat care sa verifica daca expresia introdusa este valida

    // reprezinta orice element din alfabet
    const char ELEMENT_ALFABET = '~';

    // sunt indicii pentru stari
    unordered_map<char,unsigned int> indiciElemente;
    indiciElemente.insert(make_pair(OPERATOR_STAR,1));
    indiciElemente.insert(make_pair(OPERATOR_SAU,2));
    indiciElemente.insert(make_pair(DESCHIDERE_PARANTEZA,3));
    indiciElemente.insert(make_pair(INCHIDERE_PARANTEZA,4));
    indiciElemente.insert(make_pair(ELEMENT_ALFABET,5));

    unsigned int stareInitiala = 0;
    set<unsigned int> stari{0,1,2,3,4,5,6};
    set<unsigned int> stariFinale{1,4,5};

    // -1 va reprezinta LIPSA_TRANZITIE de la i la j
    // ultima linie si ultima coloana sunt pentru operatorul '.'
    int reprezentareAFD[7][7] = {   // A va reprezenta orice element din alfabet
            {-1,-1,-1, 3,-1, 5,-1}, // din satrea intiala se poate merge mai departe doar cu '(' si 'A'
            {-1,-1, 2, 3, 4, 5,-1}, // dupa '*' pot urma doar elementele: '|', '(', ')', 'A'
            {-1,-1,-1, 3,-1, 5,-1}, // dupa '|' pot urma doar elementele: '(', 'A'
            {-1,-1,-1, 3,-1, 5,-1}, // dupa '(' pot urma doar elementele: '(', 'A'
            {-1, 1, 2, 3, 4, 5,-1}, // dupa ')' pot urma doar elementele: '*', '|', '(', ')', 'A'
            {-1, 1, 2, 3, 4, 5,-1}, // dupa 'A' pot urma doar elementele: '|', '(', ')', 'A'
            {-1,-1,-1,-1,-1,-1,-1}
    };

    // daca simbolul concatenare este deja continut in expresie se face o alta verificare care sa tina cont si de acesta
    if(contineSimbolConcatenare){
        indiciElemente.insert(make_pair(OPERATOR_CONCATENARE,6));

        // dupa '*' va putea urma si '.'
        reprezentareAFD[1][6] = 6;

        // dupa ')' pot urma doar elementele: '*', '|', ')', '.'
        reprezentareAFD[4][1] = 1;
        reprezentareAFD[4][3] = -1;
        reprezentareAFD[4][5] = -1;
        reprezentareAFD[4][6] = 6;

        // dupa 'A' pot urma doar elementele: '*', '|', ')', '.'
        // 'A' va reprezenta orice element din alfabet
        reprezentareAFD[5][1] = 1;
        reprezentareAFD[5][3] = -1;
        reprezentareAFD[5][5] = -1;
        reprezentareAFD[5][6] = 6;

        // dupa '.' pot urma doar elementele: '(', 'A'
        reprezentareAFD[6][3] = 3;
        reprezentareAFD[6][5] = 5;
    }

    unsigned int stareCurenta = stareInitiala;

    // din starea intiala se poate pleca doar cu DESCHIDERE_PARANTEZA sau cu un element din alfabet
    if (expresie[0] != DESCHIDERE_PARANTEZA && !esteInAlfabet(expresie[0])){
        cout << " \n" << EROARE << "[sablonCorect]. Expresia nu poate incepe cu elementul" << expresie[0];
        return false;
    }

    // se verifica expresia
    int lim = expresie.size();
    for(int i = 0; i < lim; i++){

        char element = expresie[i];

        // se preia indicele corespunzator starii conform elementului citit
        int indiceElement;
        // simbolul lambda il consider ca element din alfabet
        if(esteSimbolPredefinit(element)){
            indiceElement = indiciElemente.find(element)->second;
        }
        else{
            indiceElement = indiciElemente.find(ELEMENT_ALFABET)->second;
        }

        // se merge in starea noua
        stareCurenta = reprezentareAFD[stareCurenta][indiceElement];

        // Daca cu elementul nu curent nu a existat din starea anterioara tranzitie automatul se opreste
        // (-1 reprezinta LIPSA_TRANZITIEI).
        if (stareCurenta == -1 && i > 0){
            cout << " \n" << EROARE << "[sablonCorect]. Dupa elementul '" << expresie[i-1] << "' nu poate urma "
                 << "elementul '" << element << "'";
             return false;
        }
    }

    for(auto stareFinala: stariFinale){
        if(stareCurenta == stareFinala){
            return true;
        }
    }

    return false;
}

bool expresieValida(const string& expresie){

    if (expresie.empty()){
        cout << "\n" << EROARE << "[expresieValida] Expresia este vida !\n";
        return false;
    }

    // se verifica daca expresia este corect parantezata si daca contine doar simboluri din alfabetul declarat si
    // operatori
    if (!corectPrevalidat(expresie)){
        cout << "\n" << EROARE << "[expresieValida] Expresia nu are formatul corect!\n";
        return false;
    }

    // expresia este trecuta printr-un automat a.i. sa se verifica daca are formatul corect
    if (!sablonCorect(expresie,contineSimbolConcatenare(expresie))){
        cout << "\n" << EROARE << "[expresieValida] Expresia nu are formatul corect!\n";
        return false;
    }

    return true;
}

string prefixToInfix(const string& expresiePrefixata) {

    stack<string> stivaSubExpresii;
    int lim = expresiePrefixata.size();

    // expresia este prelucrata de la dreapta spre stanga
    for (int i = lim - 1; i >= 0; i--) {

        // cand este operator , acesta va trebui introdus intre primele 2 elementele ale stivei
        if (esteOperator(expresiePrefixata[i])) {
            // se extrag primele doua elemente de pe stiva
            string element1 = stivaSubExpresii.top();
            stivaSubExpresii.pop();
            string element2 = stivaSubExpresii.top();
            stivaSubExpresii.pop();

            // se face concatenarea
            string tmp;
            tmp.append(1,DESCHIDERE_PARANTEZA);
            tmp.append(element1);
            tmp.append(1,expresiePrefixata[i]);
            tmp.append(element2);
            tmp.append(1,INCHIDERE_PARANTEZA);

            // se adauga noul string pe stiva
            stivaSubExpresii.push(tmp);
            continue;
        }

        // daca simbolul nu este operator atunci este adaugat pe stiva
        stivaSubExpresii.push(string(1, expresiePrefixata[i]));

    }

    // expresia infixata va fi comupsa din subexpresiile din stiva
    string expresieInfixata;
    while (!stivaSubExpresii.empty()){
        string tmp = stivaSubExpresii.top();
        stivaSubExpresii.pop();
        expresieInfixata.append(tmp);
    }
    return expresieInfixata;
}


/** Functii pentru crearea expresiei extinse **/

string adaugaPrecedentaOperatorStar(const string& expresie){
    // In cazul in care operatorul STAR este precedat de un element din alfabet  se adauga paranteze in jurul
    // elementului care il precede si al operatorului (Ex: expresia ab*cd* va fi transformata in a(b*)c(d*) )
    // daca OPERATORUL STAR este precedat de alt element nu se face nimic.
    int lim = expresie.size();
    string tmp;
    for(int i = 0; i < lim; i++){
        if(i > 0 && expresie[i] == OPERATOR_STAR && esteInAlfabet(expresie[i-1])){
            // se elimina elementul precedent
            tmp.pop_back();
            // si se adauga perechea (element precedent,operator *)
            tmp.append(1,DESCHIDERE_PARANTEZA);
            tmp.append(1,expresie[i-1]);
            tmp.append(1,expresie[i]);
            tmp.append(1,INCHIDERE_PARANTEZA);
            continue;
        }
        tmp.append(1,expresie[i]);
    }
    return tmp;
}

string adaugaPrecedentaOperatorSau(const string& expresie){
    int lim = expresie.size();
    string tmp;
    for(int i = 0; i < lim; i++){
        if(expresie[i] == OPERATOR_SAU){
            string aux;
            aux.append(1,OPERATOR_SAU);
            aux.append(1,DESCHIDERE_PARANTEZA);
            aux += adaugaPrecedentaOperatorSau(expresie.substr(i + 1));
            aux.append(1,INCHIDERE_PARANTEZA);

            if(!tmp.empty()) {
                tmp.insert(tmp.begin(),DESCHIDERE_PARANTEZA);
                tmp.append(1,INCHIDERE_PARANTEZA);
                tmp += aux;
                return tmp;
            }
            return aux;
        }
        tmp += expresie[i];
    }

    return tmp;
}

string preiaExpresie(int index, const vector <string> & indexExpresiiPrelucrate){
    string tmp = indexExpresiiPrelucrate[index];
    if(tmp.empty()){
        return tmp;
    }
    if(tmp[0] == EXPRESIE_PRELUCRATA){
        // stoi(tmp.substr(1,tmp.size()-2) reprezinta indexul pentru noua expresie
        return preiaExpresie(stoi(tmp.substr(1,tmp.size()-2)), indexExpresiiPrelucrate);
    }
    return tmp;
}

string creeazaPrecedentaOperator(const string& expresie, bool precedentaOperatorStar){

    // o data ce o expresie este prelucrata ea va primi un index
    vector <string> indexExpresiiPrelucrate;

    // se introduce toata expresia intr-un set nou de paranteze
    string expresieModificata = DESCHIDERE_PARANTEZA + expresie + INCHIDERE_PARANTEZA;

    // impart epresiile in subexpresii (in functie de paranteze)
    // capacitatea initiala pentru nivelul 0 (cel fara paranteze)
    vector <string> stivaSubExpresii(1);
    int lim = expresieModificata.size();
    for(int i = 0; i < lim; i++){

        char element = expresieModificata[i];

        if (element == DESCHIDERE_PARANTEZA){
            // cand o paranteza noua se deschide va creste nivelul de prioritate
            string tmp;
            stivaSubExpresii.push_back(tmp);
            continue;
        }

        if (element == INCHIDERE_PARANTEZA){

            string subExpresie = stivaSubExpresii[stivaSubExpresii.size()-1];
            stivaSubExpresii.pop_back();

            // corespunde nivelului curent (era existenta in expresie)
            stivaSubExpresii[stivaSubExpresii.size()-1].append(1,DESCHIDERE_PARANTEZA);

            string precedentaSubExpresie;

            if(precedentaOperatorStar) {
                // Daca se calculeaza precedenta pentru operatorul star in momentul inchiderii nivelului se verifica daca
                // este o expresie de tipul (...)*  (in paranteza poate fi orice). Daca este o expresie de acest tip,
                // expresia se transforma in ((...)*)
                if((i + 1) < lim && expresieModificata[i+1] == OPERATOR_STAR){
                    precedentaSubExpresie.append(1,DESCHIDERE_PARANTEZA);
                    precedentaSubExpresie.append(adaugaPrecedentaOperatorStar(subExpresie));
                    precedentaSubExpresie.append(1,INCHIDERE_PARANTEZA);
                    precedentaSubExpresie.append(1,OPERATOR_STAR);
                    // la urmatoarea iteratie se va sari operatorul STAR pt ca a fost adaugat acum
                    i++;
                }
                else{
                    // se adauga parantezele pentru precedenta operatorului STAR
                    precedentaSubExpresie = adaugaPrecedentaOperatorStar(subExpresie);
                }
            }
            else{
                // se adauga parantezele pentru precedenta operatorului SAU
                precedentaSubExpresie = adaugaPrecedentaOperatorSau(subExpresie);
            }

            // se creeaza indexul pentru expresie
            indexExpresiiPrelucrate.push_back(precedentaSubExpresie);
            string indexExpresie;
            indexExpresie.append(1,EXPRESIE_PRELUCRATA);
            indexExpresie.append(to_string(indexExpresiiPrelucrate.size() - 1));
            indexExpresie.append(1,EXPRESIE_PRELUCRATA);

            // pe stiva se adauga doar indexul
            stivaSubExpresii[stivaSubExpresii.size()-1] += indexExpresie;

            // corespunde nivelului curent (era existenta in expresie)
            stivaSubExpresii[stivaSubExpresii.size()-1].append(1,INCHIDERE_PARANTEZA);
            continue;
        }

        stivaSubExpresii[stivaSubExpresii.size()-1] += element;
    }

    // La final ar trebui sa mai exista doar expresia de pe nivelul 0.
    if (stivaSubExpresii.empty() || stivaSubExpresii.size() > 1){
        cout << "\n" << EROARE << "[creeazaPrecedentaOperatorSau]! Stiva de precedenta pentru opeartorul SAU nu a fost"
                                  " prelucrata corespunzator!\n";
        exit(-1);
    }

    string expresieIntermediara = stivaSubExpresii[0];
    stivaSubExpresii.pop_back();

    // se face inclocuirea in expresia intermediara (ce cu indecsi)
    string expresiePrelucrata;

    bool prelucrareFinalizata = false;
    while(!prelucrareFinalizata){
        lim = expresieIntermediara.size();
        for (int i = 0; i < lim; i++){
            // daca este intalnim acest simbol va urma un index
            if(expresieIntermediara[i] == EXPRESIE_PRELUCRATA){
                int j = i+1;
                bool indexCalculat = false;
                string indexString;
                // se merge pana se gaseste inchiderea simbolului si se preia indexul cuprins intre
                // ex: daca este _12_ atunci indexul va fi 12
                while(!indexCalculat){
                    indexString += expresieIntermediara[j];
                    j++;
                    if(expresieIntermediara[j] == EXPRESIE_PRELUCRATA){
                        indexCalculat = true;
                        // ulterior la reluare se va continua de la pozitia de dupa preluarea indexului
                        i = j;
                    }
                }

                // se transform indexul in intreg
                int index = stoi(indexString);

                // se preia valoarea expresiei
                expresiePrelucrata.append(preiaExpresie(index,indexExpresiiPrelucrate));

                continue;
            }

            expresiePrelucrata.append(1,expresieIntermediara[i]);
        }

        // daca expresia NU mai contine simbolul EXPRESIE_PRELUCRATA atunci inlocuirea este completa
        if (expresiePrelucrata.find(EXPRESIE_PRELUCRATA) == std::string::npos) {
            prelucrareFinalizata = true;
        }
        else{
            // daca inclocuirea nu este completa se reia inlcouirea dar de data aceasta pentru noua expresie
            expresieIntermediara = expresiePrelucrata;
            expresiePrelucrata = "";
        }
    }

    // se retuneaza expresia fara prima pereche de paranteze adugata la intrarea in functie
    return expresiePrelucrata.substr(1,expresiePrelucrata.size() - 2);
}

string adaugaSimbolConcatenare(const string& expresie){
    string tmp;

    // Se merge pana la lim-1 pt ca se verifica doua cate 2 elemente si se vor face urmatoarele transformari
    // (a poate fi orice element din alfabet):
    //      daca este *a --> *.a
    //      daca este *( --> *.(
    //      daca este )a --> ).a
    //      daca este )( --> ).(
    //      daca este aa --> a.a (sunt oricare 2 simboluri alaturate)
    //      daca este a( --> a.(
    int lim = expresie.size();
    for(int i = 0; i < lim - 1; i++){
        if(
                (expresie[i] == OPERATOR_STAR && esteInAlfabet(expresie[i + 1])) ||
                (expresie[i] == OPERATOR_STAR && expresie[i + 1] == DESCHIDERE_PARANTEZA) ||
                (expresie[i] == INCHIDERE_PARANTEZA && esteInAlfabet(expresie[i + 1])) ||
                (expresie[i] == INCHIDERE_PARANTEZA && expresie[i + 1] == DESCHIDERE_PARANTEZA) ||
                (esteInAlfabet(expresie[i]) && esteInAlfabet(expresie[i + 1])) ||
                (esteInAlfabet(expresie[i]) && expresie[i + 1] == DESCHIDERE_PARANTEZA)
            ){
                tmp.append(1, expresie[i]);
                tmp.append(1,OPERATOR_CONCATENARE);
                continue;
        }

        tmp.append(1, expresie[i]);
    }

    // se adauga si ultimul element din expresie (pt ca s-a mers pana la penultimul la pasul anterior)
    tmp.append(1, expresie[lim - 1]);

    return tmp;
}

string creareExpresieExtinsa(const string& regExp){

    // Se creeaza precedenta pentru operatorul STAR de la stanga spre dreapta.
    // ex: daca expresia introdusa este a(a|bc|cd*)*bcd ea va fi transformata in a((a|bc|c(d*))*)bcd
    string expresieExtinsa = creeazaPrecedentaOperator(regExp,true);

    // Se creeaza precedenta pentru operatorul SAU de la stanga spre dreapta.
    // ex: daca expresia introdusa este (ab|cd|abcd)*|ab ea va fi transformata in (((ab)|((cd)|(abcd)))*)|(ab)
    expresieExtinsa = creeazaPrecedentaOperator(expresieExtinsa, false);

    // Daca expresia nu a avut inserat operatorul de concatenare cesta va fi inserat. Operatorul concatenare va fi
    // adaugat doar dupa operatorul star, paranteza inchisa si dupa simbolul din alfabet.
    // (ex: expresia (a|bc)*ab va deveni (a|(b.c))*.a.b
    if(!contineSimbolConcatenare(regExp)){
        expresieExtinsa = adaugaSimbolConcatenare(expresieExtinsa);
    }

    // se face o reverificare a expresiei procesate
    if(!expresieValida(expresieExtinsa)){
        cout << EROARE << "[creareExpresieExtinsa]! Expresia nu a fost prelucrata corespunzator. Rezultatul nu este valid!";
        exit(-1);
    }

    return expresieExtinsa;
}


/** Functii pentru crearea automatului Lambda NFA **/

bool creareMiniAutomat(char element){ // functia principala de creare a unui nod pentru arborele de sintaxa

    if (element == DESCHIDERE_PARANTEZA){
        // cand o paranteza noua se deschide va creste nivelul de prioritate
        stack <AutomatLambdaNFA *> tmp;
        stivaAutomate.push_back(tmp);
        return true;
    }

    if (element == INCHIDERE_PARANTEZA){ // inainte de a scadea nivelul de prioritate fac legaturile necesare daca exista
                                        // elemente in stiva pt prioritatea curenta

        // Cate stive sunt active la acest moment. Fiecare stiva corespunde unui nivel de identare al parantezelor.
        unsigned int nivelPrioritate = stivaAutomate.size() - 1;

        if (!stivaAutomate[nivelPrioritate].empty()){
            // inchiderea nivelului curent

            // se extrage automatul nivelului
            auto *tmp = stivaAutomate[nivelPrioritate].top();
            stivaAutomate[nivelPrioritate].pop();
            // se elimina stiva din vectorul de stive
            stivaAutomate.pop_back();
            nivelPrioritate--;

            // Reuniunea cu automatul nivelul precedent.
            if (!stivaAutomate[nivelPrioritate].empty()){

                // se preia automatul nivelui si se va reuni cu automatul nivelului anterior
                auto *automatNivel = stivaAutomate[nivelPrioritate].top();

                // pe nivelul precedent automatul nu poate fi decat un un operatorl de tip CONCATENARE sau un nod de tip
                // SAU (valabil pt expresia extinsa)
                if(automatNivel->ultimulElement != OPERATOR_CONCATENARE && automatNivel->ultimulElement != OPERATOR_SAU){
                    cout << EROARE << "[creareMiniAutomat]. Ultimul element al nivelului precedent este [" << automatNivel->ultimulElement
                         << "]. Nu se poate face reunirea!";
                    return false;
                }

                // Cand este operator CONCATENARE, reuniunea se face prin urmatoarele modificari:
                //      - starea initiala a automatului de la nivelul precedent devine starea finala a automatului
                //        de la nivelul curent
                //      - vechea stare initiala a nivelului precedent este eliminata
                //      - starea finala a automatului de la nivelul curent este transformata in stare nefinala a.i.
                //        starea finala a automatului va fi doar starea automatului nivelului precedent
                //      - se face reuniunea starilor celor doua automate
                //      - se face reuniunea tranzitiilor celor doua automate
                if(automatNivel->ultimulElement == OPERATOR_CONCATENARE) {
                    tmp->transfomaStare(tmp->stareInitiala,automatNivel->stareFinala);
                    tmp->stareInitiala = STARE_NESETATA;
                    automatNivel->stareFinala = tmp->stareFinala;
                }
                else {
                    // Cand este operator SAU, reuniunea se face prin urmatoarele modificari:
                    //      - din starea initiala a automatului nivelului curent se duce o tranzitie cu lambda
                    //        catre starea initiala a automatului nivelului precedent
                    //      - din starea finala a automatului nivelului precedent se duce o tranzitie cu lambda
                    //        catre starea finala a automatului nivelului precedent
                    //      - modific starea initiala a automatului nivelului precedent a.i. aceasta sa nu mai fie stare
                    //        initiala
                    //      - modific starea finala  a automatului nivelului precedent a.i. aceasta sa nu mai fie stare
                    //        finala
                    //      - se face reuniunea starilor celor doua automate
                    //      - se face reuniunea tranzitiilor celor doua automate
                    automatNivel->tranzitii.push_back(new Tranzitie(automatNivel->stareInitiala,tmp->stareInitiala,SIMBOL_LAMBDA));
                    automatNivel->tranzitii.push_back(new Tranzitie(tmp->stareFinala,automatNivel->stareFinala,SIMBOL_LAMBDA));
                    tmp->stareInitiala = STARE_NESETATA;
                    tmp->stareFinala = STARE_NESETATA;
                }

                // reuniune stari
                for(auto stare: tmp->stari){
                    automatNivel->stari.insert(stare);
                }
                // reuniune tranzitii
                for(auto tranzitie: tmp->tranzitii){
                    automatNivel->tranzitii.push_back(tranzitie);
                }
                return true;
            }

            // nivelul preedent nu a avut nici un automat a.i. acest automat va deveni noul automat al nivelului
            stivaAutomate[nivelPrioritate].push(tmp);
            return true;
        }

        // elimina stiva din vectorul de stive
        stivaAutomate.pop_back();
        return true;
    }


    if (element == OPERATOR_STAR || element == OPERATOR_CONCATENARE || element == OPERATOR_SAU){

        // Cate stive sunt active la acest moment. Fiecare stiva corespunde unui nivel de identare al parantezelor.
        unsigned int nivelPrioritate = stivaAutomate.size() - 1;

        // Stiva nu poate fi goala atunci cand pe acelasi nivel vine un operator.
        if (stivaAutomate[nivelPrioritate].empty()){
            cout << EROARE << "[creareMiniAutomat]. Stiva este goala pentru operatorul [" << element << "]!";
            return false;
        }

        // Se preia automatul nivelului curent si se modifica in functie de operator.
        AutomatLambdaNFA *automatNivel = stivaAutomate[nivelPrioritate].top();
        automatNivel->ultimulElement = element;

        if(element == OPERATOR_SAU || element == OPERATOR_STAR) {
            // Modificari comune pentru ambii operatori:
            //
            // Cand este operatorul SAU/STAR, pentru automatul nivelului curent fac urmatoarele modificari:
            //      - modific starea initiala a.i. aceasta sa nu mai fie stare initiala
            //      - modific starea finala a.i. aceasta sa nu mai fie stare finala
            //      - se adauga o noua stare initiala
            //      - se adauga o noua stare finala
            //      - din noua stare initiala se duce o tranzitie cu lambda catre vechea stare initiala
            //      - din vechea stare finala se duce o tranzitie cu lambda catre noua stare finala
            unsigned int stareInitialaVeche = automatNivel->stareInitiala;
            totalStari++;
            automatNivel->stareInitiala = totalStari;
            unsigned int stareFinalaVeche = automatNivel->stareFinala;
            totalStari++;
            automatNivel->stareFinala = totalStari;
            automatNivel->stari.insert(automatNivel->stareInitiala);
            automatNivel->stari.insert(automatNivel->stareFinala);
            automatNivel->tranzitii.push_back(new Tranzitie(automatNivel->stareInitiala ,stareInitialaVeche,SIMBOL_LAMBDA));
            automatNivel->tranzitii.push_back(new Tranzitie(stareFinalaVeche ,automatNivel->stareFinala,SIMBOL_LAMBDA));

            // pentru operatorul SAU modificarile de mai sus sunt suficiente
            if(element == OPERATOR_SAU){
                return true;
            }

            // pentru operatorul STAR pe langa cele anterioare se mai adauga modificarile:
            //      - din vechea stare finala se duce o tranzitie cu lambda catre vechea stare initiala
            //      - din noua stare initiala se duce o tranzitie cu lambda catre noua stare finala
            automatNivel->tranzitii.push_back(new Tranzitie(stareFinalaVeche ,stareInitialaVeche,SIMBOL_LAMBDA));
            automatNivel->tranzitii.push_back(new Tranzitie(automatNivel->stareInitiala ,automatNivel->stareFinala,SIMBOL_LAMBDA));
            return true;

        }

        // Aici va ajunge doar operatorul CONCATENARE

        // Cand este operator CONCATENARE, pentru automatul nivelului curent nu fac nici o modificare. Reuniunea se va face
        // in momentul in care urmatorul element va fi prelucrat.
        return true;
    }

    if (esteInAlfabet(element)){
        // Cate stive sunt active la acest moment. Fiecare stiva corespunde unui nivel de identare al parantezelor.
        unsigned int nivelPrioritate = stivaAutomate.size() - 1;

        // daca exista un automat cu care sa fie reunit elementul curent.
        if (!stivaAutomate[nivelPrioritate].empty()){
            // Elementul este reunit cu automatul anterior, in functie de tipul operatorului.
            AutomatLambdaNFA *automatNivel = stivaAutomate[nivelPrioritate].top();

            // Inaintea unui element din alfabet, pe acelasi nivel, nu poate exista decat operatorul concatenare
            // (valabil pentru expresia extinsa).
            if(automatNivel->ultimulElement != OPERATOR_CONCATENARE){
                cout << EROARE << "[creareMiniAutomat]. Elementul '" << automatNivel->ultimulElement << "' este inaintea "
                     << " elementulul din alfabet '" << element << "'";
                return false;
            }

            // Cand este operator CONCATENARE, pentru automatul nivelului curent fac urmatoarele modificari:
            //      - se creeaza o noua stare
            //      - din starea finala se duce o tranzitie cu element catre noua stare
            //      - starea finala este transformata in stare nefinala
            //      - noua stare devine starea finala
            totalStari++;
            unsigned int stareNoua = totalStari;
            automatNivel->stari.insert(stareNoua);
            automatNivel->tranzitii.push_back(new Tranzitie(automatNivel->stareFinala,stareNoua,element));
            automatNivel->stareFinala = stareNoua;
            return true;
        }

        // Daca inainte elementului nu a existat operatorul concatenare ==> se creeaza un nou automat pentru elementul
        // curent care este adaugat in stiva de nivel.
        auto *automatCurent = new AutomatLambdaNFA();
        automatCurent->ultimulElement = element;
        totalStari++;
        automatCurent->stareInitiala = totalStari;
        totalStari++;
        automatCurent->stareFinala = totalStari;
        automatCurent->tranzitii.push_back(new Tranzitie(automatCurent->stareInitiala,automatCurent->stareFinala,element));
        automatCurent->stari.insert(automatCurent->stareInitiala);
        automatCurent->stari.insert(automatCurent->stareFinala);

        stivaAutomate[nivelPrioritate].push(automatCurent);
        return true;
    }

    return false;
}

AutomatLambdaNFA *creareAutomatLambdaNFA(string expresieRegExtinsa){
    // stiva initiala pentru nivelul 0
    stack <AutomatLambdaNFA *> tmp;
    stivaAutomate.push_back(tmp);

    int lim = expresieRegExtinsa.size();
    for (int i = 0; i < lim; i++){ // parcurg expresia extinsa si creez automatul
        if (!creareMiniAutomat(expresieRegExtinsa[i])) { // creez cate un sub automat care ulterior va fi unit
                                                        // cu automatul mare
            cout << "\n" << EROARE << "[creareAutomatLambdaNFA] la crearea automatului " << i << " !\n";
            exit(-1);
        }
    }

    // Automatul final va fi elementul din stiva de pe nivelul 0 (va exista doar nivelul 0, iar pe acest nivel va fi
    // doar un singur element dupa prelucrarile anterioare).
    if (stivaAutomate.empty() || stivaAutomate.size() > 1 || stivaAutomate[0].empty() || stivaAutomate[0].size() > 1){
        cout << "\n" << EROARE << "[creareAutomatLambdaNFA]! Stiva nu a fost prelucrata corespunzator!\n";
        exit(-1);
    }

    // se extrage automatul si se elimina si stiva de nivel 0
    AutomatLambdaNFA *automatFinal = stivaAutomate[0].top();
    stivaAutomate[0].pop();
    stivaAutomate.pop_back();

    automatFinal->alfabet = alfabetFinal;

    return automatFinal;
}


/** Functii pentru conversia automatului Lambda NFA intr-un automat DFA **/

set<unsigned int> calculeazaInchidereLambda(unsigned int stareSursa, vector <Tranzitie*> & tranzitii){
    set <unsigned int> inchidereLambda;
    queue <unsigned int> coadaStari;
    coadaStari.push(stareSursa);

    while(!coadaStari.empty()){
        unsigned int stareCurenta = coadaStari.front();
        coadaStari.pop();
        inchidereLambda.insert(stareCurenta);

        // pentru fiecare tranzitie
        for(auto tranzitie : tranzitii){
            // se verifica daca din starea curenta exista tranzitie lambda catre o alta stare,
            // iar daca exista si sarea destinatie nu exista deja in inchidrea lambda, aceasta se va adauga in coada.
            if( tranzitie->stareSursa == stareCurenta &&
                tranzitie->elementAlfabet == SIMBOL_LAMBDA &&
                inchidereLambda.find(tranzitie->stareDestinatie) == inchidereLambda.end()
            ){
                coadaStari.push(tranzitie->stareDestinatie);
            }
        }
    }

    return inchidereLambda;
}

set<pair<char,unsigned int>> calculeazaTranzitii(unsigned int stareSursa, vector <Tranzitie*> & tranzitii){
    set<pair<char,unsigned int>> tranzitiiStare;
    for(auto tranzitie: tranzitii){
        if(tranzitie->stareSursa == stareSursa){
            tranzitiiStare.insert(make_pair(tranzitie->elementAlfabet,tranzitie->stareDestinatie));
        }
    }
    return tranzitiiStare;
}

bool esteStareNoua(const vector<set<unsigned int>> & listaStari, const set<unsigned int> & stareNoua, unsigned int& stareEchivalenta){
    int lim = listaStari.size();
    for(int i = 0; i < lim; i++){
        if(stareNoua == listaStari[i]){
            stareEchivalenta = i;
            return false;
        }
    }
    return true;
}

AutomatDFA* conversieLaDFA(AutomatLambdaNFA* automatLambdaNfa){
    // se calculeaza inchiderea lambda pentru fiecare stare
    unordered_map<unsigned int, set<unsigned int>> hashInchidereLambda;
    for(auto stare : automatLambdaNfa->stari){
        hashInchidereLambda.insert(make_pair(stare,calculeazaInchidereLambda(stare,automatLambdaNfa->tranzitii)));
    }

    // Se calculeaza care este starea urmatoare a fiecarei stari pentru fiecare element cu care exista tranzitie.
    // Ex: presupunem starile 1, 2, 3, 4 si tranzitiile 1 a 2, 1 a 3, 1 b 4
    // atunci hash(1) = {(a,2), (a,3) (b,4)}
    unordered_map<unsigned int, set<pair<char,unsigned int>>> hashTranzitii;
    for(auto stare : automatLambdaNfa->stari){
        hashTranzitii.insert(make_pair(stare,calculeazaTranzitii(stare,automatLambdaNfa->tranzitii)));
    }

    auto* dfa = new AutomatDFA();
    dfa->alfabet = automatLambdaNfa->alfabet;
    // dfa-ul nu va contine simbolul lambda
    dfa->alfabet.erase(SIMBOL_LAMBDA);

    // Creez starea initiala. Starea initiala va fi inchiderea lambda a starii initiala a automatului lambda NFA
    dfa->stareInitiala = 0;
    dfa->stari.push_back(hashInchidereLambda.find(automatLambdaNfa->stareInitiala)->second);

    // folosesc o coada pt a afla cand nu mai am stari noi
    // primul int va fi indexul starii, iar al doilea element va fi starea
    queue <pair<unsigned int, set<unsigned int>>> coadaStariNoi;

    coadaStariNoi.push(make_pair(dfa->stareInitiala,dfa->stari[dfa->stareInitiala])); // adaug starea initiala

    while (!coadaStariNoi.empty()){ // cat timp am stari noi

        int indexStareCurenta = coadaStariNoi.front().first;
        set<unsigned int> inchidereLambdaStareCurenta = coadaStariNoi.front().second;
        coadaStariNoi.pop();

        // pt fiecare element al alfabetului, aflu tranzitiile pt starea curenta la prelucrare
        for (char x : dfa->alfabet){

            // Creez noua stare. Noua stare va fi inchiderea lambda pentru multimea
            // reuniunii starilor in care se poate ajunge cu simbolul 'x' din inchiderea lambda a starii curente.
            //
            // Ex: Presupunem ca inchiderea lambda a starii curente este {9,7,5,1,10} si se prelucreaza simbolul 'a'.
            // Prespunem ca exista tranzitiile urmatoare cu 'a':  1 a 2 , 5 a 4,  6 a 7, 11 a 12
            // Atunci noua multime intermediara pt care din starea curenta se va ajunge cu caracterul 'a'
            // va fi {2 (din 1 se ajunge in 2 cu 'a'), 4 (din 5 se ajunge in 4 cu 'a'_} , adica multimea {2,5}
            // Se va calcula inchiderea lambda pentru multimea {2,5} calculand inchidere_lambda(2) reunit cu
            // inchiderea_lambda(3), astfel ca multimea {inchidere_lambda(2), inchidere_lambda(3)} va reprezenta noua stare
            //
            // Reuniunea se va face folosind un set a.i. se vor evita automat duplicatele a.i. e de ajuns sa se adauga
            // fiecare element, nefiind necesara o alta verificare.

            // se afla starea intermediara
            set<unsigned int> multimeIntermediara;
            for (auto element : inchidereLambdaStareCurenta) {
                set<pair<char,unsigned int>> tranzitiiElement = hashTranzitii.find(element)->second;
                for(auto pereche : tranzitiiElement){
                    if(pereche.first == x){
                        multimeIntermediara.insert(pereche.second);
                    }
                }
            }

            // se face reuniunea si se calculeaza noua stare
            set<unsigned int> multimeStareNoua;
            for(auto i : multimeIntermediara){
                set<unsigned int> inchidereLambdaElement = hashInchidereLambda.find(i)->second;
                for(auto j : inchidereLambdaElement){
                    multimeStareNoua.insert(j);
                }
            }

            // daca starea nou creata are elemente si nu exista deja va fi adaugata in lista de stari
            unsigned int stareEchivalenta = 0; // verific cu ce stare este echivalenta in caz de echivalenta
            if(!multimeStareNoua.empty() && esteStareNoua(dfa->stari, multimeStareNoua, stareEchivalenta)){
                // adauga starea in dfa
                dfa->stari.push_back(multimeStareNoua);
                // adauga starea noua in coada pt prelucrare
                coadaStariNoi.push(make_pair(dfa->stari.size() - 1,multimeStareNoua));
                // va fi echivalenta cu ea insasi
                stareEchivalenta = dfa->stari.size() - 1;
            }

            // creez tranzitia si o adaug in DFA doar daca am o stare noua nevida
            if(!multimeStareNoua.empty() ){
                dfa->tranzitii.push_back(new Tranzitie(indexStareCurenta,stareEchivalenta,x));
            }
        }
    }

    // Indicele starii finale starea finala a automatului lambda NFA
    unsigned int indiceStareFinala = automatLambdaNfa->stareFinala;

    // Starile finale vor fi starile care contin indicele 'indiceStareFinala'
    int lim = dfa->stari.size();
    for (int i = 0; i < lim; i++){
        for (int idx : dfa->stari[i]){
            if (idx == indiceStareFinala){
                dfa->stariFinale.insert(i);
                break;
            }
        }
    }

    return dfa;

}


/** Functii pentru afisarea automatelor **/

void afisareAutomat(StructuraAutomat* &automat, bool afisareDfa, const string& numeFisier){
    //afisare  consola
    if(afisareDfa){
        cout << "\n===========================\nAfisare Automat - DFA\n===========================\n";
    }
    else{
        cout << "\n============================\nAfisare Automat - Lambda NFA\n============================\n";
    }

    cout << "-----------------\nNr elemente alfabet: " << automat->alfabet.size() << "\n";
    cout << "Alfabet: ";
    for (auto x : automat->alfabet) {
        cout << x << " ";
    }
    cout <<"\n----------------\n";

    cout << "Stare initiala: " << automat->stareInitiala;

    if(afisareDfa){
        cout << "\n-----------------\nNr stari finale: " << ((AutomatDFA*)automat)->stariFinale.size() << "\n";
        cout << "Stari Finale: ";
        for (auto i : ((AutomatDFA*)automat)->stariFinale){
            cout << i << " ";
        }

        cout << "\n-----------------\nNr stari: " << automat->stari.size();
        cout << "\nStari: ";
        for (int i = 0; i < ((AutomatDFA*)automat)->stari.size(); i++) {
            cout << i << " ";
        }
        cout << "\n";
    }
    else{
        cout << "\nStare finala: " << ((AutomatLambdaNFA*)automat)->stareFinala;

        cout << "\n-----------------\nNr stari: " << automat->stari.size();
        cout << "\nStari: ";
        for (auto x : automat->stari) {
            cout << x << " ";
        }
        cout << "\n";
    }

    cout <<"----------------\n";
    cout << "Numar Tranzitii: " << automat->tranzitii.size();
    cout << "\nTranzitii:\n";
    for(auto tranzitie : automat->tranzitii) {
        cout << tranzitie->stareSursa << " " << tranzitie->elementAlfabet << " " << tranzitie->stareDestinatie << "\n";
    }
    cout << "\n\n";


    //export date in fisier
    ofstream fout;
    fout.open(numeFisier);

    for (auto x : automat->alfabet) {
        fout << x << " ";
    }
    fout <<"\n";

    fout << automat->stareInitiala << "\n";

    if(afisareDfa){
        for (auto x: ((AutomatDFA*)automat)->stariFinale) {
            fout << x << " ";
        }
        fout << "\n";
        for (int i = 0; i < ((AutomatDFA*)automat)->stari.size(); i++) {
            fout << i << " ";
        }
        fout << "\n";
    }
    else{
        fout << ((AutomatLambdaNFA*)automat)->stareFinala << "\n";

        for (auto x : automat->stari) {
            fout << x << " ";
        }
        fout << "\n";
    }

    fout << automat->tranzitii.size() << "\n";
    for(auto tranzitie : automat->tranzitii) {
        fout << tranzitie->stareSursa << " " << tranzitie->elementAlfabet << " " << tranzitie->stareDestinatie << "\n";
    }
}

void ruleaza_scriptul_python_pentru_desen() {

    cout << "\n===========================================================\n";
    cout << "Se ruleaza Scriptul python pentru desen";
    cout << "\n-----------------------------------------------------------\n";
    cout << "Informatii script:";
    cout << "\n-----------------------------------------------------------\n";

    const char *comanda = "./python_env/Scripts/python.exe ./plot_automata.py";

    PROCESS_INFORMATION informatiiProcess = {nullptr};
    STARTUPINFO startupInfo                = {0};
    startupInfo.cb                         = sizeof(startupInfo);


    BOOL resultat = CreateProcess(nullptr, (LPSTR)comanda,
                                nullptr, nullptr, FALSE,
                                NORMAL_PRIORITY_CLASS,
                                GetEnvironmentStrings(), nullptr, &startupInfo, &informatiiProcess);
    if(!resultat){
        cout << EROARE << " Scriptul nu a fost rulat";
        return;
    }

    WaitForSingleObject(informatiiProcess.hProcess, INFINITE);
    cout << "\n-----------------------------------------------------------\n";
    cout << "[SUCCES] Scriptul a fost rulat";
    cout << "\n===========================================================\n";
}

int main(){

    /*
     * Input:
     *  true: daca este introdusa o expresie in forma infixata / false: daca este introdusa in forma prefixata
     *  alfabetul
     *  expresia
     * */

    string numeFisier = "date.txt";

    ifstream fin;
    fin.open(numeFisier);
    if (!fin){
        cout << "\n" << EROARE << "Deschiderea fisierului [ " << numeFisier << " ] nu a reusit!\n";
        exit(-1);
    }

    // pentru a sti daca expresia este infixata sau prefixata
    string esteExpresieInfixata;
    fin >> esteExpresieInfixata;

    string alfabet;
    fin >> alfabet;
    cout << "\nAlfabetul introdus este: " << alfabet << "\n";
    if (!alfabetValid(alfabet)){
        cout << "\n" << EROARE << "Alfabetul introdus [ " << alfabet << " ] nu este valid !\n";
        exit(-1);
    }

    string regExp;
    fin >> regExp;
    cout << "\nExpresia introdusa este: " << regExp << "\n";
    if(esteExpresieInfixata != "true"){
        cout << "\nExpresia introdusa este in forma prefixata. Se face conversia la forma infixata.\n";
        regExp = prefixToInfix(regExp);
        cout << "\nForma infixata a expresiei introduse este [ " << regExp << " ]\n";
    }
    if (!expresieValida(regExp)){
        cout << "\n" << EROARE << "Expresia introdusa [ " << regExp << " ] nu este valida !\n";
        exit(-1);
    }

    string expresieExtinsa = creareExpresieExtinsa(regExp);
    cout << "\nExpresia extinsa este: " << expresieExtinsa << "\n";

    AutomatLambdaNFA *automatLambdaNfa = creareAutomatLambdaNFA(expresieExtinsa);

    AutomatDFA *automatDfa = conversieLaDFA(automatLambdaNfa);

    afisareAutomat((StructuraAutomat*&)automatLambdaNfa,false,"lambda_nfa_out.txt");

    afisareAutomat((StructuraAutomat*&)automatDfa,true,"dfa_out.txt");

    ruleaza_scriptul_python_pentru_desen();

    return 0;
}
