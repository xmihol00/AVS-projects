Architektury Výpočetních Systémů (AVS 2023)
Projekt č. 2 (PMC)
Login: xmihol00

Úloha 1: Paralelizace původního řešení
===============================================================================

1) Kterou ze smyček (viz zadání) je vhodnější paralelizovat a co způsobuje 
   neefektivitu paralelizaci té druhé?
Nejvhodnější je paralelizovat pouze vnější smyčku, tj. smyčku ve funkci marchCubes. Režie spojená
s vytvářením a spouštěním vláken, následným čekáním na bariéře a destrukcí vláken je takto 
minimální, protože smyčka se provádí pouze jednou. 

Naopak nejhorší, a to významně i při srovnání s referenční sekvenční implementací, je paralelizovat 
pouze vnitřní smyčku ve funkci evaluateFieldAt. Takto dochází k maximální výše popsané režii 
(smyčka je volána mnohonásobně), která daleko převyšuje užitečný výpočet. Navíc v případě 
paralelizace pouze vnitřní smyčky zůstává poměrně nezanedbatelná část výpočtu sekvenční, o čemž
víme, že zhoršuje škálování.

DALŠÍ OTESTOVANÉ MOŽNOSTI:
Při vytvoření vláken již ve funkci marchCubes (vnější smyčka je volána s direktivou omp single), 
ale paralelizaci až ve funkci evaluateFieldAt není třeba vlákna pokaždé opět vytvářet, režie je tak 
menší, ale pořád velká natolik, že i toto řešení je významně pomalejší ve srovnání s referenční 
sekvenční implementací. Navíc i toto řešení obsahuje sekvenční části.

Nakonec použití vnořené paralelizace, tj. paralelizace obou smyček, je sice pomalejší než 
paralelizace pouze vnější smyčky, ale tentokrát aspoň rychlejší než referenční sekvenční 
implementace. Zde je sice stále velká režie při práci s vlákny, ale aspoň celá část výpočtu běží 
paralelně a režie je částečně kompenzována tím, že na jednom fyzickém jádře CPU se může střídat 
více logických vláken, když některé např. čeká na přístup do kritické sekce pro zápis do sdílené 
paměti.

POZNÁMKA:
Dalším důvodem, proč neparalelizovat druhou smyčku, je, že tato smyčka lze velmi dobře vektorizovat 
po "transponování" parametrického skalárního pole z AoS na SoA (samozřejmě ideální by bylo takto 
data načítat rovnou ze souboru), čili zajištěním jednotkových rozestupů elementů v paměti, a tím 
dále významně zrychlit výpočet. Tento způsob je implementován v Loop i Octree řešení.


2) Jaké plánování (rozdělení práce mezi vlákna) jste zvolili a proč? 
   Jaký vliv má velikost "chunk" při dynamickém plánování (8, 16, 32, 64)?
Ze znalosti implementace je zřejmé, že délky vyhodnocení jednotlivých krychlí se budou lišit 
v generování a zápise trojúhelníků do sdílené paměti (pro prázdné krychle bude délka vyhodnocení 
nejkratší, pro krychle s pěti trojúhelníky nejdelší). Generování a zápis trojúhelníků není sice 
výpočetně náročný ve srovnání s výpočtem funkce evaluateFieldAt, ale ještě v kombinaci s případným 
přístupem do kritické sekce a potenciálně rozdílnou dobou přístupu do paměti z různých vláken (vliv 
NUMA) nemusí být ani zanedbatelný. To znamená, že statické plánování pravděpodobně nebude dobrá 
volba. Na druhou stranu optimální asi také nebude dynamické plánování s velmi malým chunk size, 
protože rozdíly v délkách iterací budou malé a režie spojená s dynamickým plánováním by zbytečně 
zpomalovala výpočet. Dobrým kompromisem by tedy mohlo být guided plánování, u kterého se postupně 
zmenšuje chunk size, což umožňuje práci postupně efektivně distribuovat mezi vlákna se zachováním 
malé režie. 

Pro výběr optimálního plánování byla provedena následující analýza (obdobně použita také pro 
ostatní analýzy níže):
a) Pro testovací soubory bun_zipper a dragon_vrip byly změřeno 10 běhů pro každou kombinaci 
   plánování (static, dynamic, guided) a velikosti chunk (8, 16, 32, 64) a pro velikosti grid 64
   a 128.
