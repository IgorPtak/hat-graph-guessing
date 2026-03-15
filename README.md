# Hats Project

Projekt realizuje problem "Muddy Children" oraz jego warianty, takie jak zgadywanie klru czapek na grafach ("Hat Guessing Games"). Projekt wymaga od agentów ewaluacji własnego otoczenia oraz rekurencyjnego symulowania stanów mentalnych pozostalych uczestników systemu.

Matematyczne podstawy do analizy tego typu problemów zostały zebrane w monografii Reasoning About Knowledge (Fagin, Halpern, Moses, Vardi). Autorzy pokazali, jak wykorzystać semantykę Krikego do modelowania stanów wiedzy agentó∑ oraz zdefiniowali pojędzi "wiedzy powszechnej" (Common Knowledge), która jest warunkiem koniecznym do koordynacji dzialan w systemie. Z kolei, to jak wiedza ewoluuje w czasie -- poprzez jawne deklaracje, a takze poprzez milczenie - opisuje Dynamiczna logika Epistemiczna (DEL), rozwijana m.in. przez van Ditmarscha, van der Hoeka, Kooia oraz Baltaga i Mossa. Pokazali oni, ze kazda interakcja infromacyjna stanowi operator, który zwęza przestrzeń mozliwych światów.

Wpływ samej struktury (topologii) grafu na przepływ takich informacji zbadali S. Butler i in. (2008) w kontekście gier z częściową obserwacją. Dodatkowo, rola milczenia jako asynchronicznego sposobu przekazywania informacji została sformalizowana w nowszych pracach, takich jak publikacja Gorena i Mosesa (2020).

## Sformalizowana przestrzeń stanów i model kripkego

Rozważmy system składający się z $N$ idealnie racjonalnych agentów, reprezentowanych jako zbiór wierzchołków $V = \{1, 2, \dots, N\}$ pewnego nieskierowanego grafu obserwacji $G = (V, E)$. Relacja symetryczna zadana przez zbiór krawędzi $E \subseteq V \times V$ determinuje architekturę informacji w systemie: krawędź $\{i, j\} \in E$ oznacza, że agent $i$ widzi agenta $j$, co w konsekwencji implikuje, że agent $j$ widzi agenta $i$. Definiujemy otwarte otoczenie wierzchołka $i$ w grafie $G$ jako zbiór $N(i) = \{j \in V \mid \{i, j\} \in E\}$. Zgodnie z konwencją problemów typu "Muddy Children" oraz "Hat Guessing", zakładamy brak pętli własnych w grafie, tj. dla każdego $i \in V$, $\{i, i\} \notin E$. Odzwierciedla to fundamentalne założenie o asymetrii informacyjnej: żaden agent nie może zaobserwować własnego stanu (własnego czoła lub własnej czapki).

Każdy agent $i \in V$ charakteryzuje się wewnętrznym, binarnym stanem, reprezentowanym przez zmienną $\omega_i \in \{0, 1\}$. Stan $\omega_i = 1$ interpretujemy jako posiadanie "brudnego czoła" (lub "czerwonej czapki"), natomiast $\omega_i = 0$ oznacza czoło "czyste". Globalny stan całego systemu, określany mianem "świata rzeczywistego" (actual world), można zatem opisać jako wektor alokacji $\omega = (\omega_1, \omega_2, \dots, \omega_N)$. Przestrzeń wszystkich logicznie możliwych światów początkowych, oznaczana jako $\Omega$, stanowi iloczyn kartezjański przestrzeni stanów poszczególnych agentów, tj. $\Omega = \{0, 1\}^N$. Rozmiar tej hiperkostki wynosi dokładnie $|\Omega| = 2^N$.

Aby matematycznie opisać to, co wiedzą (i czego nie wiedzą) poszczególni gracze, opieramy się na klasycznej logice epistemicznej (rozwijanej m.in. przez J. Hintikkę oraz zespół R. Fagina). Podstawowym narzędziem są tutaj tzw. struktury Kripkego, które w naszym projekcie uprościliśmy do następującego modelu:

1. Niech $\Omega$ oznacza zbiór wszystkich możliwych scenariuszy (stanów gry). W naszym przypadku pojedynczy "świat" $\omega \in \Omega$ to po prostu jeden konkretny przypadek przypisania kolorów czapek wszystkim graczom. Jeżeli mamy $N$ graczy i 2 kolory czapek (reprezentowane jako bity 0 i 1), to wszystkich możliwych światów jest $2^N$.

