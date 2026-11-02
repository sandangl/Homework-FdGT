Cosa ho implementato:

1. Spotlight (Un faretto semplice e bordo sfumato - Homework 1):
   - Classe `spotlight` derivata da `light` (in light.h) con parametri ps (posizione), s (direzione puntamento), theta_0 (semiangolo apertura) e theta_1 (semiangolo cono interno derivato da smoothness).
   - Calcolo dell'angolo fra -l e s e attenuazione f_spot:
     - Per bordo netto (smoothness = 1.0, theta_1 = theta_0 = 22 gradi): f_spot e' attivo all'interno del cono e cade a zero all'esterno.
     - Per bordo sfumato (theta_i = 12 gradi, theta_o = 24 gradi, smoothness = 0.5): f_spot degrada gradualmente dal cono interno al cono esterno secondo il decadimento angolare.
   - Aggiunti test unitari per il cono.

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

4. Coordinate di texture delle primitive (Homework 8):
   - Coordinate (u, v) calcolate analiticamente in object space nel metodo hit() per tutte le primitive:
     - Sfera (sphere.h): ricavate dagli angoli polari theta e phi della normale unitaria:
       theta = arccos(clamp(ny, -1, 1)), phi = atan2(nz, nx), u = (phi + pi) / (2*pi), v = 1 - theta / pi.
     - Cilindro con tappi (cylinder.h):
       - Mantello laterale: u = 0.5 + atan2(x, z) / (2*pi), v = (y - ymin) / (ymax - ymin).
       - Tappo superiore (y = ymax): u = 0.5 + x / (2*r), v = 0.5 + z / (2*r).
       - Tappo inferiore (y = ymin): u = 0.5 - x / (2*r), v = 0.5 + z / (2*r).
         Inversione di x sul tappo inferiore: la normale uscente punta verso -Y; guardando il disco dal basso verso l'alto il semiasse +X si trova a destra, quindi proiettare +x produrrebbe un'immagine specchiata rispetto alla vista dall'alto. Invertendo il segno di x (0.5 - x/(2r)), la texture e le lettere risultano orientate correttamente da sinistra a destra anche dal basso.
     - Quadrilatero (geometry.h): proiezione piana su Q, u_vec, v_vec tramite w = n / (n · n); i parametri bilineari alpha e beta ricavati con due prodotti vettoriali costituiscono direttamente le coordinate (u, v).
     - Box / Parallelepipedo a 6 facce (geometry.h): classificazione della faccia colpita tramite la normale all'impatto per proiettare i due assi ortogonali; per tutte le facce verticali la coordinata v = (y - ymin) / Delta y preserva l'altezza e l'orientamento verticale della griglia.
   - Fattore di ripetizione e wrap frazionario in image_texture (texture.h):
     - Introduzione dei membri repeat_u e repeat_v (default 1.0).
     - Nel campionatore value(): scalatura u = u * repeat_u, v = v * repeat_v e wrapping periodico sulla parte frazionaria u = u - floor(u), v = v - floor(v).
     - Modulo circolare per gli indici dei pixel x0, x1, y0, y1 nell'interpolazione bilineare: elimina il vincolo clamp al bordo che causava striature di colore oltre i limiti [0, 1].
   - Risposta alla domanda: "Dove sono le cuciture e i poli in ciascuna forma?"
     - Sfera: due poli alle estremita' verticali (y = +-R), dove tutti i meridiani convergono a un punto singolare e la coordinata u collassa; una cucitura (seam) verticale lungo il meridiano opposto alla vista principale (phi = +-pi), dove u salta discontinuamente da 1 a 0.
     - Cilindro: nessun polo; una cucitura verticale lungo la generatrice posteriore del mantello (x = 0, z < 0; phi = +-pi) dove u salta da 1 a 0. I dischi dei tappi non hanno cuciture interne (mappatura piana continua), ma presentano una discontinuita' di texture lungo la giunzione circolare con il mantello.
     - Box: nessun polo; le cuciture sono i 12 spigoli geometrici del solido lungo i quali la proiezione cambia asse fra facce ortogonali.
     - Quadrilatero: nessun polo e nessuna cucitura interna; la mappa bilineare (alpha, beta) e' continua e biunivoca sull'intera area piana.

5. Cielo come texture equirettangolare (Homework 9):
   - Classe `environment_texture` (derivata da `image_texture`) che effettua la conversione dalla direzione sferica del raggio d in coordinate (u, v) equirettangolari, correggendo l'inversione destra/sinistra per visualizzazione dall'interno.
   - Stima automatica della direzione del sole dall'immagine della texture tramite Stochastic Gradient Ascent (SGA) con ripartenze casuali per identificare il massimo di luminanza locale/globale.
   - Calcolo del ground truth analitico del sole calcolando il baricentro pesato (centroide) dei pixel a massima luminanza.
   - Dati rilevati su texture canyon_sky.png (4096x2048):
     - Pixel SGA sole stimato: (2391, 757)
     - Centroide reale del sole: (2450.06, 724.241)
     - Errore angolare di puntamento: 5.519 gradi
   - Luce direzionale allineata automaticamente al sole stimato con ombre coerenti alla visuale.

6. Motion blur: il tempo di posa (Homework 5):
   - Campo temporale `tau` integrato nella classe `ray` (in ray.h), propagato alle istanze e preservato negli shadow ray per coerenza delle ombre in movimento.
   - Intervallo di apertura otturatore [t0, t1] e campionamento a 48 istanti nella classe `camera` (in camera.h).
   - Gestione del moto per oggetti arbitrari (primitive e mesh) tramite la classe `moving_instance` (derivata da `instance` in instance.h) con interpolazione parametrica nel tempo: vettore di traslazione T(s) = (1-s)T0 + s*T1 e angolo di rotazione theta(s) = (1-s)theta0 + s*theta1 (mai gli elementi di matrice, preservando ortogonalita' e volume).

Extra:

- Sistema di costruzione delle scene disaccoppiato dal main, per consentire di impostare scene diverse lasciando il main pulito. Utile anche per la costruzione di diversi scenari del cortometraggio.

Cosa non funziona:
Tutto cio' che e' stato richiesto e implementato funziona correttamente.