b) Pro každý soubor, plánovač, velikost chunk a velikost grid byly vypočteny průměrné hodnoty 
   naměřených délek běhů, aby se minimalizoval vliv případných outliers.
c) Průměrné hodnoty pak byly poděleny příslušnou maximální hodnotou pro daný soubor a velikost 
   grid, aby se získaly relativní hodnoty a rozdílné délky výpočtů pro různé soubory nezkreslovaly 
   výsledky.
d) Pro každý plánovač a velikost chunk byly sečteny relativní hodnoty délek výpočtů pro všechny 
   soubory a obě velikosti grid.
e) Výsledné součty byly poděleny maximálním součtem a seřazeny sestupně.

Pro výchozí implementaci používající sdílenou paměť a kritickou sekci jsou výsledky následující:
Plánovač            Relativní průměrná délka běhu
dynamic_8                                0.958385
dynamic_16                               0.958459
dynamic_32                               0.958817
dynamic_64                               0.959500
dynamic_default                          0.962376
guided_default                           0.963360
guided_8                                 0.964481
guided_16                                0.964822
guided_32                                0.965371
guided_64                                0.966384
static_default                           0.992834
static_16                                0.993677
static_64                                0.995118
static_8                                 0.996425
static_32                                1.000000
Graficky pak znázorněno viz: https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/schedulers/relative_averaged_schedulers_default.png
Dle naměřených hodnot je nejlepší dynamické plánovaní s nejmenší velikostí chunk, což neodpovídá 
úvaze výše. Z toho lze usoudit, že generování a zejména zápis trojúhelníků do sdílené paměti 
(vliv NUMA, koherentní výpadky cache, přístup do kritické sekce) je natolik náročný, že se délky 
jednotlivých iterací liší dostatečně, aby se vyplatila i poměrně velká režie spojená s dynamickým 
plánováním s poměrně malým chunk size.

Stejná analýza byla také provedena pro implementaci nepoužívající sdílenou paměť a kritickou sekci 
(separate_memory_vectorized). Implementace je blíže popsána v otázce 3). Výsledky jsou následující:
Plánovač            Relativní průměrná délka běhu
guided_32                                0.848866
guided_64                                0.849618
guided_8                                 0.858723
guided_16                                0.860047
guided_default                           0.861195
dynamic_64                               0.861449
dynamic_32                               0.863120
dynamic_16                               0.871664
dynamic_8                                0.875464
dynamic_default                          0.888595
static_64                                0.940720
static_default                           0.943351
static_16                                0.953011
static_8                                 0.959202
static_32                                1.000000
Graficky pak znázorněno viz: https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/schedulers/relative_averaged_schedulers_optimized.png
Odstraněním sdílené paměti a s tím spojených problémů již výsledky odpovídají očekávání. Ve 
výsledném programu tedy zvolíme guided plánování s velikostí chunk 32.

U dynamického plánování je s rostoucí velikostí chunk plánování více statické, tj. k dynamickému 
přidělování práce vláknům dochází méně často a vlákna počítají delší dobu. S rostoucí velikostí 
chunk se tak snižuje režie spojená s dynamickým plánováním, ale naopak se zvyšuje pravděpodobnost, 
že některá vlákna budou již mít výpočet hotov a nebude pro ně k dispozici další práce. Menší 
velikost chunk je tak výhodnější pokud se délky jednotlivých iterací liší poměrně významně. Naopak 
věší velikost chunk je pak lepší pro případy, kdy se délky iterací příliš neliší. Toto lze i krásně 
pozorovat v naměřených hodnotách, kdy pro výchozí implementaci (první tabulka) je nejlepší velikost 
chunk 8, ale pro optimalizovanou implementaci je nejlepší velikost chunk 64 (druhá tabulka).


3) Jakým způsobem zajišťujete ukládání trojúhelníků z několika vláken současně?
Otestoval jsem následující možnosti ukládání trojúhelníků:
a) Použití sdílené paměti a kritické sekce - jedná se prakticky o referenční řešení s tím rozdílem, 
že nad operací push back do standardního C++ vektoru je přidána direktiva překladače omp critical. 
Implementace je tedy velmi jednoduchá, ale pro větší počty vláken neefektivní, kvůli jejich 
soupeření o přístup do kritické sekce, kdy vlákna čekající na přístup a neprovádí žádný užitečný 
výpočet.