2. Relacja $R_i$ łączy dwa światy ($\omega$ oraz $\omega'$), jeśli dla gracza $i$ wyglądają one dokładnie tak samo.W naszej grze ograniczenia wzroku narzuca graf $G$. Gracz $i$ widzi jedynie zbiór swoich sąsiadów, oznaczany jako $N(i)$. Zatem dwa różne światy (układy czapek) są dla niego nierozróżnialne wtedy i tylko wtedy, gdy wszyscy jego sąsiedzi mają w obu wariantach identyczne kolory czapek.Formalnie zapisujemy ten warunek następująco:$$(\omega, \omega') \in R_i \iff \forall j \in N(i), \; \omega_j = \omega'_j.$$

Powyższy wzór to matematyczny fundament pierwszej fazy naszego algorytmu. Oznacza on, że w chwili startowej ($t=0$) gracz filtruje całą przestrzeń $\Omega$ ($2^N$ wariantów) i pozostawia na swojej prywatnej liście tylko te scenariusze, które są w 100% zgodne z barwami czapek obserwowanymi u sąsiadów. Wartości na pozostałych węzłach grafu (w tym na jego własnej głowie) pozostają dla niego niewiadomą, co zmusza go do rozważania wielu potencjalnych "światów" jednocześnie.

Oznacza to, że z perspektywy gracza $i$, dwa scenariusze ($\omega$ oraz $\omega'$) są nie do odróżnienia, jeśli kolory czapek zgadzają się u wszystkich jego sąsiadów zdefiniowanych przez zbiór $N(i)$.

Wynikają z tego trzy bardzo ważne, matematyczne i algorytmiczne właściwości naszego modelu:

1.  Niewiedza o własnym stanie i odległych węzłach: Ponieważ gracz nie widzi samego siebie ($i \notin N(i)$), w rozważanych przez niego światach jego własny kolor $\omega_i$ może przyjmować dowolną wartość (0 lub 1). Dokładnie to samo dotyczy wszystkich graczy znajdujących się poza jego zasięgiem wzroku, czyli wierzchołków $k \notin N(i) \cup \{i\}$. W ten naturalny sposób modelujemy brak informacji o odległych częściach grafu.
2.  Relacja równoważności i logika S5: Relacja $R_i$, oparta na identyczności widocznych stanów, jest z definicji zwrotna, symetryczna i przechodnia. Stanowi więc relację równoważności. Oznacza to, że dzieli ona całą przestrzeń wszystkich $2^N$ możliwych światów na rozłączne podzbiory, zwane klasami abstrakcji. Z perspektywy teorii systemów wieloagentowych implikuje to, że wnioskowanie agentów opisane jest klasyczną logiką modalną S5. W praktyce gwarantuje to pełną introspekcję: algorytm każdego gracza "wie, co wie" oraz "wie, czego nie wie".
3.  Rozmiar przestrzeni poszukiwań: Z ilu wariantów składa się taka pojedyncza klasa abstrakcji (czyli ile światów bierze pod uwagę dany gracz)? Rozmiar ten wynosi dokładnie $2^{N - |N(i)|}$. Wynika to wprost z kombinatoryki: gracz nie widzi dokładnie $N - |N(i)|$ osób (włączając w to siebie), a każda z nich może posiadać jeden z dwóch kolorów czapek.

Zbiór wszystkich tych wariantów, które gracz $i$ uważa za matematycznie możliwe, gdy w rzeczywistości gra toczy się w świecie $\omega$, oznaczamy formalnie jako $\mathcal{K}_i(\omega)$:$$\mathcal{K}_i(\omega) = \{ \omega' \in \Omega \mid (\omega, \omega') \in R_i \}$$To właśnie ten zbiór $\mathcal{K}_i(\omega)$ będzie punktem startowym ($t=0$) w głównej pętli symulacyjnej naszego programu, a każda kolejna tura będzie polegała na jego stopniowym pomniejszaniu.

| Komponent Modelu                          | Matematyczna Definicja                                    | Interpretacja Epistemiczna                                                 |
| :---------------------------------------- | :-------------------------------------------------------- | :------------------------------------------------------------------------- |
| Przestrzeń $\Omega$                       | $\{0,1\}^N$                                               | Zbiór wszystkich możliwych konfiguracji zabrudzeń/czapek.                  |
| Zmienna $p_i$                             | $p_i \in \pi(\omega) \iff \omega_i = 1$                   | Fakt "Agent $i$ ma czerwoną czapkę/brudne czoło".                          |
| Relacja $R_i$                             | $\forall j \in N(i), \omega_j = \omega'_j$                | Kryterium nierozróżnialności dwóch światów przez agenta $i$.               |
| Stan Epistemiczny $\mathcal{K}_i(\omega)$ | $\{ \omega' \in \Omega \mid (\omega, \omega') \in R_i \}$ | Zbiór światów, które agent $i$ uważa za możliwe, będąc w świecie $\omega$. |

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Testing

```bash
./test_graph
```

## Features

- Graph representation using bitmasks (up to 64 vertices).
- Zero-allocation hot paths.
- C++17.
