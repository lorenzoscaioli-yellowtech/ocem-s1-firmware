# CCR — modulo di controllo e supervisione

Firmware **sintetico di laboratorio** per un regolatore di corrente costante (CCR) di un circuito
luci di pista. Non è codice OCEM e non è qualificato per pilotare apparati reali: hardware, soglie e
protocollo sono inventati. Gira sul PC grazie a una HAL simulata.

## Compilazione

Servono un compilatore C99 e, se c'è, `make`. Nessuna libreria aggiuntiva, nessuna rete.

```sh
make          # build/ccr_sim
make test     # esegue i test
make demo     # scenario: avvio B3, passaggio a B5, spegnimento
make clean
```

Senza `make`, gli stessi passi a mano (gcc o clang):

```sh
mkdir -p build
cc -Isrc -std=c99 -Wall -Wextra -pedantic src/ccr_ctrl.c src/ccr_steps.c src/ccr_protect.c src/ccr_comm.c src/hal_sim.c tests/test_ccr.c -o build/test_ccr
./build/test_ccr
```

## Organizzazione

- `src/ccr_ctrl.c`: ciclo periodico, gestione comandi e macchina a stati.
- `src/ccr_steps.c`: tabelle delle due varianti (3 e 5 gradini).
- `src/ccr_protect.c`: soglie, temporizzazioni e bit di allarme.
- `src/ccr_comm.c`: parser incrementale a memoria fissa e risposte.
- `src/hal_sim.c`, `src/hal.h`: adattatore hardware e implementazione host.
- `src/main_sim.c`: scenario dimostrativo.
- `tests/test_ccr.c`: test deterministici con riferimenti `@req`.
- [docs/requisiti-ccr.md](docs/requisiti-ccr.md): requisiti del modulo.

## Contratto di integrazione

Inizializzare HAL, controllore e parser una sola volta. Invocare `ccr_ctrl_tick()` ogni 10 ms;
serializzare i comandi nello stesso contesto. Gli interrupt possono alimentare una coda RX esterna:
non devono modificare il controllore. Servire `ccr_comm_feed()` per ogni byte e trasmettere la
risposta solo quando restituisce `true`. Nessuna gestione UART, DMA o concorrenza è inclusa.

La HAL espone un **riferimento all'anello di corrente**, non un duty-cycle calibrato. Il modello host
segue idealmente il riferimento al tick successivo: serve a verificare il software, non la stabilità
del regolatore o il comportamento elettrico.

Le protezioni sono valutate sul campione corrente e sul riferimento del tick precedente. La
tolleranza è sorvegliata in REGULATING, il circuito aperto anche durante le rampe; sovracorrente e
validità ADC sono valutate in tutti gli stati.