b) Použití sdílené paměti a atomické operace "capture and add" - při tomto řešení je sdílená paměť 
alokována dopředu a vlákna zapisují do této paměti pomocí indexu, který si atomicky zkopírují a 
zvýší o jedna. Toto řešení sice eliminuje kritickou sekci a synchronizaci řeší rychlejším způsobem, 
nic méně pořád zde zůstává problém s koherentními výpadky cache a také s tím, že několik po sobě 
jdoucích zápisů potenciálně ze všech různých vláken bude vždy do jedné stránky v hlavní paměti, což 
povede na přetížení řadiče paměti, který má tuto stránku pod správou.

c) Použití oddělených pamětí a následné kopie výsledků do jedné spojité paměti - toto řešení 
nepoužívá sdílenou paměť, ale každé vlákno si alokuje vlastní paměť. Při ukládání trojúhelníků tak 
není potřeba vlákna synchronizovat, nedochází ke koherentním výpadkům cache a vlákna zapisují do 
stránek v paměti, která jsou jim nejblíže (first touch memory allocation). Jediný problém tohoto 
řešení je nutnost na konci výpočtu všechny výsledky zkopírovat do jedné spojité paměti. To je 
vyřešeno za pomocí omp tasks, kdy se při jejich plánování počítá offset do této spojité paměti a 
následně může kopírování probíhat paralelně. Ke kopírování by ale nemuselo vůbec docházet, pokud by 
se výsledky z různých pamětí rovnou zapisovaly na disk, což je nutné stejně dělat sekvenčně a 
jelikož disk je výrazně pomalejší než paměť, přenosy ze vzdálených pamětí by nebyly problematické.

Pro různé varianty ukládání trojúhelníků do paměti a pro vektorizaci byla provedena 
obdobná analýza jako v otázce 2). Výsledky jsou následující:
Loop implementace               Relativní průměrná délka běhu
separate_memory_vectorized                           0.305848
capture_and_add_vectorized                           0.417675
critical_section_vectorized                          0.439149
capture_and_add                                      0.985852
separate_memory                                      0.991271
critical_section                                     1.000000
Graficky pak znázorněno viz: https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/loop_performance/relative_bar.png
                             https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/loop_performance/relative_plot.png
Zřejmě použití oddělené paměti a vektorizace je nejrychlejší, i přes nutnost na začátku výpočtu 
kopírovat parametrické skalární pole a na konci výpočtu kopírovat vygenerované trojúhelníky. Při 
odstranění těchto nedostatků, jak bylo popsáno výše, by pak toto řešení bylo nejrychlejší.

Úloha 2: Paralelní průchod stromem
===============================================================================

1) Stručně popište použití OpenMP tasků ve vašem řešení.
V každém uzlu stromu dochází k osmi větvení (pokud uzel splňuje podmínku opsané koule 
a je dostatečně vzdálen od listové úrovně). Každé toto větvení realizované rekurzivním 
voláním funkce procházející octree lze jednoduše naplánovat jako OpenMP task, který následně
atomicky přičte svůj výsledek do sdílené proměnné. Naplánované tasks jsou pak rozebírány 
dostupnými vlákny, což umožňuje jednoduchou paralelizaci jinak poměrně obtížně 
paralelizovatelného problému.


2) Jaký vliv má na vaše řešení tzv. "cut-off"? Je vhodné vytvářet nový 
   task pro každou krychli na nejnižší úrovni?
Cut-off je hodnota, která v mém řešení určuje vzdálenost od listové úrovně, tj. od krychlí 
s hranou o velikosti 1, od které již nedochází k dalšímu paralelnímu průchodu stromem. Zbytek 
podstromu se pak stejným způsobem prochází v rámci jednoho task sekvenčně až po listovou úroveň, 
kde dochází ke generování trojúhelníků. 

