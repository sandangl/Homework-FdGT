Cosa ho implementato:

1. Spotlight (Un faretto semplice e bordo sfumato - Homework 1):
   - Classe `spotlight` derivata da `light` (in light.h) con parametri ps (posizione), s (direzione puntamento), theta_0 (semiangolo apertura) e theta_1 (semiangolo cono interno derivato da smoothness).
   - Calcolo dell'angolo fra -l e s e attenuazione f_spot:
     - Per bordo netto (smoothness = 1.0, theta_1 = theta_0 = 22 gradi): f_spot e' attivo all'interno del cono e cade a zero all'esterno.
     - Per bordo sfumato (theta_i = 12 gradi, theta_o = 24 gradi, smoothness = 0.5): f_spot degrada gradualmente dal cono interno al cono esterno secondo il decadimento angolare.
   - Supporto completo nei test unitari (14/14 superati in tests.exe).

2. Più sorgenti di luce e ombre morbide (Homework 3):
   - Estensione della gestione luci tramite `light_list`, in cui convivono tipi differenti di luce:
     - Luce puntiforme (`point_light`)
     - Luce direzionale (`directional_light`)
     - Faretto spot (`spotlight`)
     - Luce estesa rettangolare con campionamento stocastico Monte Carlo (`ext_light` / `point_ext_light`)
   - Shading multi-luce con shadow ray indipendente per ogni sorgente, visibilita' Vi, attenuazione proporzionale al numero di sorgenti per evitare saturazione al bianco, e componente ambientale calcolata una volta sola per scena.
     - Ombre morbide con luce estesa:
     Misurazione dei tempi di calcolo (risoluzione 700x393, multi-threaded PPL):
     Le luce estesa è stata implementata come una nuvola probabilistica bidimensionale (quadrilatero), generando a caso N luci puntiformi.
     - Luce puntiforme (baseline): 0.042 s -> 3_ombre_morbide_puntiforme.bmp
     - Luce estesa con N = 1 campione: 0.042 s -> 3_ombre_morbide_N1.bmp
     - Luce estesa con N = 16 campioni: 0.221 s -> 3_ombre_morbide_N16.bmp
     - Luce estesa con N = 256 campioni: 3.612 s -> 3_ombre_morbide_N256.bmp
     All'aumentare di N il rumore stocastico nella penombra scompare progressivamente lasciando una penombra continua e morbida; il costo cresce linearmente con N shadow ray per pixel.

3. Contorno, riflesso e Toon Shading (Homework 4):
   - Flag `is_toon`, livelli `K` e soglia `epsilon` integrati nella classe `material` (con costruttori dedicati) e sottoclasse `toon_material`.
   - Quantizzazione del termine diffuso su K livelli: d = ceil(K * max(n . l, 0)) / K.
   - Riflesso speculare a macchia netta: piena se r . v > 0.95, altrimenti nulla.
   - Contorno nero (silhouette) calcolato valutando |n · v| < epsilon. Disabilitato sul piano del pavimento tramite flag per evitare l'annerimento dell'orizzonte.
   - Risposta alla domanda: "Dove sbaglia il contorno calcolato con |n · v|?"
     Il test |n · v| < epsilon fallisce sulle superfici piane/poligonali viste radenti (come un pavimento o facce inclinate di taglio), dove la normale e' quasi perpendicolare alla vista anche se il punto non e' un contorno esterno della sagoma (silhouette); inoltre, lo spessore della linea in pixel non e' costante ma dipende dalla curvatura locale della geometria e dalla distanza della camera, e non rileva i bordi di occlusione interni fra parti diverse dello stesso modello aventi normali simili.

4. Cielo come texture equirettangolare (Homework 9):
   - Classe `environment_texture` (derivata da `image_texture`) che effettua la conversione dalla direzione sferica del raggio d in coordinate (u, v) equirettangolari, correggendo l'inversione destra/sinistra per visualizzazione dall'interno.
   - Stima automatica della direzione del sole dall'immagine della texture tramite Stochastic Gradient Ascent (SGA) con ripartenze casuali per identificare il massimo di luminanza locale/globale.
   - Calcolo del ground truth analitico del sole calcolando il baricentro pesato (centroide) dei pixel a massima luminanza.
   - Dati rilevati su texture canyon_sky.png (4096x2048):
     - Pixel SGA sole stimato: (2391, 757)
     - Centroide reale del sole: (2450.06, 724.241)
     - Errore angolare di puntamento: 5.519 gradi
   - Luce direzionale allineata automaticamente al sole stimato con ombre coerenti alla visuale.


Cosa non funziona:
Tutto cio' che e' stato richiesto e implementato funziona correttamente. Le immagini sono generate nei formati e nelle risoluzioni indicate, i tempi di calcolo per le ombre morbide scalano coerentemente e la suite di test unitari ha esito 100% positivo (14 test su 14 superati).
