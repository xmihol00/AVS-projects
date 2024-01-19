#### Stručně odpovězte na následující otázky: ######


1. Byla "Line" vektorizace časově efektivní? Proč?
==============================================================================
Ano byla.

Vektorizace by měla teoreticky pro AVX-512 instrukce pracující s vektory s 32 bitovými 
prvky (všechny operandy jsou v Line implementaci převedeny na 32 bitů) zrychlit výpočet 
až 16x. Tohoto teoretického zrychlení ale nelze prakticky dosáhnout z důvodu propustnosti 
paměti, snížení frekvence procesoru atd. V našem případě je toto ideální zrychlení dále 
omezeno nutností redukovat výsledky podmínky testující hranici divergence a tedy 
nemožností ukončit výpočet pro jednotlivé prvky. Výpočet lze ukončit, až když všechny 
prvky v řádku divergují. Toto je zejména výpočetně neefektivní, pokud jen velmi malý 
počet prvků v řádku nediverguje (z většiny se počítají nepotřebné hodnoty).

Pokud ale uvážíme, že zhruba pro první pětinu řádků (z poloviny výšky) dochází 
k divergenci všech prvků poměrně brzy, výpočet této části bude pravděpodobně významně 
urychlen. Ve zbylých čtyřech pětinách řádků se vždy bude počítat do dosažení 
stanoveného limitu (vždy je zde aspoň jeden prvek, který nediverguje). Nicméně těchto 
nedivergujících prvků je v této části zhruba jedna pětina, takže i když se výpočet 
vektorizací zrychlí pouze 5x, bude pořád efektivnější než výpočet bez vektorizace 
(u jedné pětiny prvků bez vektorizace se stejně bude počítat až do dosažení stanoveného 
limitu). Dále u vektorizované verze dojde řádově k 'width' méně špatných predikcí skoku 
dosažení divergence (skáče se pro celý řádek), což bude mít další významný dopad na 
zrychlení.

V neposlední řadě bude výpočet urychlen počítáním jen poloviny zadané výšky a 
zrcadlovým zkopírováním výsledků do druhé poloviny. Takto by ale mohla být urychlena 
i nevektorizovaná verze.


2. Jaká byla dosažena výkonnost v Intel Advisoru pro jednotlivé implementace 
(v GFLOPS)?
==============================================================================
Uvádím GFLOPS jen pro výpočetní jádra. Spojení '_mm512 verze' znamená, že byly 
v implementaci použity funkce _mm512* z immintrin.h překládané přímo na AVX-512 
instrukce.

Ref:   
* 0.805 GFLOPS

Line:  
* neoptimalizovaná verze: 49.919 GFLOPS, doba běhu 1023 ms pro s=4096, limit=100 a 
       8702 ms pro s=4096, limit=1000,
* optimalizovaná verze obdelnikem (viz konec souboru): průměrně 46.623 GFLOPS, doba
       běhu 896 ms pro s=4096, limit=100 a 7489 ms pro s=4096, limit=1000,

Batch:
* neoptimalizovaná verze: 82.327 GFLOPS, doba běhu 389 ms pro s=4096, limit=100 a 
       2694 ms pro s=4096, limit=1000,
* neoptimalizovaná _mm512 verze (viz sekce níže): 45.435 GFLOPS, doba běhu 260 ms pro 
       s=4096, limit=100 a 2058 ms pro s=4096, limit=1000,
* optimalizovaná _mm512 verze kružnicí (viz konec souboru): 38.061 GFLOPS, doba běhu 
       137 ms pro s=4096, limit=100 a 774 ms pro s=4096, limit=1000,
* optimalizovaná _mm512 verze elipsou a obdelniky (viz konec souboru): 48.491 GFLOPS, 
       doba běhu 110 ms pro s=4096, limit=100 a 546 ms pro s=4096, limit=1000.

Zajímavé je, že 'neoptimalizovaná' Batch implementace s velikostí dávky 64, ačkoliv 
dosahuje významně lepších GFLOPS, je pomalejší, než 'neoptimalizovaná _mm512' Batch 
implementace, která počítá prakticky to stejné akorát s velikostí dávky 16. Plyne z toho,
že pro dávku o velikosti 64 nelze vygenerovat tak efektivní kód, případně že kompilátoru
nemůže provést některé optimalizace, které lze provést ručně se znalostí počítaného 
problému. Výsledky z Intel Advisor si lze prohlédnout na: 
https://github.com/xmihol00/mandelbrot_set_optimization/tree/main/intel_advisor