Vytváření nového task pro každou krychli až po nejnižší úroveň spíše není vhodné, protože takto 
každý task obsahuje pouze poměrně malé množství práce (v tomto případě jedno vyhodnocení funkce 
evaluateFieldAt, v jiných případech ale může být průchod i jednodušší) a dochází tak k relativně 
velké režii spojené s tvorbou a spuštěním nových tasks. Se zvětšováním cut-off se pak úloha stává 
méně dynamickou, stejně jak je to u dynamického plánování a velikostí chunk, jak je vysvětleno 
výše. Opět je nutné najít kompromis v souvislosti s variabilitou délek výpočtů podstromů daných 
volbou cut-off (podobný problém jako různé délky výpočtů iterací smyčky a volba velikosti chunk), 
velikostí vstupu a počtem dostupných vláken. 

Experimentálně byly naměřeny pro různé velikosti grid tyto cut-off jako nejlepší pro 36 vláken:
cut-off   Relativní průměrná délka běhu - grid 64    cut-off   Relativní průměrná délka běhu - grid 128
8                                        0.990515    1                                         0.994513
2                                        0.991400    2                                         0.996009
1                                        0.991796    32                                        0.996129
32                                       0.991966    8                                         0.996353
4                                        0.994872    16                                        0.998326
16                                       1.000000    4                                         1.000000

cut-off   Relativní průměrná délka běhu - grid 256    cut-off   Relativní průměrná délka běhu - grid 512
8                                         0.994359    4                                         0.992943
16                                        0.996311    8                                         0.992945
32                                        0.996813    1                                         0.994124
1                                         0.998300    32                                        0.996843
4                                         0.999015    2                                         0.997793
2                                         1.000000    16                                        1.000000

cut-off   Relativní průměrná délka běhu - průměr
8                                       0.995149
1                                       0.996288
32                                      0.996761
2                                       0.997613
4                                       0.998773
16                                      1.000000
Je vidět, že doby výpočtu pro různé hodnoty cut-off pro 36 vláken se prakticky neliší. Nic méně
z pokusů na domácím CPU s 8 jádry vykazovaly větší hodnoty cut-off lepší výsledky. Proto do 
výsledné implementace zvolíme cut-off 8.


3) Jakým způsobem zajišťujete ukládání trojúhelníků z několika vláken současně?
Ukládání trojúhelníků je řešeno stejně jako u Loop implementace. Opět byla provedena analýza pro
různé varianty ukládání trojúhelníků do paměti a pro vektorizaci. Výsledky jsou následující:
Octree implementace             Relativní průměrná délka běhu
separate_memory_vectorized                           0.276241
capture_and_add_vectorized                           0.344617
critical_section_vectorized                          0.388389
capture_and_add                                      0.934387
separate_memory                                      0.937682
critical_section                                     1.000000
Graficky pak znázorněno viz: https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/tree_performance/relative_bar.png
                             https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/tree_performance/relative_plot.png
I v tomto případě je jednoznačně nejrychlejší použití oddělené paměti a vektorizace.
                             

Úloha 3: Grafy škálování všech řešení
===============================================================================

1) Stručně zhodnoťte efektivitu vytvořených řešení (na základě grafů ŠKÁLOVÁNÍ).
Na začátek je vhodné poznamenat, že zvolená benchmark úloha není pro měření škálování úplně vhodná. 
Marching cubes algoritmus má v našem podání složitost O(n^3*m), kde n je velikost grid a m je počet
bodů v parametrickém skalárním poli. Benchmark úloha ale testuje pouze škálování počtu bodů v 
parametrickém skalárním poli, které dle složitosti nemá až tak velký vliv na celkovou dobu výpočtu. 
Lepší by bylo testovat škálování se zvětšováním obou parametrů, případně otestovat škálování všech 
různých kombinací (pouze n, pouze m a m i n současně).

Dále je vhodné poznamenat, že odevzdané grafy odpovídají konfiguraci algoritmů s oddělenými pamětmi
ale bez vektorizace. Zvolená benchmark úloha je příliš malá na to, aby se vyplatilo kopírovat
parametrické skalární pole za účelem vektorizace. Ostatní grafy si lze prohlédnout viz: 
https://github.com/xmihol00/AVS-projects/tree/main/marching_cubes/plots/scaling.

