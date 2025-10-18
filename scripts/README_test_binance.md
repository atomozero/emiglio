# Test Binance Login - Script di Test

⚠️ **EDUCATIONAL TOOL ONLY** - This is a learning tool for understanding API authentication. Not for real trading.

Script a riga di comando per testare le credenziali API di Binance prima di usarle in Emiglio (solo a scopo didattico).

## Compilazione

```bash
cd /boot/home/Emiglio/scripts
make -f Makefile_test_binance
```

## Utilizzo

```bash
./test_binance_login "YOUR_API_KEY" "YOUR_API_SECRET"
```

**Importante**: Usa le virgolette intorno alle credenziali per evitare problemi con caratteri speciali.

## Esempio

```bash
./test_binance_login "abcdefghijklmnopqrstuvwxyz123456" "xyz789abcdefghijklmnopqrstuvwxyz"
```

## Output del Test

Il programma esegue 5 test progressivi:

1. **Inizializzazione API** - Verifica che le credenziali siano formattate correttamente
2. **Ping** - Testa la connessione di rete a Binance
3. **Server Time** - Verifica che l'API risponda
4. **Autenticazione** - Testa che le credenziali siano valide
5. **Fetch Balances** - Scarica e mostra i balance del tuo account

### Output di Successo

```
=== Binance API Login Test ===

API Key: abcdefgh...
API Secret: ********************************

Step 1: Initializing Binance API...
✓ API initialized successfully

Step 2: Testing connection (ping)...
✓ Ping successful

Step 3: Getting server time...
✓ Server time: 1729281234

Step 4: Testing authenticated connection...
✓ Authentication successful

Step 5: Fetching account balances...
✓ Found 5 non-zero balances:

Asset          Total           Free            Locked
--------------------------------------------------------------
BTC             0.12345678      0.12345678      0.00000000
ETH             2.50000000      2.50000000      0.00000000
USDT          100.00000000     80.00000000     20.00000000
BNB             1.23456789      1.23456789      0.00000000
EUR            50.00000000     50.00000000      0.00000000

=== Test Summary ===
✓ All tests passed!
✓ API credentials are valid
✓ Connection to Binance is working
✓ Successfully retrieved 5 asset balances

You can now use these credentials in Emiglio Settings tab.
```

### Output di Errore

Se le credenziali sono sbagliate:

```
Step 4: Testing authenticated connection...
✗ Authentication test failed
  Possible reasons:
  - Invalid API Key or Secret
  - API keys don't have required permissions
  - IP restriction on API keys
```

## Come Ottenere le API Keys di Binance

1. Vai su [Binance.com](https://www.binance.com) e fai login
2. Clicca sul tuo profilo → **API Management**
3. Clicca **Create API**
4. Dai un nome alla tua API (es. "Emiglio Trading Bot")
5. Completa la verifica di sicurezza (2FA)
6. **IMPORTANTE**: Abilita solo "Read" permissions (non servono permessi di trading)
7. Copia la **API Key** e la **Secret Key**
8. (Opzionale) Restrizioni IP: Se usi sempre lo stesso computer, puoi aggiungere il tuo IP per maggiore sicurezza

## Permessi Richiesti

Per Emiglio bastano i permessi di **lettura (Read)**:
- ✅ Enable Reading
- ❌ Enable Spot & Margin Trading (NON necessario)
- ❌ Enable Withdrawals (NON necessario)

## Troubleshooting

### Errore: "Ping failed"
- Verifica la tua connessione internet
- Controlla che Binance.com sia accessibile dal tuo browser

### Errore: "Authentication test failed"
- Verifica di aver copiato correttamente API Key e Secret
- Controlla che le API keys abbiano i permessi di lettura abilitati
- Se hai impostato restrizioni IP, verifica che il tuo IP attuale sia autorizzato

### Errore: "No balances found"
- Il tuo account Binance è vuoto (nessun problema, è normale)
- Le credenziali sono comunque valide e funzionanti

## Sicurezza

⚠️ **IMPORTANTE**:
- Non condividere mai la tua API Secret con nessuno
- Le API keys vengono mostrate oscurate nello script (solo i primi 8 caratteri)
- Usa permessi di sola lettura per sicurezza
- Considera l'uso di restrizioni IP
- Se pensi che le tue keys siano compromesse, revocale immediatamente da Binance

## ⚠️ EDUCATIONAL DISCLAIMER

Questo tool è sviluppato **esclusivamente a scopo educativo** per:
- Apprendere come funziona l'autenticazione API
- Comprendere il funzionamento delle richieste HTTPS autenticate
- Studiare la gestione sicura delle credenziali
- Esplorare l'integrazione con exchange di criptovalute

**NON è destinato al trading reale**. Il trading comporta rischi sostanziali e richiede:
- Formazione finanziaria adeguata
- Piena comprensione dei rischi
- Consulenza professionale
- Preparazione psicologica e disciplina

Usalo solo per imparare, non per fare trading senza la dovuta preparazione.

## Prossimi Passi

Una volta verificato che le credenziali funzionano:

1. Avvia Emiglio: `../objects.x86_64-cc13-release/Emiglio`
2. Vai al tab **Settings**
3. Inserisci le stesse API Key e Secret
4. Clicca **Save** (verranno criptate con AES-256)
5. Vai al tab **Dashboard** per vedere il tuo portafoglio

## Pulizia

Per rimuovere i file compilati:

```bash
make -f Makefile_test_binance clean
```
