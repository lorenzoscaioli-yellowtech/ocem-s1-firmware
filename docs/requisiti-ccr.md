# Requisiti CCR — baseline di laboratorio 0.1

**Stato: proposta sintetica, non approvata dal team OCEM.** Numeri, soglie e protocollo
sono ipotesi per il laboratorio, senza pretesa di conformità a standard di prodotto.

Il controllore è chiamato ogni 10 ms esatti. I tempi si misurano in campioni
consecutivi, includendo il primo campione anomalo. Correnti in mA, tensioni in V.

| ID | Comportamento richiesto |
|---|---|
| REQ-CCR-001 | Nella variante a 5 gradini, B1–B5 corrispondono a 2800, 3400, 4100, 5200, 6600 mA. |
| REQ-CCR-002 | Nella variante a 3 gradini, B1–B3 corrispondono a 4800, 5500, 6600 mA. |
| REQ-CCR-003 | Il gradino 0 spegne; gradini o varianti non validi sono rifiutati senza modificare il riferimento richiesto. |
| REQ-CCR-004 | Inizializzazione in OFF con uscita disabilitata. Un comando non nullo avvia STARTUP. Raggiunti riferimento e misura entro ±100 mA dal target, entrare in REGULATING. |
| REQ-CCR-005 | Il riferimento varia al massimo di 100 mA ogni 10 ms, senza superare il target. Un cambio in REGULATING entra in STEP_CHANGE; al raggiungimento del riferimento torna in REGULATING. Un nuovo target durante la rampa sostituisce quello precedente. |
| REQ-CCR-006 | Una misura valida superiore a 7200 mA genera sovracorrente nello stesso tick, anche in OFF. La soglia esatta non genera questo allarme. |
| REQ-CCR-007 | Con uscita attiva e riferimento ≥500 mA, corrente <500 mA e tensione ≥3000 V per 100 ms consecutivi generano circuito aperto. Un campione regolare azzera il tempo. |
| REQ-CCR-008 | In REGULATING, errore assoluto >100 mA per 1000 ms consecutivi genera fuori tolleranza. Un campione entro tolleranza o l'uscita da REGULATING azzera il tempo. |
| REQ-CCR-009 | Ogni allarme porta in FAULT, azzera target e riferimento e disabilita l'uscita nello stesso tick. Gli allarmi sono memorizzati tramite OR. I comandi gradino sono rifiutati. Reset solo con misura valida, corrente ≤100 mA e tensione <3000 V; ritorno in OFF senza riavvio automatico. |
| REQ-CCR-010 | I temporizzatori delle protezioni saturano alla rispettiva soglia senza overflow. |
| REQ-CCR-011 | STARTUP genera allarme se il target non viene acquisito entro 3000 ms. L'acquisizione nel tick limite ha precedenza sul timeout. |
| REQ-CCR-012 | Frame di 5 byte: SOF 0xA5, comando, dato, riservato, checksum somma modulo 256 dei primi quattro byte. Frame incompleti o checksum errato non producono azioni né risposta. |
| REQ-CCR-013 | SET_STEP 0x10 usa il dato come gradino e richiede riservato=0. Risposta 0x90 con esito 0 accettato, 1 rifiutato e quarto byte 0. |
| REQ-CCR-014 | STATUS 0x20 con dato=0 e riservato=0 restituisce 0xA0, stato e maschera allarmi. Richieste malformate restituiscono 0xA0, 1, 0. |
| REQ-CCR-015 | RESET 0x30 richiede dato=0 e riservato=0, restituisce 0xB0 e l'esito. Comandi sconosciuti restituiscono comando OR 0x80, esito 2 e quarto byte 0. Tutte le risposte includono SOF e checksum. |
| REQ-CCR-016 | Il comando gradino 0 accettato disabilita subito l'uscita, senza rampa di discesa. |
| REQ-CCR-017 | HAL separata: campione ADC coerente, abilitazione e riferimento intero verso l'anello di corrente. Simulatore host con misura iniettabile e impianto ideale ritardato di un tick; niente allocazione dinamica o ricorsione. |
| REQ-CCR-018 | Campione ADC non valido genera allarme sensore nello stesso tick in qualsiasi stato e impedisce il reset. |
| REQ-CCR-019 | Il parser ignora rumore prima del SOF; dopo checksum errato conserva l'eventuale primo SOF interno come candidato per il frame successivo. SOF nel payload di un frame valido è ammesso. |
| REQ-CCR-020 | Dopo 2000 ms senza un comando valido di supervisione, con uscita attiva, memorizzare allarme comunicazione e disabilitare l'uscita. |

## Codifiche

Stati: OFF=0, STARTUP=1, REGULATING=2, STEP_CHANGE=3, FAULT=4.
Allarmi: bit 0 sovracorrente, bit 1 circuito aperto, bit 2 tolleranza,
bit 3 timeout avvio, bit 4 sensore; bit 5 riservato alla comunicazione.

Esempio SET_STEP B1: `A5 10 01 00 B6`; risposta accettata: `A5 90 00 00 35`.
Il protocollo è inventato per questa fixture e non rappresenta un protocollo OCEM.