3. Jaká část kódu byla vektorizována v "Line" implementaci? Vyčteme tuto 
informaci i u batch kalkulátoru?
==============================================================================
U obou implementací (u Batch pouze základní verze bez _mm512* funkcí) byly vektorizovany
smyčky, nad kterými jsem použil direktivy překladače pro vektorizaci. Zejména jde o 
nejvnitřnější smyčky, tj. výpočet následujícího 'c' pro celý řádek u Line implementace a 
pro část řádku u Batch implementace. Pro dosažení vektorizace používající maximální 
velikost single precision floating point vektoru (16 prvků) bylo ještě nutné explicitně 
přidat direktivu 'simdlen(16)'. Kromě hlavních výpočetních jader jsem požadoval 
vektorizaci i smyček zajišťující inicializaci paměti a zrcadlové kopírování výsledků 
z první poloviny matice do druhé. Smyčka inicializující paměť byla vektorizována se 100% 
efektivitou, naopak vektorizace kopírování paměti nevedla k žádnému zlepšení. To bude 
dáno tím, že tato funkce není ve standardní knihovně přeložena vektorizovaně. Její 
implementace pravděpodobně kopíruje paměť za využití DMA kontroleru, čili bez zátěže CPU.
         

4. Co vyčteme z Roofline modelu pro obě vektorizované implementace?
==============================================================================
Neoptimalizovaná Line: 
Je omezená propustností paměti, bod reprezentující výpočetní jádro je zhruba ve středu 
Roofline modelu (pozor, ten není linearní, ale logaritmický, takže zkresluje) a zejména
mezi linkami, které znázorňují maximální propustnost L2 cache a L3 cache, což je dáno
tím, že se celý řádek nevleze do L2 cache.

Neoptimalizovaná Batch:
Ačkoliv u této implementace leží bod reprezentující výpočetní jádro prakticky přímo na
lince reprezentující propustnost L2 cache, řekl bych, že tato implementace je spíše
omezená výpočetním výkonem. To z toho důvodu, že jsem zvolil velikost dávky o 64 prvcích,
což je množství dat (64 * 4 * 3 (reálná část, imaginární část a výsledek) = 768 B), které 
se jistě vleze celý do L1 cache i s možností přednačíst data následující dávky.  


==============================================================================
Popis optimalizací:

Hlavní optimalizace vychází ze článku na Wikipedia 
(https://cs.wikipedia.org/wiki/Mandelbrotova_množina#Výpočet_a_grafické_zobrazení),
zejména z části: "Výpočet je možno zrychlit tím, že se rychle detekují body, které do
množiny evidentně patří, protože se nacházejí uvnitř hlavních částí množiny – kružnice 
a kardioidy." 

U Line implementace jsem se rozhodl pro aproximaci kardiody obdélníkem, protože krajní
hodnoty lze jednoduše vypočítat z šířky matice a následně řádky rozdělit na dvě části.
Vizualizace části matice, která lze takto přeskočit si lze prohlédnout na:
https://github.com/xmihol00/mandelbrot_set_optimization/blob/main/rectangle_optimization.md

U Batch implementace jsem nejdříve aproximoval kružnici i kardiodu kružnicemi. Tato
aproximace testuje každou dávku, jestli je mimo obě kružnice (nepotřebuje tedy počítat
odmocninu). Vizualizace části matice, která lze takto přeskočit si lze prohlédnout na:
https://github.com/xmihol00/mandelbrot_set_optimization/blob/main/circle_optimization.md

Jako druhou optimalizaci u Batch implementace jsem zvolil aproximaci obdelniky a 
elipsou. U této implementace navíc pro každý řádek určuji hranice elipsy za použití 
aproximace druhé odmocniny popsané na Wikipedia 
(https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_estimates). 
v kódu pak jde o část:
```c
int elipseEquation = elipseRadiusSquared - 
                     (((i - size) * (i - size) * elipseMultiply) >> elipseShift);

// approximation of sqrt(elipseEquation)
int msb = ((32 - __builtin_clz(elipseEquation)) >> 1);
int elipseEquationSqrt = (1 << (msb - 1)) + (elipseEquation >> (msb + 1));

int elipseStart = (elipseCenter - elipseEquationSqrt) & (~15); // align to 16
int elipseEnd = (elipseCenter + elipseEquationSqrt) & (~15);   // align to 16
int elipseWidth = elipseEnd - elipseStart;
```
Vizualizace části matice, která lze takto přeskočit si lze prohlédnout na:
https://github.com/xmihol00/mandelbrot_set_optimization/blob/main/elipse_optimization.md

Dalšího nezanedbatelného zrychlení u Batch implementace lze dosáhnout zapisováním 
výsledků jednotlivých dávek rovnou na dvě místa v paměti. Na konci výpočtu pak není 
nutné kopírovat půlku vypočtených dát.

Dále lze výpočet nepatrně zrychlit postupným snižováním počtů testů na divergenci 
s rostoucím počtem iterací nejvíce zanořené smyčky. S rostoucím počtem iterací je méně
pravděpodobné, že k divergenci dojde před stanoveným limitem. Kompilátor pak dokáže 
rozvinout smyčky a snížit tak počet instrukcí a míst se skokem (i když jen minimálně). 

Poznámka na závěr – Batch implementace nevyžaduje vůbec provádět redukci, protože AVX-512
instrukce umožňují přímo otestovat divergenci všech prvků dávky najednou a výsledek 
zapsat do 16 bitové proměnné, která pak lze porovnat s nulou.
