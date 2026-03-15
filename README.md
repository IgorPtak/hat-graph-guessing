# Hats Project

Niniejszy dokument stanowi kompleksowe studium teoretyczne mechanizmów dedukcyjnych zaimplementowanych w projekcie `hats`. Projekt ten bada ewolucję wiedzy w systemach wieloagentowych na grafach częściowej obserwacji, opierając się na Dynamicznej Logice Epistemicznej (DEL).

---

## 1. Architektura Stanów: Przestrzeń Możliwych Światów i Semantyka Kripkego

Podstawą symulacji jest modelowanie niepewności agentów za pomocą **struktur Kripkego**.

### 1.1. Definicja Świata ($\omega$)

Globalny stan systemu (świat rzeczywisty) to wektor binarny $\omega = (\omega_1, \omega_2, \dots, \omega_N)$, gdzie $\omega_i \in \{0, 1\}$. Przestrzeń wszystkich logicznie dopuszczalnych światów $\Omega = \{0, 1\}^N$ tworzy hiperkostkę o rozmiarze $2^N$.

### 1.2. Relacja Nierozróżnialności ($R_i$)

Topologia grafu $G=(V, E)$ definiuje relację równoważności $R_i$ dla każdego agenta $i$. Relacja ta łączy światy, które z perspektywy agenta $i$ są identyczne:
$$(\omega, \omega') \in R_i \iff \forall j \in N(i), \omega_j = \omega'_j$$
Relacja $R_i$ indukuje partycję (podział) zbioru $\Omega$ na rozłączne klasy abstrakcji $[\omega]_i$. Z perspektywy agenta $i$ znajdującego się w świecie $\omega$, zbiór światów możliwych to $\mathcal{K}_i(\omega) = \{ \omega' \in \Omega \mid (\omega, \omega') \in R_i \}$.

| Komponent Modelu                          | Matematyczna Definicja                                    | Interpretacja Epistemiczna                                                 |
| :---------------------------------------- | :-------------------------------------------------------- | :------------------------------------------------------------------------- |
| Przestrzeń $\Omega$                       | $\{0,1\}^N$                                               | Zbiór wszystkich możliwych konfiguracji zabrudzeń/czapek.                  |
| Zmienna $p_i$                             | $p_i \in \pi(\omega) \iff \omega_i = 1$                   | Fakt "Agent $i$ ma czerwoną czapkę/brudne czoło".                          |
| Relacja $R_i$                             | $\forall j \in N(i), \omega_j = \omega'_j$                | Kryterium nierozróżnialności dwóch światów przez agenta $i$.               |
| Stan Epistemiczny $\mathcal{K}_i(\omega)$ | $\{ \omega' \in \Omega \mid (\omega, \omega') \in R_i \}$ | Zbiór światów, które agent $i$ uważa za możliwe, będąc w świecie $\omega$. |

---

## 2. Dynamika Wiedzy: Rekurencyjna Redukcja Modelu

Ewolucja wiedzy następuje poprzez operatory ogłoszeń publicznych (Public Announcement Logic).

### 2.1. Inicjalizacja: Ogłoszenie $\phi_0$

Początkowe ogłoszenie $\phi_0 = \bigvee_{j \in V} p_j$ ("Istnieje co najmniej jedna czerwona czapka") usuwa z modelu świat zerowy $\mathbf{0}$. Od tego momentu $\Omega_0 = \Omega \setminus \{\mathbf{0}\}$.

**Ważne założenie:** Przyjmujemy, że rzeczywisty wektor stanu $\omega^*$ nie jest wektorem zerowym ($\omega^* \neq \mathbf{0}$). W przeciwnym razie początkowe ogłoszenie publiczne $\phi_0$ byłoby fałszywe, co w logice PAL prowadzi do załamania modelu (brak światów możliwych).

### 2.2. Operator Milczenia (Silence)

Niech $\sigma_t$ będzie formułą stwierdzającą, że w rundzie $t$ żaden agent nie zna swojego koloru:
$$\sigma_t = \bigwedge_{j \in V} \neg (K_j p_j \lor K_j \neg p_j)$$
Publiczna obserwacja faktu, że $\sigma_t$ jest prawdziwe, prowadzi do redukcji modelu:
$$\Omega_{t+1} = \{ \omega \in \Omega_t \mid \Omega_t, \omega \models \sigma_t \}$$
Agent $i$ aktualizuje swój zbiór światów możliwych: $\mathcal{K}_i(t+1) = \mathcal{K}_i(t) \cap \Omega_{t+1}$.

---

## 3. Analiza Topologii: Szkice Dowodów

### 3.1. Graf Pełny ($K_N$): Dowód Indukcyjny