Zhodnocení silného škálování:
Silné škálování pro Loop i Octree řešení se velmi blíží k ideálnímu škálování, až na malou výchylku
u Octree implementace mezi jedním a dvěma vlákny. U Loop implementace pak můžeme pozorovat, 
neuvažujeme-li odchylky pro velikost vstupu 40, že škálování se pro největší počet vláken (32) a 
nejmenší vstupy (10, 20) nepatrně zhoršuje. Téměř ideální škálování lze pozorovat ze směrnic přímek 
pro jednotlivé velikosti vstupů, které se shora téměř blíží k -1, tj. se zdvojnásobením počtu jader 
se výpočet zrychlí prakticky dvakrát. Dle grafů silného škálování jsou obě řešení velmi efektivní.

Zhodnocení slabého škálování:
U slabého škálování bychom v ideálním případě očekávali horizontální přímky, v realitě ale s co
nejmenšími kladnými směrnicemi. V našem případě jsou směrnice u obou řešení většinou záporné, opět
až na odchylku mezi jedním a dvěma vlákny u Octree implementace. Můžeme tedy říci, že slabé 
škálování je extrémně dobré. Opět můžeme pozorovat nepatrné zhoršení až v případě, kdy je použit 
příliš velký počet vláken na příliš malou úlohu, viz měření s 10 vstupy na vlákno.


2) V jakém případě (v závislosti na počtu bodů ve vstupním souboru a velikosti 
   mřížky) bude vaše řešení 1. úlohy neefektivní? (pokud takový případ existuje)
Řešení nebude efektivní pro velké mřížky a malé počty bodů, protože většinou krychlí nebudou 
procházet žádné trojúhelníky. Loop implementace ale nijak nedokáže identifikovat podprostory, kde
se nachází takovéto krychle, a musí vyhodnotit celou mřížku, na rozdíl od Octree implementace, 
která by v tomto případě značnou část mřížky nevyhodnocovala. (Vyhodnocení prázdné krychle je, až
na generování a zápis trojúhelníků, stejně náročné jako vyhodnocení krychle s trojúhelníky.)


3) Je (nebo není) stromový algoritmus efektivnější z pohledu slabého škálování 
   vzhledem ke vstupu?
Intuitivně bychom srovnali slabé škálování Octree a Loop implementací následovně. Octree 
implementace by měla být efektivnější, pokud se zvětšuje velikost mřížky. Pak by se mělo více 
krychlí nacházet v podprostorech, kterými neprochází žádné trojúhelníky a které tak nemusí být 
vyhodnoceny. Naopak pro zvětšující se počet bodů v parametrickém skalárním poli bychom zase spíše 
očekávali lepší škálování Loop implementace, protože se zvětšuje počet krychlí, které obsahují 
trojúhelníky a které musí být vyhodnoceny, což je u Octree implementace náročnější než vyhodnocení 
prázdných krychlí.

Experimentálně naměřené hodnoty této intuici však neodpovídají. Škálování je pro obě implementace 
prakticky shodné (směrnice přímek nejsou vypovídající, protože testované úlohy svými rozměry 
neodpovídají zdvojnásobení počtu vláken):
Zvětšující se velikost mřížky         loop          tree
vlákna: 2, grid: 16              12.920000     83.780000
vlákna: 4, grid: 32             429.250000    304.150000
vlákna: 8, grid: 64            1176.516667   1217.266667
vlákna: 16, grid: 128          4703.433333   4875.216667
vlákna: 32, grid: 256         18842.700000  19658.366667
Graficky pak pro velikost mřížky znázorněno viz: https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/grid_scaling/weak_scaling.png,

Zvětšující se počet bodů 
v parametrickém skalárním poli    loop     tree
vlákna: 4, field: 4            91931.7  93479.8
vlákna: 8, field: 3            10652.7  10870.2
vlákna: 16, field: 2            1206.0   1239.8
vlákna: 32, field: 1             144.9    161.8
Graficky pak pro počet bodů v parametrickém skalárním poli znázorněno viz: https://github.com/xmihol00/AVS-projects/blob/main/marching_cubes/plots/field_scaling/weak_scaling.png

Pokud ale porovnáme odevzdané grafy slabého škálování, které testují situaci se zvětšujícím se 
počtem bodů v parametrickém skalárním poli, teoreticky by zde Loop implementace mohla být 
efektivnější než Octree implementace, pokud bychom dále testovali i pro větší počty vláken.
                         

