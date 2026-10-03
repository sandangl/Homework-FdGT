Cosa ho implementato:

   - Generazione dell'anaglifo con distanza interoculare variabile, con dimostrazione dei valori seguenti:
      - 1% del viewport
      - 5% del viewport
      - 6.5 cm (distanza interoculare umana) -> 245px @ 1280x720
     La distanza è comunque impostabile liberamente

   - Animazione della camera seguendo una spline di Catmull-Rom. Per generarla ho dovuto implementare anche:
     - Libreria point3_list.h per gestire una lista di punti (point3, ossia vec3)
     - Libreria vec.h, che generalizza vec3, per gestire vettori N dimensionali
     - Libreria matrix.h, basata su vec.h, che gestisce matrici NxM
     - Un sottoprogetto di test per guidare al meglio lo sviluppo delle funzionalità di calcolo matriciale
     - Libreria spline.h che fornisce le funzioni inline di calcolo per la spline di Catmull-Rom
     Le librerie per gestire matrici e vettori consentono il calcolo matriciale, che ho scelto per implementare una versione vettorizzata e veloce dell'interpolazione secondo Catmull-Rom.
     Il codice per generare i frame numerati sfrutta la funzionalità di screenshot, salvando ogni frame generato per ciascun punto della spline, il cui numero è stato calcolato in base alla durata e al framerate desiderato. Questa porzione di codice si trova direttamente in main.cpp

Cosa non funziona:

Tutto ciò che è stato implementato funziona come dovrebbe. Unica nota: non sono riuscito a renderizzare la scena in modo abbastanza fluido per produrre il video direttamente, perciò ho montato i frame in separata sede usando ffmpeg, 252 frame in totale al framerate fissato di 25, come indicato.

La documentazione completa è disponibile nel file docs.html.