**Twierdzenie:** W grafie pełnym, jeśli obiektywnie istnieje $k$ czerwonych czapek, to agenci z czerwonymi czapkami zgadną swój kolor dokładnie w rundzie $T=k-1$.

**Szkic dowodu (Indukcja po $k$):**

1.  **Baza ($k=1$):** Agent $i$ widzi same zera. $\mathcal{K}_i(0) = \{\omega^*, \mathbf{0}\}$. Po $\phi_0$ zostaje tylko $\omega^*$. Agent zgaduje w $t=0$.
2.  **Krok indukcyjny:** Załóżmy, że dla $k$ czapek agenci zgadują w $k-1$. Przy $k+1$ czapkach, agent $i$ widzi $k$ czerwonych czapek. Gdyby sam miał białą, ktoś musiałby zgadnąć w $k-1$. Milczenie w $k-1$ eliminuje światy z $k$ czapkami. Zostaje tylko świat z $k+1$.

### 3.2. Cykl ($C_N$): Lemat o Horyzoncie Epistemicznym

**Twierdzenie:** Prędkość propagacji informacji o braku wiedzy w grafie jest ograniczona przez odległość geodezyjną $d(i, j)$.

**Szkic dowodu:** Redukcja $\mathcal{K}_i$ zależy od wiedzy sąsiadów. Aby milczenie odległego agenta $z$ wpłynęło na $i$, musi dojść do sekwencyjnej redukcji klas abstrakcji wzdłuż ścieżki. Każdy krok redukcji wymaga jednej rundy milczenia ($\sigma$). Stąd $t=d(i, z)$.

### 3.3. Gwiazda ($S_N$): Dowód Impasu (Deadlock)

**Twierdzenie:** Jeśli centrum $c$ ma czapkę białą ($\omega_c=0$), a co najmniej dwa liście $l_1, l_2$ mają czapki czerwone, dedukcja liści zatrzymuje się w rundzie $t=1$.

**Szkic dowodu:** Liść $l_1$ widzi tylko $c=0$. Milczenie centrum w $t=0$ nie pozwala mu odróżnić świata, w którym on sam ma 1, od świata, w którym ma 0 (bo w obu światach centrum widzi czerwoną czapkę – albo u $l_1$, albo u $l_2$). Klasa abstrakcji liścia nie ulega redukcji.

---

## 4. Wyzwania Obliczeniowe: PSPACE-Zupełność i Optymalizacje

Problem sprawdzania wiedzy na grafach jest fundamentalnie trudny (Succinct Model Checking).

### 4.1. Złożoność Algorytmiczna

Problem weryfikacji wiedzy odpowiada ewaluacji zagnieżdżonych kwantyfikatorów, co plasuje go w klasie PSPACE-complete. Rozmiar modelu rośnie wykładniczo ($2^N$).

### 4.2. Wyzwania Implementacyjne i Wektoryzacja (SIMD)

Aby sprostać wymaganiom wydajnościowym (bliskim standardom High-Frequency Trading), projekt wykorzystuje zaawansowane techniki przetwarzania równoległego:

- **Wektoryzacja SIMD**: Operacje na zbiorach światów (`intersect_with`, `count`) są zaimplementowane z wykorzystaniem instrukcji procesora (AVX2 / AVX-512). Dzięki zastosowaniu rejestrów takich jak `__m256i` i instrukcji typu `_mm256_and_si256`, możliwe jest przetwarzanie 256 bitów w jednym cyklu zegara.
- **Złożoność**: Dzięki wektoryzacji, bazowa złożoność operacji na zbiorze światów zostaje zredukowana z $O(2^N)$ do $O(2^N / 256)$, co pozwala na symulację systemów do $N=20$ w czasie rzeczywistym.
- **Cache Locality**: Struktura `WorldSet` gwarantuje zwartość danych w pamięci, co minimalizuje kosztowne chybienia pamięci podręcznej (cache misses).

---

## 5. Podsumowanie i Wnioski

Symulacja potwierdza, że w systemach rozproszonych **topologia jest epistemologią**.

1.  **Gęstość krawędzi**: W grafach gęstych ($K_N$) wiedza powszechna buduje się przez zliczanie rund.
2.  **Dystans fizyczny**: W grafach rzadkich ($C_N$) czas jest zdominowany przez geometrię sieci.
3.  **Asymetria ról i Pasożytnictwo Informacyjne**: W grafach asymetrycznych (jak $S_N$) występuje zjawisko pasożytnictwa informacyjnego. Centrum grafu błyskawicznie buduje swoją wiedzę kosztem milczenia liści, jednak samo nie jest w stanie wygenerować milczenia, które zwrotnie pomogłoby liściom. Brak krawędzi między liśćmi o tym samym poziomie wiedzy prowadzi do trwałej pułapki informacyjnej.

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