4) Jaký je rozdíl mezi silným a slabým škálováním?
Silné škálování sleduje, jakého zrychlení paralelizace problému dosahuje při konstantní velikosti
vstupu s rostoucím počtem jader (respektive vláken, která ale mají vyhrazené fyzické jádro na čipu,
takto lze pojmy prakticky zaměnit). Řídí se Amdahlovým zákonem.

Slabé škálování naopak sleduje, jak lze při konstantní době výpočtu zvětšovat vstup s rostoucím 
počtem jader, respektive jak se mění doba výpočtu s rostoucím vstupem a rostoucím počtem jader. 
Řídí se Gustafsonovým zákonem.


Úloha 4: Analýza využití jader pomocí VTune
================================================================================

1) Jaké bylo průměrné využití jader pro všechny tři implementace s omezením na 
   18 vláken? Na kolik procent byly využity?
   
   ref: průměrně 0.998 jader - 2.8 %

Implementace s kritickou sekcí bez vektorizace:
   loop: průměrně 17.469 jader - 48.5 %
   tree: průměrně 16.387 jader - 45.5 %

Implementace s 'capture and add' bez vektorizace:
   loop: průměrně 15.303 jader - 42.5 %
   tree: průměrně 14.488 jader - 40.2 %

Implementace s privátními pamětmi bez vektorizace:
   loop: průměrně 17.431 jader - 48.4 %
   tree: průměrně 16.456 jader - 45.7 %

Implementace s kritickou sekcí s vektorizací:
   loop: průměrně 15.576 jader - 43.3 %
   tree: průměrně 11.995 jader - 33.3 %

Implementace s 'capture and add' s vektorizací:
   loop: průměrně 9.557 jader - 26.5 %
   tree: průměrně 8.316 jader - 23.1 %

Implementace s privátními pamětmi s vektorizací:
   loop: průměrně 15.197 jader - 42.2 %
   tree: průměrně 12.021 jader - 33.4 %


2) Jaké bylo průměrné využití jader pro všechny tři implementace s využitím 
   všech jader? Na kolik procent se podařilo využít obě CPU?
   
   ref: průměrně 0.998 jader - 2.8 %

Implementace s kritickou sekcí bez vektorizace:
   loop: průměrně 33.077 jader - 91.9 %
   tree: průměrně 30.567 jader - 84.8 %

Implementace s 'capture and add' bez vektorizace:
   loop: průměrně 26.354 jader - 73.2 %
   tree: průměrně 24.646 jader - 68.8 %

Implementace s privátními pamětmi bez vektorizace:
   loop: průměrně 32.595 jader - 90.5 %
   tree: průměrně 30.308 jader - 84.2 %

Implementace s kritickou sekcí s vektorizací:
   loop: průměrně 28.153 jader - 78.2 %
   tree: průměrně 21.443 jader - 59.6 %

Implementace s 'capture and add' s vektorizací:
   loop: průměrně 16.935 jader - 47.0 %
   tree: průměrně 12.550 jader - 34.9 %

Implementace s privátními pamětmi s vektorizací:
   loop: průměrně 22.350 jader - 62.1 %
   tree: průměrně 18.952 jader - 52.6 %


3) Jaké jsou závěry z těchto měření?
Z některých měření, např. pro implementaci s privátními pamětmi bez vektorizace, lze vyvodit závěr, 
že je úloha velmi dobře paralelizovatelná, protože využití jader dosahuje vysokých procent i pro 
maximální počet jader. Při porovnání výsledků pro Loop a Octree pak můžeme usoudit, že Octree 
obsahuje více režie (přidělování tasks vláknům), u které některá vlákna čekají na přidělení práce, 
a tedy procentuální využití jader je nižší. Dále lze vyvodit závěr, že čekání na přístup do 
kritické sekce se řeší pomocí aktivního čekání, jinak by bylo procentuální využití jader pro tyto 
implementace nižší. A nakonec lze udělat závěr, že maximální využití jader ne nutně vede na 
rychlejší výpočet. To je např. patrné z implementace s privátními pamětmi s vektorizací (zhoršení 
využití jader je způsobeno především sekvenčním kopírováním parametrického skalárního pole), 
při porovnání s implementací s kritickou sekcí bez vektorizace. 
